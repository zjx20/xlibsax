#include <sax/os_api.h>
#include <sax/timer.h>

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

	g_timer_poll(timer, 2);
	ASSERT_EQ(102, a);

	g_timer_poll(timer, 5);
	ASSERT_EQ(104, a);

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

}

TEST(timer, destroy_not_fire_all)
{

}

TEST(timer, cancel_when_destroy)
{

}

TEST(timer, cancel_when_firing)
{

}

TEST(timer, shrink)
{

}

int main( int argc, char *argv[] )
{
    testing::InitGoogleTest(&argc, argv);
    srand((unsigned)time(NULL));

    return RUN_ALL_TESTS();
}
