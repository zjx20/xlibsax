#include "timer.h"
#include "os_api.h"

#include <memory.h>
#include <stdlib.h>

#pragma pack(4)
typedef struct timer_link 
{
	struct timer_link *prev;
	struct timer_link *next;
} timer_link;

/// @brief a timer node for the time list
typedef struct timer_node 
{
	timer_link chain;
	g_timer_event func;
	void *client;
	void *param;
	uint64_t dead_usec;
} timer_node;

/// @brief Timer Wheel Structure used to manage the timer wheel
/// and keep stats to help understand performance
struct g_timer_t {
	uint32_t capacity;
	uint32_t wheel_size;
	uint32_t granularity;
	uint32_t spoke_index;
	uint64_t bench_usec;
	timer_node *nodes;
	timer_link *spokes;
	timer_link  idles;
	g_mutex_t  *mutex;
};
#pragma pack()

g_timer_t *g_timer_create(
	uint32_t capacity, uint32_t wheel_size, uint32_t granularity)
{
	if (capacity && wheel_size>=32 && 
		wheel_size<=4096 && granularity>=1 && granularity<=1000)
	{
		timer_link *last;
		uint32_t i;
		
		g_timer_t *stw = (g_timer_t *) malloc(sizeof(g_timer_t));
		if (!stw) return NULL;
		
		stw->nodes = (timer_node *) calloc(capacity, sizeof(timer_node));
		stw->spokes = (timer_link *) malloc(wheel_size*sizeof(timer_link));
		stw->mutex = g_mutex_init();
		if (!stw->nodes || !stw->spokes || !stw->mutex) 
		{
			g_timer_destroy(stw); return NULL;
		}
		
		// initialize spoke list
		for (i=0; i<wheel_size; i++)
		{
			timer_link *spoke = &stw->spokes[i];
			spoke->prev = spoke->next = spoke;
		}
		
		// initialize idle list
		last = &stw->idles;
		for (i=0; i<capacity; i++)
		{
			timer_link *tmr = &stw->nodes[i].chain;
			last->next = tmr;
			tmr->prev = last;
			last = tmr; // loop to next
		}
		last->next = &stw->idles;
		stw->idles.prev = last;
		
		stw->bench_usec = g_now_hr();
		stw->spoke_index = 0;
		
		stw->capacity = capacity;
		stw->wheel_size = wheel_size;
		stw->granularity = granularity*1000;
		return stw;
	}
	return NULL;
}

void g_timer_destroy(g_timer_t *stw)
{
	if (stw) {
		if (stw->spokes) free(stw->spokes);
		if (stw->nodes) free(stw->nodes);
		if (stw->mutex) g_mutex_free(stw->mutex);
		free(stw);
	}
}

// event_node_s/event_list_s are designed for a vector.
// they cache the events and call together later.
// this trick reduces the thread-context switching.
// ---optimized by Qingshan, 2010/3/15
typedef struct {
	g_timer_event func;
	void *client;
	void *param;
	uint32_t id;
} event_node_s;

typedef struct {
	uint32_t capacity;
	uint32_t size;
	event_node_s node[0];
} event_list_s;

static event_list_s *event_list_add(
	event_list_s *el, uint32_t id, const timer_node *node)
{
	event_node_s *s;
	if (!el) {
		const uint32_t cap = 256;
		el = (event_list_s *) malloc(
			sizeof(event_list_s) + cap*sizeof(event_node_s));
		if (!el) return NULL;
		el->size = 0;
		el->capacity = cap;
	}
	else if (el->size == el->capacity) {
		uint32_t cap = el->capacity*2;
		event_list_s *neu = (event_list_s *) realloc(el, 
			sizeof(event_list_s) + cap*sizeof(event_node_s));
		if (!neu) return el; // todo, handle the error
		(el = neu)->capacity = cap;
	}
	s = &el->node[el->size++];
	s->id = id;
	s->func = node->func;
	s->client = node->client;
	s->param = node->param;
	return el;
}

static void event_list_end(event_list_s *el)
{
	uint32_t i = 0;
	for (; i < el->size; i++) {
		event_node_s *s = &el->node[i];
		s->func(s->id, s->client, s->param);
	}
	free(el);
}

void g_timer_loop(g_timer_t *stw)
{
	event_list_s *el = NULL;
	uint64_t acc, now; 
	g_mutex_enter(stw->mutex);
	
	acc = stw->bench_usec + stw->granularity;
	now = g_now_hr();
	while (acc <= now)
	{
		timer_link *spoke, *tmr;
		timer_node *node;
		
		// advance the index to the next spoke
		stw->bench_usec = acc;
		stw->spoke_index = (stw->spoke_index + 1 ) % stw->wheel_size;
		acc += stw->granularity;
		
		// process the spoke
		spoke = &stw->spokes[stw->spoke_index];
		tmr = spoke->next;
		
		// removing timers that have expired
		while (tmr != spoke && 
			(node = (timer_node *) tmr)->dead_usec < acc) 
		{
			long id = ((long)node - (long)stw->nodes)/sizeof(timer_node)+1;
			
			// remove from spoke list
			timer_link *prev = tmr->prev;
			timer_link *next = tmr->next;
			prev->next = next;
			next->prev = prev;
			
			// add callback etc. to event_list_s
			el = event_list_add(el, id, node);
			node->func = NULL;
			
			// add node to idle list
			stw->idles.prev->next = tmr;
			tmr->next = &stw->idles;
			tmr->prev = stw->idles.prev;
			stw->idles.prev = tmr;
			
			tmr = next; // loop to next
		}
	}
	g_mutex_leave(stw->mutex);
	if (el) event_list_end(el);
}

uint32_t g_timer_start(g_timer_t *stw, uint32_t delay, 
	g_timer_event func, void *client, void *param)
{ 
	timer_link *idle = &stw->idles;
	timer_link *tmr;
	uint32_t id = 0;
	g_mutex_enter(stw->mutex);
	
	if (NULL != func && (tmr = idle->next) != idle) 
	{
		uint64_t dead_usec = g_now_hr() + delay*1000;
		uint32_t u = dead_usec - stw->bench_usec;
		uint32_t t = (u < stw->granularity ? 1 : u/stw->granularity);
		uint32_t j = ((stw->spoke_index + t) % stw->wheel_size);
		
		timer_link *spoke = &stw->spokes[j];
		timer_link *last = spoke->prev;
		timer_node *node = (timer_node *) tmr;
		
		// remove from idle list
		timer_link *prev = tmr->prev;
		timer_link *next = tmr->next;
		prev->next = next;
		next->prev = prev;
		
		// finds the nice position of spoke
		for (; spoke != last; last = last->prev) {
			timer_node *p = (timer_node *) last;
			if (dead_usec >= p->dead_usec) break;
		}
		
		// adds to the nice position
		tmr->next = last->next;
		tmr->prev = last;
		last->next->prev = tmr;
		last->next  = tmr;
		
		node->dead_usec = dead_usec;
		node->func = func;
		node->client = client;
		node->param = param;
		
		id = ((long)node - (long)stw->nodes)/sizeof(timer_node)+1;
	}
	
	g_mutex_leave(stw->mutex);
	return id;
}

int g_timer_cancel(g_timer_t *stw, uint32_t id)
{
	if (id && id <= stw->capacity) 
	{
		timer_node *node = &stw->nodes[id-1];
		g_mutex_enter(stw->mutex);
		
		if (NULL == node->func) {
			id = 0; // already in the idle list?
		}
		else {
			timer_link *tmr = &node->chain;
			
			// remove from spoke list
			timer_link *prev = tmr->prev;
			timer_link *next = tmr->next;
			prev->next = next;
			next->prev = prev;
			
			// append to idle list
			stw->idles.prev->next = tmr;
			tmr->next = &stw->idles;
			tmr->prev = stw->idles.prev;
			stw->idles.prev = tmr;
			
			node->func = NULL;
		}
		g_mutex_leave(stw->mutex);
		return id;
	}
	return 0;
}
