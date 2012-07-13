/*
 * t_slab.cpp
 *
 *  Created on: 2012-7-13
 *      Author: x
 */

#include "gtest/gtest.h"
#include "sax/slab.h"

using namespace sax;

TEST(slab, basic_test)
{
	{
		slab_t slab(4);

		ASSERT_EQ(sizeof(void*) * 2, slab.get_alloc_size());

		void* ptr = slab.alloc();

		ASSERT_EQ((size_t) 1, slab.get_total_amount());
		ASSERT_EQ((size_t) 0, slab.get_list_length());
		ASSERT_EQ((size_t) 0, slab.get_shrink_amount());
		ASSERT_EQ((size_t) 1, slab.get_malloc_times());

		slab.free(ptr);

		ASSERT_EQ((size_t) 1, slab.get_total_amount());
		ASSERT_EQ((size_t) 1, slab.get_list_length());
		ASSERT_EQ((size_t) 0, slab.get_shrink_amount());
		ASSERT_EQ((size_t) 1, slab.get_malloc_times());

		ptr = slab.alloc();

		ASSERT_EQ((size_t) 1, slab.get_total_amount());
		ASSERT_EQ((size_t) 0, slab.get_list_length());
		ASSERT_EQ((size_t) 0, slab.get_shrink_amount());
		ASSERT_EQ((size_t) 1, slab.get_malloc_times());

		void* ptr2 = slab.alloc();

		ASSERT_EQ((size_t) 2, slab.get_total_amount());
		ASSERT_EQ((size_t) 0, slab.get_list_length());
		ASSERT_EQ((size_t) 0, slab.get_shrink_amount());
		ASSERT_EQ((size_t) 2, slab.get_malloc_times());
	}

	{
		slab_t slab(32);

		ASSERT_EQ(32, slab.get_alloc_size());
	}
}

TEST(slab, shrink)
{

}

TEST(slab, SLAB_NEW)
{

}

TEST(slab_mgr, slab_list)
{
	{
		slab_t slab(4);

		{
			slab_t slab(8);

			{
				slab_t slab(16);
			}
		}
	}
}

TEST(slab_mgr, shrink_slabs)
{
	{
		slab_t slab(4);

		{
			slab_t slab(8);

			{
				slab_t slab(16);
			}
		}
	}
}

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    srand((unsigned)time(NULL));

    return RUN_ALL_TESTS();
}
