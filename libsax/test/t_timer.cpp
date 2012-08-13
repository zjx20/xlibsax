#include <sax/os_api.h>
#include <sax/timer.h>
#include <sax/stage/stimer.h>

#include <stdio.h>

#include "gtest/gtest.h"

static void inc_one(g_timer_handle_t handle, void* param)
{
	if (param != NULL) (*((int*)param))++;
}

TEST(timer, basic_test1)
{
	g_timer_t* timer = g_timer_create(1);

	volatile int a = 100;

	ASSERT_TRUE(NULL != g_timer_start(timer, 1, inc_one, (void*) &a));
	ASSERT_TRUE(NULL != g_timer_start(timer, 1, inc_one, (void*) &a));
	ASSERT_TRUE(NULL != g_timer_start(timer, 3, inc_one, (void*) &a));
	ASSERT_TRUE(NULL != g_timer_start(timer, 4, inc_one, (void*) &a));
	ASSERT_TRUE(NULL != g_timer_start(timer, 6, inc_one, (void*) &a));

	g_timer_handle_t ch = g_timer_start(timer, 7, inc_one, (void*) &a);
	ASSERT_TRUE(NULL != ch);

	g_timer_poll(timer, 2);
	ASSERT_EQ(102, a);

	g_timer_poll(timer, 5);
	ASSERT_EQ(104, a);

	ASSERT_EQ(0, g_timer_cancel(timer, ch, NULL));
	ASSERT_EQ(1, g_timer_cancel(timer, ch, NULL));

	g_timer_poll(timer, 10);
	ASSERT_EQ(105, a);

	ASSERT_TRUE(NULL != g_timer_start(timer, 256, inc_one, (void*) &a));

	g_timer_poll(timer, 265);
	ASSERT_EQ(105, a);
	g_timer_poll(timer, 267);
	ASSERT_EQ(106, a);

	ASSERT_TRUE(NULL != g_timer_start(timer, 0x01000100, inc_one, (void*) &a));

	g_timer_poll(timer, 0x01000000);
	ASSERT_EQ(106, a);
	g_timer_poll(timer, 0x01000100 + 268);
	ASSERT_EQ(107, a);

	g_timer_destroy(timer, 0);
}

TEST(timer, basic_test2)
{
	g_timer_t* timer = g_timer_create(0);

	volatile int a = 100;

	ASSERT_TRUE(NULL != g_timer_start(timer, 100, inc_one, (void*) &a));
	ASSERT_TRUE(NULL != g_timer_start(timer, 100, inc_one, (void*) &a));
	ASSERT_TRUE(NULL != g_timer_start(timer, 500, inc_one, (void*) &a));
	ASSERT_TRUE(NULL != g_timer_start(timer, 600, inc_one, (void*) &a));
	ASSERT_TRUE(NULL != g_timer_start(timer, 900, inc_one, (void*) &a));

	g_thread_sleep(0.200);
	g_timer_poll(timer, 0);
	ASSERT_EQ(102, a);

	g_thread_sleep(0.500);
	g_timer_poll(timer, 0);
	ASSERT_EQ(104, a);

	g_thread_sleep(0.300);
	g_timer_poll(timer, 0);
	ASSERT_EQ(105, a);

	ASSERT_TRUE(NULL != g_timer_start(timer, 500, inc_one, (void*) &a));

	g_thread_sleep(0.6);
	g_timer_poll(timer, 0);
	ASSERT_EQ(106, a);

	g_timer_destroy(timer, 0);
}

TEST(timer, linkedlist)
{
	g_timer_t* timer = g_timer_create(0);

	ASSERT_EQ(0, g_timer_count(timer));

	g_timer_handle_t handle1 = g_timer_start(timer, 1, inc_one, NULL);
	ASSERT_TRUE(handle1 != NULL);
	ASSERT_EQ(1, g_timer_count(timer));

	g_timer_handle_t handle2 = g_timer_start(timer, 1, inc_one, NULL);
	ASSERT_TRUE(handle2 != NULL);
	ASSERT_EQ(2, g_timer_count(timer));

	ASSERT_EQ(0, g_timer_cancel(timer, handle2, NULL));
	ASSERT_EQ(1, g_timer_count(timer));

	handle2 = g_timer_start(timer, 1, inc_one, NULL);
	ASSERT_TRUE(handle2 != NULL);
	ASSERT_EQ(2, g_timer_count(timer));

	ASSERT_EQ(0, g_timer_cancel(timer, handle1, NULL));
	ASSERT_EQ(1, g_timer_count(timer));

	handle1 = g_timer_start(timer, 1, inc_one, NULL);
	ASSERT_TRUE(handle1 != NULL);
	ASSERT_EQ(2, g_timer_count(timer));

	ASSERT_EQ(0, g_timer_cancel(timer, handle1, NULL));
	ASSERT_EQ(0, g_timer_cancel(timer, handle2, NULL));
	ASSERT_EQ(0, g_timer_count(timer));

	g_timer_destroy(timer, 0);
}

TEST(timer, destroy_fire_all)
{
	g_timer_t* timer = g_timer_create(0);

	volatile int a = 100;

	ASSERT_TRUE(NULL != g_timer_start(timer, 100, inc_one, (void*) &a));
	ASSERT_TRUE(NULL != g_timer_start(timer, 100, inc_one, (void*) &a));
	ASSERT_TRUE(NULL != g_timer_start(timer, 500, inc_one, (void*) &a));
	ASSERT_TRUE(NULL != g_timer_start(timer, 600, inc_one, (void*) &a));
	ASSERT_TRUE(NULL != g_timer_start(timer, 900, inc_one, (void*) &a));

	g_thread_sleep(0.200);
	g_timer_poll(timer, 0);
	ASSERT_EQ(102, a);

	g_timer_destroy(timer, 1);

	ASSERT_EQ(105, a);
}

TEST(timer, destroy_not_fire_all)
{
	g_timer_t* timer = g_timer_create(0);

	volatile int a = 100;

	ASSERT_TRUE(NULL != g_timer_start(timer, 100, inc_one, (void*) &a));
	ASSERT_TRUE(NULL != g_timer_start(timer, 100, inc_one, (void*) &a));
	ASSERT_TRUE(NULL != g_timer_start(timer, 500, inc_one, (void*) &a));
	ASSERT_TRUE(NULL != g_timer_start(timer, 600, inc_one, (void*) &a));
	ASSERT_TRUE(NULL != g_timer_start(timer, 900, inc_one, (void*) &a));

	g_thread_sleep(0.200);
	g_timer_poll(timer, 0);
	ASSERT_EQ(102, a);

	g_timer_destroy(timer, 0);

	ASSERT_EQ(102, a);
}

static void cancel_and_add(g_timer_handle_t handle, void* param)
{
	g_timer_t* timer = (g_timer_t*)((void**)param)[0];
	g_timer_handle_t* ch = (g_timer_handle_t*)((void**)param)[1];
	int* a = (int*)((void**)param)[2];

	g_timer_cancel(timer, *ch, NULL);

	(*a) += 1;
}

TEST(timer, cancel_when_firing)
{
	int a = 100;
	int b = 100;
	int c = 100;

	g_timer_handle_t ch = NULL;

	g_timer_t* timer = g_timer_create(0);

	void* param1[] = {timer, &ch, &a};
	void* param2[] = {timer, &ch, &b};
	void* param3[] = {timer, &ch, &c};

	ASSERT_TRUE(NULL != g_timer_start(timer, 100, cancel_and_add, param1));

	ch = g_timer_start(timer, 100, cancel_and_add, param2);
	ASSERT_TRUE(NULL != ch);

	ASSERT_TRUE(NULL != g_timer_start(timer, 100, cancel_and_add, param3));

	g_thread_sleep(0.200);
	g_timer_poll(timer, 0);

	ASSERT_EQ(101, a);
	ASSERT_EQ(100, b);
	ASSERT_EQ(101, c);

	g_timer_destroy(timer, 0);
}

class test_handler : public sax::handler_base
{
public:
	virtual bool init(void* param) {return true;}
	virtual void on_start(int32_t thread_id) {printf("test_handler started, thread id: %d.\n", thread_id);}
	virtual void on_finish(int32_t thread_id) {printf("test_handler finished, thread id: %d.\n", thread_id);}
	virtual ~test_handler() {}

	virtual void on_event(const sax::event_type *ev)
	{
		switch(ev->get_type())
		{
		case sax::timer_timeout_event::ID:
		{
			sax::timer_timeout_event* event = (sax::timer_timeout_event*) ev;
			printf("test_handler recv timer_timeout_event. trans_id: %lu.\n", event->trans_id);
			(*((int*)event->invoke_param))++;
			break;
		}
		default:
			assert(0);
			break;
		}
	}
};

TEST(stimer, basic_test)
{
	sax::stage* test_stage = sax::create_stage<test_handler, sax::thread_obj>(
			"test_stage", 4, NULL, 1024, new sax::default_dispatcher());

	sax::stage* timer = sax::create_stimer();

	volatile int a = 100;

	{
		sax::add_timer_event ev;
		ev.biz_stage = test_stage;
		ev.delay_ms = 50;
		ev.invoke_param = (void*)&a;
		ev.trans_id = 1;
		timer->push_event(&ev);
	}

	g_thread_sleep(0.051);
	ASSERT_EQ(101, a);

	{
		sax::add_timer_event ev;
		ev.biz_stage = test_stage;
		ev.delay_ms = 50;
		ev.invoke_param = (void*)&a;
		ev.trans_id = 2;
		timer->push_event(&ev);
	}

	{
		sax::add_timer_event ev;
		ev.biz_stage = test_stage;
		ev.delay_ms = 51;
		ev.invoke_param = (void*)&a;
		ev.trans_id = 3;
		timer->push_event(&ev);
	}

	{
		sax::add_timer_event ev;
		ev.biz_stage = test_stage;
		ev.delay_ms = 150;
		ev.invoke_param = (void*)&a;
		ev.trans_id = 4;
		timer->push_event(&ev);
	}

	{
		sax::add_timer_event ev;
		ev.biz_stage = test_stage;
		ev.delay_ms = 1500;
		ev.invoke_param = (void*)&a;
		ev.trans_id = 5;
		timer->push_event(&ev);
	}

	g_thread_sleep(0.100);
	ASSERT_EQ(103, a);

	g_thread_sleep(0.100);
	ASSERT_EQ(104, a);

	g_thread_sleep(1.500);
	ASSERT_EQ(105, a);

	sax::stage_mgr::get_instance()->stop_all();
}

int main( int argc, char *argv[] )
{
    testing::InitGoogleTest(&argc, argv);
    srand((unsigned)time(NULL));

    return RUN_ALL_TESTS();
}
