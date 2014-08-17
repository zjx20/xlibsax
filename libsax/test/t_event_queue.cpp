/*
 * t_event_queue.cpp
 *
 *  Created on: 2012-8-20
 *      Author: x
 */

#include "sax/stage/event_queue.h"
#include "gtest/gtest.h"

struct test_event : public sax::user_event_base<123, test_event>
{
	char body[64 - sizeof(sax::user_event_base<123, test_event>)];
};

TEST(event_queue, roll_back)
{
	{
		sax::event_queue queue(200);

		test_event* ev = queue.allocate_event<test_event>(false);
		sax::event_queue::commit_event(ev);

		test_event* first_pos = ev;
		ASSERT_EQ(first_pos, queue.pop_event());
		queue.destroy_event(first_pos);

		ev = queue.allocate_event<test_event>(false);
		sax::event_queue::commit_event(ev);
		ASSERT_EQ(ev, queue.pop_event());
		queue.destroy_event(ev);

		ASSERT_EQ(first_pos, queue.allocate_event<test_event>(false));
	}

	{
		sax::event_queue queue(sizeof(test_event) + 8);	// exactly one event

		test_event* ev = queue.allocate_event<test_event>(false);
		sax::event_queue::commit_event(ev);

		ASSERT_EQ(NULL, queue.allocate_event<test_event>(false));

		ASSERT_EQ(ev, queue.pop_event());

		queue.destroy_event(ev);

		ASSERT_EQ(NULL, queue.allocate_event<test_event>(false));
	}

	{
		sax::event_queue queue((sizeof(test_event) + 8) * 3);	// exactly 3 events

		test_event* ev = queue.allocate_event<test_event>(false);
		sax::event_queue::commit_event(ev);
		ASSERT_EQ(ev, queue.pop_event());
		queue.destroy_event(ev);

		test_event* first = ev;

		ev = queue.allocate_event<test_event>(false);
		sax::event_queue::commit_event(ev);
		ASSERT_EQ(ev, queue.pop_event());
		queue.destroy_event(ev);

		ev = queue.allocate_event<test_event>(false);
		sax::event_queue::commit_event(ev);
		ASSERT_EQ(ev, queue.pop_event());
		queue.destroy_event(ev);

		ASSERT_EQ(first, queue.allocate_event<test_event>(false));
	}
}

TEST(event_queue, push_and_pop)
{
	{
		sax::event_queue queue((sizeof(test_event) + 8) * 3);	// exactly 3 events

		ASSERT_EQ(NULL, queue.pop_event());

		test_event* ev = queue.allocate_event<test_event>(false);

		ASSERT_EQ(NULL, queue.pop_event());

		sax::event_queue::commit_event(ev);

		ASSERT_EQ(ev, queue.pop_event());
		queue.destroy_event(ev);
	}
}

TEST(event_queue, no_space)
{
	{
		sax::event_queue queue(sizeof(test_event) - 8);	// less than one event

		ASSERT_EQ(NULL, queue.allocate_event<test_event>(false));
	}

	{
		sax::event_queue queue((sizeof(test_event) + 8) * 3);	// exactly 3 events

		test_event* ev = queue.allocate_event<test_event>(false);
		sax::event_queue::commit_event(ev);

		ev = queue.allocate_event<test_event>(false);
		sax::event_queue::commit_event(ev);

		ev = queue.allocate_event<test_event>(false);
		sax::event_queue::commit_event(ev);

		ASSERT_EQ(NULL, queue.allocate_event<test_event>(false));
	}

	{
		sax::event_queue queue((sizeof(test_event) + 8) * 3 + 8);	// little bigger than 3 events

		test_event* ev = queue.allocate_event<test_event>(false);
		sax::event_queue::commit_event(ev);

		test_event* first = ev;

		ev = queue.allocate_event<test_event>(false);
		sax::event_queue::commit_event(ev);

		test_event* second = ev;

		ev = queue.allocate_event<test_event>(false);
		sax::event_queue::commit_event(ev);

		ASSERT_EQ(NULL, queue.allocate_event<test_event>(false));	// too small to allocate

		ASSERT_EQ(first, queue.pop_event());
		queue.destroy_event(first);

		// will cause the "empty" state if allocate successfully
		ASSERT_EQ(NULL, queue.allocate_event<test_event>(false));

		ASSERT_EQ(second, queue.pop_event());
		queue.destroy_event(second);

		ASSERT_EQ(first, queue.allocate_event<test_event>(false));
	}
}

TEST(event_queue, empty_pop)
{
	sax::event_queue queue((sizeof(test_event) + 8) * 3);

	ASSERT_EQ(NULL, queue.pop_event());

	test_event* ev = queue.allocate_event<test_event>(false);
	sax::event_queue::commit_event(ev);
	ASSERT_EQ(ev, queue.pop_event());
	queue.destroy_event(ev);

	ASSERT_EQ(NULL, queue.pop_event());

	ev = queue.allocate_event<test_event>(false);
	sax::event_queue::commit_event(ev);
	ASSERT_EQ(ev, queue.pop_event());
	queue.destroy_event(ev);

	ASSERT_EQ(NULL, queue.pop_event());

	ev = queue.allocate_event<test_event>(false);
	sax::event_queue::commit_event(ev);
	ASSERT_EQ(ev, queue.pop_event());
	queue.destroy_event(ev);

	ASSERT_EQ(NULL, queue.pop_event());
}

TEST(event_queue, multi_allocate)
{
	sax::event_queue queue((sizeof(test_event) + 8) * 3);

	test_event* ev1 = queue.allocate_event<test_event>(false);
	test_event* ev2 = queue.allocate_event<test_event>(false);
	test_event* ev3 = queue.allocate_event<test_event>(false);

	ASSERT_EQ(NULL, queue.pop_event());

	sax::event_queue::commit_event(ev2);

	ASSERT_EQ(NULL, queue.pop_event());

	sax::event_queue::commit_event(ev1);

	ASSERT_EQ(ev1, queue.pop_event());
	queue.destroy_event(ev1);

	ASSERT_EQ(ev2, queue.pop_event());
	queue.destroy_event(ev2);

	ASSERT_EQ(NULL, queue.pop_event());

	sax::event_queue::commit_event(ev3);

	ASSERT_EQ(ev3, queue.pop_event());
	queue.destroy_event(ev3);
}

void* wait_and_pop(void* param)
{
	sax::event_queue* queue = (sax::event_queue*) ((void**)param)[0];
	double sec = *(double*) ((void**)param)[1];
	sax::event_type** value = (sax::event_type**) ((void**)param)[2];

	g_thread_sleep(sec);
	*value = queue->pop_event();
	queue->destroy_event(*value);

	return 0;
}

TEST(event_queue, wait)
{
	sax::event_queue queue((sizeof(test_event) + 8) * 3);
	double sec = 0.1;
	test_event* value;

	int64_t start = g_now_ms();

	test_event* ev = queue.allocate_event<test_event>(false);
	sax::event_queue::commit_event(ev);

	ASSERT_EQ(ev, queue.pop_event());
	queue.destroy_event(ev);

	test_event* first = ev;

	ev = queue.allocate_event<test_event>(false);
	sax::event_queue::commit_event(ev);

	test_event* second = ev;

	ev = queue.allocate_event<test_event>(false);
	sax::event_queue::commit_event(ev);

	void* param[] = {&queue, &sec, &value};
	g_thread_t tid = g_thread_start(wait_and_pop, param);

	ASSERT_EQ(first, queue.allocate_event<test_event>(true));

	ASSERT_EQ(second, value);

	int64_t end = g_now_ms();

	ASSERT_TRUE((end-start) < 120 && (end-start) >= 100);

	g_thread_join(tid, NULL);
}

int main(int argc, char* argv[])
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
