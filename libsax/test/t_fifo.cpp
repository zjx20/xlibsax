/*
 * t_slab.cpp
 *
 *  Created on: 2012-7-13
 *      Author: x
 */

#include "gtest/gtest.h"
#include "sax/c++/fifo.h"

using namespace sax;

TEST(fifo, basic_test)
{
	{
		fifo<int> p;
		ASSERT_TRUE(p.push_back(1));

		int a;
		ASSERT_TRUE(p.pop_front(a));
		ASSERT_EQ(1, a);

		ASSERT_TRUE(p.push_back(2));
		ASSERT_TRUE(p.push_back(3));
		ASSERT_TRUE(p.push_back(4));

		ASSERT_TRUE(p.pop_front(a));
		ASSERT_EQ(2, a);

		ASSERT_TRUE(p.push_back(5));
		ASSERT_TRUE(p.push_back(6));

		ASSERT_TRUE(p.pop_front(a));
		ASSERT_EQ(3, a);

		ASSERT_TRUE(p.pop_front(a));
		ASSERT_EQ(4, a);

		ASSERT_TRUE(p.pop_front(a));
		ASSERT_EQ(5, a);

		ASSERT_TRUE(p.pop_front(a));
		ASSERT_EQ(6, a);

		ASSERT_FALSE(p.pop_front(a));
	}

	{
		fifo<std::string> p;
		ASSERT_TRUE(p.push_back("a"));

		std::string a;
		ASSERT_TRUE(p.pop_front(a));
		ASSERT_EQ("a", a);

		ASSERT_TRUE(p.push_back("b"));
		ASSERT_TRUE(p.push_back("c"));
		ASSERT_TRUE(p.push_back("d"));

		ASSERT_TRUE(p.pop_front(a));
		ASSERT_EQ("b", a);

		ASSERT_TRUE(p.push_back("e"));
		ASSERT_TRUE(p.push_back("f"));

		ASSERT_TRUE(p.pop_front(a));
		ASSERT_EQ("c", a);

		ASSERT_TRUE(p.pop_front(a));
		ASSERT_EQ("d", a);

		ASSERT_TRUE(p.pop_front(a));
		ASSERT_EQ("e", a);

		ASSERT_TRUE(p.pop_front(a));
		ASSERT_EQ("f", a);

		ASSERT_FALSE(p.pop_front(a));
	}
}

void* wait_and_push(void* param)
{
	fifo<int>* p = (fifo<int>*)((void**)param)[0];
	double sec = *(double*)((void**)param)[1];
	int push_value = *(int*)((void**)param)[2];

	g_thread_sleep(sec);
	p->push_back(push_value);

	return 0;
}

TEST(fifo, pop_front_wait1)
{
	printf("pop_front_wait1 will take 3 seconds to finish.\n");
	fifo<int> p;
	int a;
	ASSERT_FALSE(p.pop_front(a, 3));
}

TEST(fifo, pop_front_wait2)
{
	printf("pop_front_wait2 will take 1 seconds to finish.\n");
	fifo<int> p;
	double sec = 1;
	int push_value = 100;

	void* param[] = {&p, &sec, &push_value};
	g_thread_t tid = g_thread_start(wait_and_push, param);

	int a;
	ASSERT_TRUE(p.pop_front(a, 3));
	ASSERT_EQ(push_value, a);

	g_thread_join(tid, NULL);
}

TEST(fifo, pop_front_wait3)
{
	printf("pop_front_wait3 will take 2 seconds to finish.\n");
	fifo<int> p;
	double sec = 2;
	int push_value = 101;

	void* param[] = {&p, &sec, &push_value};
	g_thread_t tid = g_thread_start(wait_and_push, param);

	int a;
	ASSERT_TRUE(p.pop_front(a, -1));
	ASSERT_EQ(push_value, a);

	g_thread_join(tid, NULL);
}

void* wait_and_pop(void* param)
{
	fifo<int>* p = (fifo<int>*)((void**)param)[0];
	double sec = *(double*)((void**)param)[1];
	int* pop_value = (int*)((void**)param)[2];

	g_thread_sleep(sec);
	p->pop_front(*pop_value);

	return 0;
}

TEST(fifo, push_back_wait1)
{
	printf("push_back_wait1 will take 1 seconds to finish.\n");

	int cap = 10;
	fifo<int> p(cap);
	cap = (int)p.get_capacity();
	double sec = 1;
	int pop_value;

	void* param[] = {&p, &sec, &pop_value};
	g_thread_t tid = g_thread_start(wait_and_pop, param);

	for (int i=0; i<cap; i++) {
		p.push_back(i+123456);
	}

	p.push_back(99999, -1);	// wait 1 second

	ASSERT_EQ(123456, pop_value);

	int v;

	for (int i=0; i<cap-1; i++) {
		ASSERT_TRUE(p.pop_front(v));
		ASSERT_EQ(123456+i+1, v);
	}

	ASSERT_TRUE(p.pop_front(v));
	ASSERT_EQ(99999, v);

	g_thread_join(tid, NULL);
}

TEST(fifo, push_back_wait2)
{
	printf("push_back_wait2 will take 1 seconds to finish.\n");
	int cap = 10;
	fifo<std::string> p(cap);
	cap = (int)p.get_capacity();
	for (int i=0; i<cap; i++) {
		ASSERT_TRUE(p.push_back(std::string("hello")));
	}
	ASSERT_FALSE(p.push_back(std::string("wait"), 1.0));
}

TEST(fifo, push_back_wait3)
{
	printf("push_back_wait3 will take 1 seconds to finish.\n");
	int cap = 10;
	fifo<int> p(cap);
	cap = (int)p.get_capacity();
	double sec = 1;
	int pop_value;

	void* param[] = {&p, &sec, &pop_value};
	g_thread_t tid = g_thread_start(wait_and_pop, param);

	for (int i=0; i<cap; i++) {
		ASSERT_TRUE(p.push_back(i+654321));
	}

	ASSERT_TRUE(p.push_back(77777, 3.0));

	ASSERT_EQ(654321, pop_value);

	int v;
	for (int i=0; i<cap-1; i++) {
		ASSERT_TRUE(p.pop_front(v));
		ASSERT_EQ(654321+i+1, v);
	}

	ASSERT_TRUE(p.pop_front(v));
	ASSERT_EQ(77777, v);

	g_thread_join(tid, NULL);
}

int main(int argc, char *argv[])
{
	testing::InitGoogleTest(&argc, argv);
	srand((unsigned)time(NULL));

	return RUN_ALL_TESTS();
}
