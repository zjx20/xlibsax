/*
 * t_slab.cpp
 *
 *  Created on: 2012-7-13
 *      Author: x
 */

#include "gtest/gtest.h"
#include "sax/slab.h"

using namespace sax;

#define SEED 0

TEST(slab_mgr, slab_list)
{
	slab_mgr* mgr = slab_mgr::get_instance();

	ASSERT_EQ(0, mgr->get_slabs_size());

	{
		slab_t slab1(4);

		ASSERT_EQ(1, mgr->get_slabs_size());

		{
			slab_t slab2(8);

			ASSERT_EQ(2, mgr->get_slabs_size());

			{
				slab_t slab3(16);

				ASSERT_EQ(3, mgr->get_slabs_size());
			}

			ASSERT_EQ(2, mgr->get_slabs_size());

			slab_t slab4(32);

			ASSERT_EQ(3, mgr->get_slabs_size());
		}

		ASSERT_EQ(1, mgr->get_slabs_size());
	}

	ASSERT_EQ(0, mgr->get_slabs_size());
}

TEST(slab_mgr, shrink_slabs)
{
	{
		slab_t slab1(4);

		slab1.free(slab1.alloc());

		{
			slab_t slab2(8);

			{
				slab_t slab3(16);

				slab_mgr::get_instance()->shrink_slabs(0.9);

				ASSERT_EQ(1, slab1.get_shrink_amount());
			}
		}
	}
}

TEST(slab, basic_test)
{
	{
		slab_t slab(4);

		ASSERT_EQ(sizeof(void*), slab.get_alloc_size());

		void* ptr = slab.alloc();

		ASSERT_EQ((size_t) 0, slab.get_list_length());
		ASSERT_EQ((size_t) 0, slab.get_shrink_amount());

		slab.free(ptr);

		ASSERT_EQ((size_t) 1, slab.get_list_length());
		ASSERT_EQ((size_t) 0, slab.get_shrink_amount());

		ptr = slab.alloc();

		ASSERT_EQ((size_t) 0, slab.get_list_length());
		ASSERT_EQ((size_t) 0, slab.get_shrink_amount());

		void* ptr2 = slab.alloc();

		ASSERT_EQ((size_t) 0, slab.get_list_length());
		ASSERT_EQ((size_t) 0, slab.get_shrink_amount());

		slab.free(ptr2);

		slab.free(ptr);
	}

	{
		slab_t slab(32);

		ASSERT_EQ(32, slab.get_alloc_size());
	}
}

TEST(slab, shrink)
{
	slab_t slab(128);
	void* ptr = slab.alloc();

	ASSERT_EQ(0, slab.get_shrink_amount());

	slab.shrink(0.9);

	ASSERT_EQ(0, slab.get_shrink_amount());

	slab.free(ptr);
	slab.shrink(0.9);

	ASSERT_EQ(1, slab.get_shrink_amount());

	ptr = slab.alloc();
	slab.free(ptr);

	ASSERT_EQ(0, slab.get_list_length());
	ASSERT_EQ(0, slab.get_shrink_amount());

	{
		void* ptrs[10];
		for (size_t i = 0; i < sizeof(ptrs) / sizeof(ptrs[0]); i++) {
			ptrs[i] = slab.alloc();
		}

		for (size_t i = 0; i < sizeof(ptrs) / sizeof(ptrs[0]); i++) {
			slab.free(ptrs[i]);
		}

		slab.shrink(0.6);

		ASSERT_EQ(4, slab.get_shrink_amount());

		for (size_t i = 0; i < sizeof(ptrs) / sizeof(ptrs[0]); i++) {
			ptrs[i] = slab.alloc();
		}

		for (size_t i = 0; i < sizeof(ptrs) / sizeof(ptrs[0]); i++) {
			slab.free(ptrs[i]);
		}

		ASSERT_EQ(6, slab.get_list_length());
		ASSERT_EQ(0, slab.get_shrink_amount());
	}
}

TEST(slab, SLAB_NEW)
{
	int* int1 = SLAB_NEW(int);
	SLAB_DELETE(int, int1);
	ASSERT_EQ(1, slab_holder<sizeof(int)>::get_slab().get_list_length());
}

#include <list>

#define TEST_LIST_APPEND_COUNT 5000000

struct big_struct
{
	char a[256];
};

TEST(slab_stl_allocator, list_std_allocator)
{
	std::list<big_struct> l;
	big_struct a;
	for (int i = 0; i < TEST_LIST_APPEND_COUNT; i++) {
		l.push_back(a);
	}
}

TEST(slab_stl_allocator, list1)
{
	std::list< big_struct, sax::slab_stl_allocator<big_struct> > l;
	big_struct a;
	for (int i = 0; i < TEST_LIST_APPEND_COUNT; i++) {
		l.push_back(a);
	}
}

TEST(slab_stl_allocator, list2)
{
	std::list< big_struct, sax::slab_stl_allocator<big_struct> > l;
	big_struct a;
	for (int i = 0; i < TEST_LIST_APPEND_COUNT; i++) {
		l.push_back(a);
	}
}

#include <map>

TEST(slab_stl_allocator, map)
{
	{
		std::map<int, std::list<int>, std::less<int>,
		sax::slab_stl_allocator< std::pair<const int, std::list<int> > > > m;

		m[0].push_back(1);
	}
}

TEST(benchmark, slab_int)
{
	slab_t slab(sizeof(int));
	void** ptrs = (void**)malloc(sizeof(void*) * 1000000);
	for (int j=0;j<1000000;j++) {
		for (int i=0;i<10;i++) {
			ptrs[i] = slab.alloc();
		}
		for (int i=0;i<10;i++) {
			slab.free(ptrs[i]);
		}
	}
	free(ptrs);
}

TEST(benchmark, slab_new_int)
{
	int** ptrs = (int**)malloc(sizeof(int*) * 1000000);
	for (int j=0;j<1000000;j++) {
		for (int i=0;i<10;i++) {
			ptrs[i] = SLAB_NEW(int);
		}
		for (int i=0;i<10;i++) {
			SLAB_DELETE(int, ptrs[i]);
		}
	}
	free(ptrs);
}

TEST(benchmark, new_int)
{
	int** ptrs = (int**)malloc(sizeof(int*) * 1000000);
	for (int j=0;j<1000000;j++) {
		for (int i=0;i<10;i++) {
			ptrs[i] = new int;
		}
		for (int i=0;i<10;i++) {
			delete ptrs[i];
		}
	}
	free(ptrs);
}

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    srand((unsigned)time(NULL));

    return RUN_ALL_TESTS();
}
