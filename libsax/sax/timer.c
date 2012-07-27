#include <stdlib.h>
#include "timer.h"
#include "compiler.h"
#include "mempool.h"
#include "os_api.h"

typedef struct timer_node timer_node;

struct timer_node
{
	timer_node* next;
	timer_node* prev;
	g_timer_proc func;
	void* user_data;
	uint32_t expire;
};

typedef struct g_timer_t
{
	timer_node* lv1;
	timer_node* lv2;
	timer_node* lv3;
	timer_node* lv4;
	g_xslab_t* pool;
	uint32_t last;
	uint32_t count;
	int32_t destroying;
} g_timer_t;

static void remove_node(timer_node* node)
{
	node->prev->next = node->next;
	node->next->prev = node->prev;
	node->prev = node->next = NULL;
}

static void schedule(g_timer_t* t, timer_node* node, uint32_t delay_ms)
{
	timer_node* list_head;

	if (delay_ms <= 0x0ffFF) {
		if (delay_ms > 0x0FF)
		{ /*lv2*/ list_head = &t->lv2[(node->expire >> 8) & 0x0FF]; }
		else
		{ /*lv1*/ list_head = &t->lv1[(node->expire >> 0) & 0x0FF]; }
	}
	else {
		if (delay_ms <= 0x0FFffFF)
		{ /*lv3*/ list_head = &t->lv3[(node->expire >> 16) & 0x0FF]; }
		else
		{ /*lv4*/ list_head = &t->lv4[(node->expire >> 24) & 0x0FF]; }
	}

	list_head->next->prev = node;
	node->next = list_head->next;
	list_head->next = node;
	node->prev = list_head;
}

static void reschedule(g_timer_t* t, timer_node* list_head, uint32_t now)
{
	timer_node* node = list_head->next;
	while (node != list_head) {
		timer_node* tmp = node->next;
		schedule(t, node, node->expire - now);
		node = tmp;
	}
	list_head->next = list_head->prev = list_head;
}

static void fire(g_timer_t* t, timer_node* list_head)
{
	timer_node* node = list_head->next;
	while (node != list_head) {
		timer_node* tmp = node->next;
		remove_node(node);

		node->func((g_timer_handle_t) node, node->user_data);

		g_xslab_free(t->pool, node);
		node = tmp;
		--(t->count);
	}
}

g_timer_t* g_timer_create(uint32_t now_ms)
{
	g_timer_t* timer = (g_timer_t*) malloc(sizeof(g_timer_t));
	if (timer == NULL) return NULL;

	timer->lv1 = (timer_node*) malloc(4 * 256 * sizeof(timer_node));
	timer->pool = g_xslab_init(sizeof(timer_node));
	if (timer == NULL || timer->pool == NULL) {
		goto failed;
	}

	if (now_ms == 0) now_ms = (uint32_t) g_now_ms();

	timer->last = now_ms;
	timer->count = 0;
	timer->destroying = 0;

	int32_t i;
	for (i = 0; i < 4 * 256; i++) {
		timer->lv1[i].next = timer->lv1[i].prev = &timer->lv1[i];
	}

	timer->lv2 = timer->lv1 + 256;
	timer->lv3 = timer->lv2 + 256;
	timer->lv4 = timer->lv3 + 256;

	return timer;

failed:
	if (timer != NULL && timer->lv1 != NULL) free(timer->lv1);
	if (timer != NULL && timer->pool != NULL) g_xslab_destroy(timer->pool);
	if (timer != NULL) free(timer);

	return NULL;
}

void g_timer_destroy(g_timer_t* t, int32_t fire_all)
{
	t->destroying = 1;	// not allow adding new timer

	int32_t i;
	if (fire_all) {
		for (i = 0; i < 4 * 256; i++) {
			timer_node* list_head = &t->lv1[i];
			fire(t, list_head);
		}
	}
	else {
		// just recycle timer_node
		for (i = 0; i < 4 * 256; i++) {
			timer_node* list_head = &t->lv1[i];
			timer_node* node = list_head->next;
			while (node != list_head) {
				timer_node* tmp = node->next;
				g_xslab_free(t->pool, node);
				node = tmp;
			}
		}
	}

	g_xslab_destroy(t->pool);
	free(t->lv1);
	free(t);
}

g_timer_handle_t g_timer_start(g_timer_t* t, uint32_t delay_ms,
		g_timer_proc func, void* user_data)
{
	if (UNLIKELY(t->destroying)) return NULL;

	timer_node* node = (timer_node*) g_xslab_alloc(t->pool);
	if (UNLIKELY(node == NULL)) return NULL;

	node->func = func;
	node->user_data = user_data;
	node->expire = t->last + delay_ms;

	schedule(t, node, delay_ms);

	++(t->count);

	return (g_timer_handle_t) node;
}

int32_t g_timer_cancel(g_timer_t* t, g_timer_handle_t handle, void** user_data)
{
	timer_node* node = (timer_node*) handle;

	if (node->next == NULL) return 1;

	if (user_data) *user_data = node->user_data;

	--(t->count);
	remove_node(node);
	g_xslab_free(t->pool, node);

	return 0;
}

void g_timer_poll(g_timer_t* t, uint32_t now_ms)
{
	if (now_ms == 0) now_ms = (uint32_t) g_now_ms();
	while (t->last < now_ms) {
		++(t->last);
		timer_node* list_head;
		if (LIKELY(t->last & 0x0FF)) {
			list_head = &t->lv1[t->last & 0x0FF];
		}
		else if (LIKELY(t->last & 0x0ffFF)) {
			reschedule(t, &t->lv2[(t->last >> 8) & 0x0FF], t->last);
			list_head = &t->lv1[0];
		}
		else if (LIKELY(t->last & 0x0FFffFF)) {
			reschedule(t, &t->lv3[(t->last >> 16) & 0x0FF], t->last);
			reschedule(t, &t->lv2[0], t->last);
			list_head = &t->lv1[0];
		}
		else {
			reschedule(t, &t->lv4[(t->last >> 24) & 0x0FF], t->last);
			reschedule(t, &t->lv3[0], t->last);
			reschedule(t, &t->lv2[0], t->last);
			list_head = &t->lv1[0];
		}

		fire(t, list_head);
	}
}

uint32_t g_timer_count(g_timer_t* t)
{
	return t->count;
}

void g_timer_shrink_mempool(g_timer_t* t, double keep)
{
	g_xslab_shrink(t->pool, keep);
}
