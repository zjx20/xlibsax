/*
 * t_buffer.cpp
 *
 *  Created on: 2011-8-10
 *      Author: x_zhou
 */

#include "gtest/gtest.h"
#include "sax/net/buffer.h"

#include <string>

using namespace std;
using namespace sax;

TEST(concurrent_queue, push_pop_size)
{
	concurrent_queue<int> q;
	EXPECT_TRUE(q.size() == 0);
	q.push(10);
	EXPECT_TRUE(q.size() == 1);
	q.push(11);
	EXPECT_TRUE(q.size() == 2);
	q.push(12);
	EXPECT_TRUE(q.size() == 3);
	q.push(13);
	EXPECT_TRUE(q.size() == 4);

	int a;
	EXPECT_TRUE(q.pop(a));
	EXPECT_EQ(a, 10);
	EXPECT_TRUE(q.size() == 3);
	EXPECT_TRUE(q.pop(a));
	EXPECT_EQ(a, 11);
	EXPECT_TRUE(q.size() == 2);
	EXPECT_TRUE(q.pop(a));
	EXPECT_EQ(a, 12);
	EXPECT_TRUE(q.size() == 1);
	EXPECT_TRUE(q.pop(a));
	EXPECT_EQ(a, 13);
	EXPECT_TRUE(q.size() == 0);
}

TEST(memory_pool, size_level)
{
	memory_pool mem_pool;
	size_t size = 0;
	for(int i=2; i<16; i++) {
		size = (size_t)1 << i;
		size--;
		void* p1 = mem_pool.allocate(size);
		ASSERT_TRUE(p1 != NULL);
		EXPECT_EQ(size, size_t(1) << i);
		mem_pool.recycle(size, p1);
	}
}

TEST(memory_pool, recycle)
{
	memory_pool mem_pool;
	size_t size = 0;
	void* p1 = mem_pool.allocate(size);
	ASSERT_TRUE(p1 != NULL);
	mem_pool.recycle(size, p1);
	void* p2 = mem_pool.allocate(size);
	ASSERT_TRUE(p2 != NULL);
	EXPECT_EQ((uint64_t)p1, (uint64_t)p2);
	mem_pool.recycle(size, p2);

	memory_pool mem_pool2(0);
	size = 20;
	p1 = mem_pool2.allocate(size);
	ASSERT_TRUE(p1 != NULL);
	EXPECT_FALSE(mem_pool2.recycle(size, p1));
}

TEST(memory_pool, memory_leak)
{
	printf("please check memory leak via valgrind.\n");

	const size_t N = 1000;

	memory_pool* mem_pool = new memory_pool();
	void* ptrs[N] = {0};
	size_t size[N];
	for(size_t i=0;i<N;i++) {
		size[i] = rand() % 65535;
		ptrs[i] = mem_pool->allocate(size[i]);
		ASSERT_TRUE(ptrs[i] != NULL);
	}

	for(size_t i=0;i<N;i++) {
		mem_pool->recycle(size[i], ptrs[i]);
	}
	delete mem_pool;
}

TEST(libsax_net_buffer, put_get)
{
	memory_pool mem_pool;
	buffer buf(&mem_pool, 1);

	EXPECT_TRUE(buf.put((uint8_t)0xff));
	EXPECT_TRUE(buf.flip());
	uint8_t b;
	EXPECT_EQ(buf.get(b), true);
	EXPECT_EQ(b, 0xff);
}

TEST(libsax_net_buffer_x, single_put_and_get_memory)
{
	memory_pool mem_pool;
	buffer buf(&mem_pool, 1);

	uint8_t b;

	EXPECT_TRUE(buf.put((uint8_t)0xff));

	EXPECT_TRUE(buf.flip());

	EXPECT_TRUE(buf.get(b));
	EXPECT_EQ(b, 0xff);

	EXPECT_TRUE(buf.compact());
}

TEST(libsax_net_buffer_x, complex_put_and_get_memory)
{
	memory_pool mem_pool;
	buffer buf(&mem_pool, 1);

	uint8_t b;

	for(int i=0;i<15;i++) {
		EXPECT_TRUE(buf.put((uint8_t)i));
	}

	EXPECT_TRUE(buf.flip());

	EXPECT_TRUE(buf.get(b));
	EXPECT_EQ(b, 0);

	EXPECT_EQ(buf.remaining(), (size_t)14);

	EXPECT_TRUE(buf.compact());

	uint8_t i_buf[20];
	for(int i=15;i<30;i++) {
		i_buf[i-15] = i;
	}
	EXPECT_TRUE(buf.put(i_buf, 15));
	EXPECT_TRUE(buf.flip());

	EXPECT_TRUE(buf.get(b));
	EXPECT_EQ(b, 1);
	EXPECT_EQ(buf.remaining(), (size_t)28);

	uint8_t o_buf[30];
	EXPECT_TRUE(buf.get(o_buf, 28));
	EXPECT_FALSE(buf.get(b));
	EXPECT_TRUE(buf.compact());

	for(int i=2;i<30;i++) {
		EXPECT_EQ(o_buf[i-2], (uint8_t)i);
	}

}

TEST(libsax_net_buffer_x, put_get_ints)
{
	memory_pool mem_pool;
	buffer buf(&mem_pool, 1);

	uint8_t i8 = 123;
	uint16_t i16 = 45678;
	uint32_t i32 = 789212456;
	uint64_t i64 = 78121324568786123ULL;

	EXPECT_TRUE(buf.put(i8));
	EXPECT_TRUE(buf.put(i16));
	EXPECT_TRUE(buf.put(i32));
	EXPECT_TRUE(buf.put(i64));

	EXPECT_TRUE(buf.flip());

	uint8_t o8;
	uint16_t o16;
	uint32_t o32;
	uint64_t o64;

	EXPECT_TRUE(buf.get(o8));
	EXPECT_TRUE(buf.get(o16));
	EXPECT_TRUE(buf.get(o32));
	EXPECT_TRUE(buf.get(o64));

	EXPECT_EQ(i8, o8);
	EXPECT_EQ(i16, o16);
	EXPECT_EQ(i32, o32);
	EXPECT_EQ(i64, o64);
}

TEST(libsax_net_buffer_x, put_buf1)
{
	memory_pool mem_pool;
	buffer buf1(&mem_pool, 1);
	buffer buf2(&mem_pool, 1);

	string ss("123456789");

	// test copy_data == true
	EXPECT_TRUE(buf2.put((uint8_t*)ss.c_str(), ss.size()));
	EXPECT_TRUE(buf2.flip());

	EXPECT_TRUE(buf1.put(buf2));
	EXPECT_TRUE(buf1.flip());

	char tmp[100] = {0};

	EXPECT_TRUE(buf1.get((uint8_t*)tmp, ss.size()));
	EXPECT_STREQ(tmp, ss.c_str());

	EXPECT_FALSE(buf2.get((uint8_t*)tmp, ss.size()));	// empty

}

TEST(libsax_net_buffer_x, put_buf2)
{
	memory_pool mem_pool;
	buffer buf1(&mem_pool, 1);
	buffer buf2(&mem_pool, 1);

	string ss("123456789");
	string ss1("9834567");

	// test copy_data == true
	EXPECT_TRUE(buf2.put((uint8_t*)ss.c_str(), ss.size()));
	EXPECT_TRUE(buf2.flip());

	sax::buffer::pos p = buf2.position();
	p += 2;
	EXPECT_TRUE(buf2.position(p));

	p = buf1.position();
	EXPECT_TRUE(buf1.put((uint8_t*)"98765", 5));
	p += 2;
	EXPECT_TRUE(buf1.position(p));
	EXPECT_TRUE(buf1.put(buf2, 5));
	EXPECT_TRUE(buf1.flip());

	char tmp[100] = {0};

	EXPECT_TRUE(buf1.get((uint8_t*)tmp, ss1.size()));
	EXPECT_STREQ(tmp, ss1.c_str());

	memset(tmp, 0, sizeof(tmp));
	EXPECT_TRUE(buf2.rewind());
	EXPECT_TRUE(buf2.get((uint8_t*)tmp, ss.size()));
	EXPECT_STREQ(tmp, ss.c_str());
}

TEST(normal_string1_l, test1)
{
	memory_pool mem_pool;
	buffer buf(&mem_pool, 1);

    string a = "livingroom";
    EXPECT_TRUE(buf.put((uint8_t *)a.c_str() , a.size()));
    EXPECT_TRUE(buf.flip());
    char tmp[100] = {0};
    EXPECT_TRUE(buf.get((uint8_t *)tmp , 10));
    EXPECT_STREQ(a.c_str() , tmp);
}

TEST(normal_string2_l, test1)
{
	memory_pool mem_pool;
	buffer buf(&mem_pool, 1);

    char* a = "livingroom";
    EXPECT_TRUE(buf.put((uint8_t *)a , 10));
    EXPECT_TRUE(buf.flip());
    char tmp[100] = {0};
    EXPECT_TRUE(buf.get((uint8_t *)tmp , 10));
    EXPECT_STREQ(a , tmp);
}

TEST(normal_string3_l, test1)
{
	memory_pool mem_pool;
	buffer buf(&mem_pool, 1);

    char a[100] = {0};
    strcpy(a,"livingroom");
    EXPECT_STREQ(a,"livingroom");
    EXPECT_TRUE(buf.put((uint8_t *)a , 10));
    EXPECT_TRUE(buf.flip());
    char tmp[100] = {0};
    EXPECT_TRUE(buf.get((uint8_t *)tmp , 10));
    EXPECT_STREQ(a , tmp);
}

TEST(error_l, test1)
{
	memory_pool mem_pool;
	buffer buf(&mem_pool, 1);

    char tmp[100] = {};
    EXPECT_FALSE(buf.get((uint8_t *)tmp , 10));
}

TEST(error_l, test2)
{
	memory_pool mem_pool;
	buffer buf(&mem_pool, 1);

    EXPECT_FALSE(buf.compact());
}

TEST(error_l, test3)
{
	memory_pool mem_pool;
	buffer buf(&mem_pool, 1);

    char ch;
    EXPECT_FALSE(buf.get((uint8_t &)ch));
}

TEST(error_l, test4)
{
	memory_pool mem_pool;
	buffer buf(&mem_pool, 1);

    EXPECT_FALSE(buf.reset());
}

TEST(muti_string1_l, test1)
{
	memory_pool mem_pool;
	buffer buf(&mem_pool, 1);

    string a = "livingroom";
    EXPECT_TRUE(buf.put((uint8_t *)a.c_str() , a.size()));
    EXPECT_TRUE(buf.put((uint8_t *)a.c_str() , a.size()));
    EXPECT_TRUE(buf.flip());
    char tmp[100] = {0};
    EXPECT_TRUE(buf.get((uint8_t *)tmp , 10));
    EXPECT_STREQ(a.c_str() , tmp);
    memset(tmp,0,sizeof(tmp));
    EXPECT_TRUE(buf.get((uint8_t *)tmp , 9));
    EXPECT_STREQ("livingroo" , tmp);
    EXPECT_EQ(buf.remaining(), (size_t)1);
    char ch = 0;
    EXPECT_TRUE(buf.get((uint8_t &)ch));
    EXPECT_EQ(ch , 'm');
}

TEST(muti_string2_l, test1)
{
	memory_pool mem_pool;
	buffer buf(&mem_pool, 1);

    string a = "livingroom";
    EXPECT_TRUE(buf.put((uint8_t *)a.c_str() , a.size()));
    EXPECT_TRUE(buf.put((uint8_t *)a.c_str() , a.size()));
    EXPECT_TRUE(buf.flip());
    char tmp[100] = {0};
    EXPECT_TRUE(buf.get((uint8_t *)tmp , 10));
    EXPECT_STREQ(a.c_str() , tmp);
    EXPECT_TRUE(buf.compact());

    string b = a;
    reverse(b.begin(),b.end());
    EXPECT_TRUE(buf.put((uint8_t *)b.c_str() , b.size()));
    EXPECT_TRUE(buf.flip());

    memset(tmp,0,sizeof(tmp));
    EXPECT_TRUE(buf.get((uint8_t *)tmp , 10));
    EXPECT_STREQ(a.c_str() , tmp);
    EXPECT_EQ(buf.remaining(), (size_t)10);
    memset(tmp,0,sizeof(tmp));
    EXPECT_TRUE(buf.get((uint8_t *)tmp , 10));
    EXPECT_STREQ(b.c_str() , tmp);
    EXPECT_EQ(buf.remaining(), (size_t)0);
}

TEST(recover_read1_l, test1)
{
	memory_pool mem_pool;
	buffer buf(&mem_pool, 1);

    string a = "livingroom";
    EXPECT_TRUE(buf.mark());
    EXPECT_TRUE(buf.put((uint8_t *)a.c_str() , a.size()));
    EXPECT_TRUE(buf.flip());
    char tmp[100] = {0};
    EXPECT_TRUE(buf.get((uint8_t *)tmp , 10));
    EXPECT_STREQ(a.c_str() , tmp);
    EXPECT_EQ(buf.remaining(), (size_t)0);
    EXPECT_TRUE(buf.reset());
    EXPECT_EQ(buf.remaining(), (size_t)10);
    memset(tmp,0,sizeof(tmp));
    EXPECT_TRUE(buf.get((uint8_t *)tmp , 10));
    EXPECT_STREQ(a.c_str() , tmp);
    EXPECT_EQ(buf.remaining(), (size_t)0);
    EXPECT_FALSE(buf.get((uint8_t *)tmp , 10));
}

TEST(recover_read2_l, test1)
{
	memory_pool mem_pool;
	buffer buf(&mem_pool, 1);

    string a = "livingroom";
    buffer::pos p = buf.position();
    EXPECT_TRUE(buf.put((uint8_t *)a.c_str() , a.size()));
    EXPECT_TRUE(buf.flip());
    char tmp[100] = {0};
    EXPECT_TRUE(buf.get((uint8_t *)tmp , 10));
    EXPECT_STREQ(a.c_str() , tmp);
    EXPECT_EQ(buf.remaining(), (size_t)0);
    EXPECT_TRUE(buf.position(p));
    EXPECT_EQ(buf.remaining(), (size_t)10);
    memset(tmp,0,sizeof(tmp));
    EXPECT_TRUE(buf.get((uint8_t *)tmp , 10));
    EXPECT_STREQ(a.c_str() , tmp);
    EXPECT_EQ(buf.remaining(), (size_t)0);
    EXPECT_FALSE(buf.get((uint8_t *)tmp , 10));
}

TEST(recover_write1_l, test1)
{
	memory_pool mem_pool;
	buffer buf(&mem_pool, 1);

    string a = "livingroom";
    EXPECT_TRUE(buf.mark());
    EXPECT_TRUE(buf.put((uint8_t *)a.c_str() , a.size()));
    EXPECT_TRUE(buf.reset());
    reverse(a.begin() , a.end());
    EXPECT_STREQ(a.c_str(),"moorgnivil");
    EXPECT_TRUE(buf.put((uint8_t *)a.c_str() , a.size()));
    EXPECT_TRUE(buf.flip());
    char tmp[100] = {0};
    EXPECT_TRUE(buf.get((uint8_t *)tmp , 10));
    EXPECT_STREQ(a.c_str() , tmp);
    EXPECT_EQ(buf.remaining(), (size_t)0);
    EXPECT_TRUE(buf.reset());
    EXPECT_EQ(buf.remaining(), (size_t)10);
    memset(tmp,0,sizeof(tmp));
    EXPECT_TRUE(buf.get((uint8_t *)tmp , 10));
    EXPECT_STREQ(a.c_str() , tmp);
    EXPECT_EQ(buf.remaining(), (size_t)0);
    EXPECT_FALSE(buf.get((uint8_t *)tmp , 10));
}

TEST(over_limit1_l, test1)
{
	memory_pool mem_pool;
	buffer buf(&mem_pool, 1);

    string a = "livingroom";
    buffer::pos p = buf.position();
    EXPECT_TRUE(buf.limit(p));
    EXPECT_FALSE(buf.put((uint8_t *)a.c_str() , a.size()));
    EXPECT_TRUE(buf.flip());
    char tmp[100] = {0};
    EXPECT_FALSE(buf.get((uint8_t *)tmp , 10));
    EXPECT_EQ(buf.remaining(), (size_t)0);
    EXPECT_TRUE(buf.compact());
    memset(tmp,0,sizeof(tmp));
    EXPECT_TRUE(buf.put((uint8_t *)a.c_str() , a.size()));
    EXPECT_TRUE(buf.flip());
    EXPECT_TRUE(buf.get((uint8_t *)tmp , 10));
    EXPECT_STREQ(a.c_str() , tmp);
}

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    srand((unsigned)time(NULL));


    return RUN_ALL_TESTS();
}
