/*
 * t_linkedlist.cpp
 *
 *  Created on: 2013-10-5
 *      Author: X
 */

#include "gtest/gtest.h"
#include "sax/c++/linkedlist.h"

struct TestStruct
{
	TestStruct* _next;
	TestStruct* _prev;

	int data;
};

TEST(linkedlist, general)
{
	sax::linkedlist<TestStruct> list;

	EXPECT_EQ(true, list.empty());

	TestStruct* tmp;

	tmp = new TestStruct();
	tmp->data = 1;
	list.push_back(tmp);

	EXPECT_EQ(false, list.empty());

	TestStruct* tmp2 = list.head();

	EXPECT_EQ(tmp, tmp2);
	EXPECT_EQ(1, tmp2->data);

	list.erase(tmp2);

	EXPECT_EQ(true, list.empty());
	EXPECT_EQ(NULL, list.head());
	EXPECT_EQ(NULL, list.tail());

	delete tmp2;
}

TEST(linkedlist, push)
{
	sax::linkedlist<TestStruct> list;

	TestStruct* tmp;
	TestStruct* tmp2;

	tmp = new TestStruct();
	tmp->data = 3;
	list.push_back(tmp);

	tmp = new TestStruct();
	tmp->data = 4;
	list.push_back(tmp);

	tmp = new TestStruct();
	tmp->data = 5;
	list.push_back(tmp);

	tmp2 = list.head();
	EXPECT_EQ(3, tmp2->data);

	tmp2 = list.tail();
	EXPECT_EQ(5, tmp2->data);

	tmp = new TestStruct();
	tmp->data = 6;
	list.push_front(tmp);

	tmp2 = list.head();
	EXPECT_EQ(6, tmp2->data);

	int expect[] = {6, 3, 4, 5};
	int count = 0;
	while(!list.empty()) {
		tmp2 = list.head();
		EXPECT_EQ(expect[count++], tmp2->data);

		list.erase(tmp2);
		delete tmp2;
	}
}

TEST(linkedlist, erase)
{
	sax::linkedlist<TestStruct> list;

	TestStruct* tmp;
	TestStruct* tmp2;
	TestStruct* mid;

	mid = tmp = new TestStruct();
	tmp->data = 100;
	list.push_back(tmp);

	tmp = new TestStruct();
	tmp->data = 101;
	list.push_back(tmp);

	tmp = new TestStruct();
	tmp->data = 102;
	list.push_front(tmp);

	list.erase(mid);
	delete mid;

	tmp2 = list.tail();
	list.erase(tmp2);
	delete tmp2;

	EXPECT_EQ(102, list.tail()->data);

	tmp2 = list.tail();
	list.erase(tmp2);
	delete tmp2;

	EXPECT_TRUE(list.empty());

	EXPECT_EQ(NULL, list.head());
	EXPECT_EQ(NULL, list.tail());
}

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    srand((unsigned)time(NULL));

    return RUN_ALL_TESTS();
}
