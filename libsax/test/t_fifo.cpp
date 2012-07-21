/*
 * t_slab.cpp
 *
 *  Created on: 2012-7-13
 *      Author: x
 */

#include "gtest/gtest.h"
#include "sax/c++/fifo.h"
#include "sax/slab.h"

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

TEST(fifo, slab_stl_allocator)
{
	{
		sax::fifo<std::string, sax::slab_stl_allocator<std::string> > p;
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

int main(int argc, char *argv[])
{
	testing::InitGoogleTest(&argc, argv);
	srand((unsigned)time(NULL));

	return RUN_ALL_TESTS();
}
