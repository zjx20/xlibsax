/*
 * t_buffer.cpp
 *
 *  Created on: 2012-4-4
 *      Author: X
 */

#include "gtest/gtest.h"
#include "sax/net/buffer.h"

using namespace sax;

struct A {
	A* next;
	A* prev;
	int a;
};

const int32_t BLOCK_HEADER_SIZE = 16;

TEST(linked_list, push_pop)
{
	A arr[100];

	for(size_t i=0;i<ARRAY_SIZE(arr);i++) {
		arr[i].a = i;
	}

	_linked_list<A> l;

	EXPECT_EQ(NULL, l.get_head());
	EXPECT_EQ(NULL, l.pop_front());

	l.push_back(&arr[0]);

	EXPECT_EQ(&arr[0], l.get_head());
	EXPECT_EQ(&arr[0], l.pop_front());

	for(size_t i=0;i<ARRAY_SIZE(arr);i++) {
		l.push_back(&arr[i]);
	}

	for(size_t i=0;i<ARRAY_SIZE(arr);i++) {
		A* tmp = l.pop_front();
		EXPECT_EQ(i, tmp->a);
	}

	////////////////////////////////////////

	for(size_t i=0;i<ARRAY_SIZE(arr);i++) {
		l.push_back(&arr[i]);
	}

	for(size_t i=0;i<10;i++) {
		l.pop_front();
	}

	A* h = l.get_head();
	int cc = 0;
	while(h != NULL) {
		cc++;
		h = h->next;
	}
	EXPECT_EQ(cc, 90);
}

TEST(linked_list, roll)
{
	A arr[100];

	for(size_t i=0;i<ARRAY_SIZE(arr);i++) {
		arr[i].a = i;
	}

	{
		_linked_list<A> l;

		for(size_t i=0;i<ARRAY_SIZE(arr);i++) {
			l.push_back(&arr[i]);
		}

		l.roll(&arr[20]);

		for (size_t i=0;i<80;i++) {
			A* tmp = l.pop_front();
			ASSERT_EQ(tmp->a, (int)i+20);
		}

		for (size_t i=0;i<20;i++) {
			A* tmp = l.pop_front();
			ASSERT_EQ(tmp->a, (int)i);
		}

		ASSERT_EQ(NULL, l.pop_front());
	}

	{
		_linked_list<A> l;

		for(size_t i=0;i<ARRAY_SIZE(arr);i++) {
			l.push_back(&arr[i]);
		}

		l.roll(&arr[0]);	// do nothing

		for(size_t i=0;i<ARRAY_SIZE(arr);i++) {
			A* tmp = l.pop_front();
			ASSERT_EQ(tmp->a, (int)i);
		}

		ASSERT_EQ(NULL, l.pop_front());
	}

	{
		_linked_list<A> l;

		for(size_t i=0;i<ARRAY_SIZE(arr);i++) {
			l.push_back(&arr[i]);
		}

		l.roll(&arr[99]);	// do nothing

		ASSERT_EQ(&arr[99], l.pop_front());

		for(size_t i=0;i<ARRAY_SIZE(arr)-1;i++) {
			A* tmp = l.pop_front();
			ASSERT_EQ(tmp->a, (int)i);
		}

		ASSERT_EQ(NULL, l.pop_front());
	}
}

TEST(buffer, empty)
{
	g_xslab_t* slab = g_xslab_init(1 + BLOCK_HEADER_SIZE);

	{
		buffer buf(slab);
		buffer buf2(slab);

		uint8_t tmp[100];

		buf.mark();
		EXPECT_TRUE(buf.reset());

		EXPECT_TRUE(buf.flip());
		EXPECT_TRUE(buf.get(tmp, 0));
		EXPECT_TRUE(buf.get(buf2));

		EXPECT_TRUE(buf.compact());
		EXPECT_TRUE(buf2.flip());
		EXPECT_TRUE(buf.put(tmp, 0));
		EXPECT_TRUE(buf.put(buf2));
		EXPECT_TRUE(buf.flip());

		EXPECT_FALSE(buf.get(tmp, 1));

		EXPECT_EQ(1/*alloc one block in initialization*/, buf.capacity());
		EXPECT_EQ(0, buf.position());

		EXPECT_EQ(0, buf.remaining());
		EXPECT_EQ(0, buf.data_length());
		EXPECT_FALSE(buf.has_remaining());
		EXPECT_FALSE(buf.has_data());

		EXPECT_FALSE(buf.peek(tmp[0]));
	}

	ASSERT_EQ(2/*two buffer*/, g_xslab_usable_amount(slab));

	g_xslab_destroy(slab);
}

TEST(buffer, writing_mode)
{
	g_xslab_t* slab = g_xslab_init(1 + BLOCK_HEADER_SIZE);

	const char* str = "this is a test string.";

	{
		buffer buf(slab);

		EXPECT_TRUE(buf.put((uint8_t*)str, strlen(str)));
		EXPECT_EQ(strlen(str), buf.position());

		EXPECT_TRUE(buf.put((uint8_t*)str, strlen(str)));
		EXPECT_EQ(strlen(str) * 2, buf.position());

		EXPECT_EQ(strlen(str) * 2 + 1/*reserve() extra one*/, buf.capacity());
		EXPECT_FALSE(buf.compact());

		EXPECT_EQ(strlen(str) * 2, buf.data_length());
		EXPECT_TRUE(buf.has_data());

		//EXPECT_EQ(0, buf.remaining());
		//EXPECT_FALSE(buf.has_remaining());

		EXPECT_TRUE(buf.flip());

		EXPECT_EQ(0, buf.position());
	}

	g_xslab_destroy(slab);
}

TEST(buffer, reading_mode)
{
	g_xslab_t* slab = g_xslab_init(1 + BLOCK_HEADER_SIZE);

	const char* str = "this is a test string.";
	uint8_t tmp[100];

	{
		buffer buf(slab);

		EXPECT_TRUE(buf.put((uint8_t*)str, strlen(str)));
		EXPECT_EQ(strlen(str), buf.position());

		EXPECT_TRUE(buf.flip());
		EXPECT_EQ(0, buf.position());

		EXPECT_FALSE(buf.put(tmp, 1));
		EXPECT_FALSE(buf.get(tmp, 10000));

		EXPECT_TRUE(buf.get(tmp, 4));
		EXPECT_EQ(0, memcmp(tmp, str, 4));

		EXPECT_EQ(strlen(str) - 4, buf.remaining());
		EXPECT_TRUE(buf.has_remaining());
		//EXPECT_EQ(0, buf.data_length());
		//EXPECT_FALSE(buf.has_data());

		EXPECT_TRUE(buf.compact());
		EXPECT_FALSE(buf.compact());
		EXPECT_EQ(strlen(str) - 4, buf.position());

		EXPECT_TRUE(buf.put((uint8_t*)"have fun", 8));
		EXPECT_TRUE(buf.flip());

		EXPECT_EQ(0, buf.position());
		EXPECT_TRUE(buf.get(tmp, strlen(str) - 4 + 8));
		EXPECT_EQ(0, memcmp(tmp, " is a test string.have fun", strlen(str) - 4 + 8));
	}

	g_xslab_destroy(slab);
}

TEST(buffer, put_get_buffer)
{
	g_xslab_t* slab = g_xslab_init(2 + BLOCK_HEADER_SIZE);

	{
		buffer buf1(slab);
		buffer buf2(slab);

		const char* str1 = "a testing string";
		const char* str2 = "string2";

		uint8_t tmp[100];

		EXPECT_TRUE(buf2.put(&tmp[0], 1));
		EXPECT_TRUE(buf2.put((uint8_t*)str2, strlen(str2)));
		EXPECT_TRUE(buf2.flip());

		EXPECT_TRUE(buf2.get(tmp, 1));
		EXPECT_TRUE(buf2.compact());
		EXPECT_TRUE(buf2.flip());

		EXPECT_TRUE(buf1.put((uint8_t*)str1, strlen(str1)));
		EXPECT_TRUE(buf1.put(buf2, 3));

		EXPECT_EQ(strlen(str1) + 3, buf1.position());
		EXPECT_EQ(3, buf2.position());

		EXPECT_TRUE(buf1.flip());

		buf1.mark();
		EXPECT_TRUE(buf1.get(tmp, strlen(str1) + 3));

		EXPECT_EQ(std::string("a testing stringstr"), std::string((char*)tmp, strlen(str1) + 3));

		EXPECT_TRUE(buf1.reset());
		EXPECT_EQ(0, buf1.position());

		EXPECT_TRUE(buf2.compact());
		EXPECT_EQ(strlen(str2) - 3, buf2.position());
		EXPECT_TRUE(buf1.get(buf2));
		EXPECT_EQ(strlen(str1) + strlen(str2), buf2.position());
		EXPECT_TRUE(buf2.flip());
		EXPECT_TRUE(buf2.get(tmp, strlen(str1) + strlen(str2)));

		EXPECT_EQ(std::string("ing2a testing stringstr"), std::string((char*)tmp, strlen(str1) + strlen(str2)));

		EXPECT_TRUE(buf1.compact());
		EXPECT_TRUE(buf2.compact());

		// get with a reading buffer
		EXPECT_TRUE(buf1.put((uint8_t*)str1, strlen(str1)));
		EXPECT_TRUE(buf1.flip());
		EXPECT_TRUE(buf2.put((uint8_t*)str2, strlen(str2)));
		EXPECT_TRUE(buf2.flip());
		EXPECT_FALSE(buf2.get(buf1));

		// put with a writing buffer
		EXPECT_TRUE(buf1.compact());
		EXPECT_TRUE(buf2.compact());
		EXPECT_FALSE(buf2.put(buf1));

		// reading buffer put a buffer
		EXPECT_TRUE(buf1.flip());
		EXPECT_TRUE(buf2.flip());
		EXPECT_FALSE(buf1.put(buf2));

		// writing buffer get a buffer
		EXPECT_TRUE(buf1.compact());
		EXPECT_TRUE(buf2.compact());
		EXPECT_FALSE(buf1.get(buf2));
	}

	g_xslab_destroy(slab);
}

TEST(buffer, seeking)	// preappend rewind
{
	g_xslab_t* slab = g_xslab_init(1 + BLOCK_HEADER_SIZE);

	const char* str = "this is a test string";
	uint8_t tmp[100];

	// mark reset
	{
		buffer buf(slab);

		buf.mark();
		EXPECT_TRUE(buf.put((uint8_t*)str, strlen(str)));
		EXPECT_EQ(strlen(str), buf.position());
		EXPECT_TRUE(buf.reset());
		EXPECT_EQ(0, buf.position());
		EXPECT_TRUE(buf.put((uint8_t*)"that", 4));
		EXPECT_TRUE(buf.reset());
		EXPECT_TRUE(buf.flip());
		EXPECT_FALSE(buf.reset());

		EXPECT_TRUE(buf.get(tmp, strlen(str)));
		EXPECT_EQ(0, memcmp(tmp, "that is a test string", strlen(str)));

		EXPECT_TRUE(buf.compact());
		EXPECT_TRUE(buf.put((uint8_t*)"hello ", 6));

		buf.mark();
		EXPECT_TRUE(buf.put((uint8_t*)"x", 1));
		EXPECT_TRUE(buf.reset());
		EXPECT_TRUE(buf.put((uint8_t*)"world", 5));
		EXPECT_TRUE(buf.flip());

		EXPECT_TRUE(buf.get(tmp, buf.remaining()));
		EXPECT_EQ(0, memcmp(tmp, "hello world", 11));
	}

	{
		// rewind when writing
		buffer buf(slab);

		uint8_t tmp[100];

		const char* str = "a buffer for high throughput io";

		EXPECT_TRUE(buf.put((uint8_t*)str, strlen(str)));
		buf.rewind();
		EXPECT_TRUE(buf.put((uint8_t*)"A", 1));
		EXPECT_TRUE(buf.reset());
		EXPECT_TRUE(buf.flip());
		EXPECT_TRUE(buf.get(tmp, buf.remaining()));
		EXPECT_EQ(0, memcmp("A buffer for high throughput io", tmp, strlen(str)));

		// rewind when reading
		buf.rewind();
		EXPECT_EQ(0, buf.position());
		EXPECT_TRUE(buf.get(tmp, 1));
		EXPECT_EQ(tmp[0], (uint8_t)'A');
		EXPECT_TRUE(buf.reset());
		EXPECT_EQ(strlen(str), buf.position());
	}

	{
		buffer buf(slab);

		uint8_t tmp[100];

		EXPECT_TRUE(buf.put((uint8_t*)"what ", 5));
		EXPECT_TRUE(buf.preappend(2));
		EXPECT_EQ(7, buf.position());
		EXPECT_TRUE(buf.put((uint8_t*)"wonderful day", 13));
		EXPECT_TRUE(buf.reset());
		EXPECT_TRUE(buf.put((uint8_t*)"a ", 2));
		EXPECT_TRUE(buf.reset());
		EXPECT_TRUE(buf.flip());

		EXPECT_TRUE(buf.get(tmp, buf.remaining()));
		EXPECT_EQ(0, memcmp(tmp, "what a wonderful day", 20));
	}

	g_xslab_destroy(slab);
}

TEST(buffer, clear)
{
	g_xslab_t* slab = g_xslab_init(1 + BLOCK_HEADER_SIZE);

	{
		buffer buf(slab);

		buf.put((uint8_t*)"testing", 7);
		buf.flip();

		buf.clear();
		EXPECT_EQ(8/*7 bytes + reserve() extra one*/, g_xslab_usable_amount(slab));
		EXPECT_EQ(0, buf.position());
		EXPECT_EQ(0, buf.capacity());
	}

	g_xslab_destroy(slab);
}

TEST(buffer, endianness)
{
	g_xslab_t* slab = g_xslab_init(1 + BLOCK_HEADER_SIZE);

	{
		buffer buf(slab);

		uint32_t a = 0x12345678;

		EXPECT_TRUE(buf.put(a, true));
		EXPECT_TRUE(buf.flip());

		uint8_t tmp[100];

		EXPECT_TRUE(buf.get(tmp, 4));
		EXPECT_EQ(tmp[0], (uint8_t)0x012);
		EXPECT_EQ(tmp[1], (uint8_t)0x034);
		EXPECT_EQ(tmp[2], (uint8_t)0x056);
		EXPECT_EQ(tmp[3], (uint8_t)0x078);
		EXPECT_TRUE(buf.compact());

		EXPECT_TRUE(buf.put(a, false));
		EXPECT_TRUE(buf.flip());

		EXPECT_TRUE(buf.get(tmp, 4));
		EXPECT_EQ(tmp[0], (uint8_t)0x078);
		EXPECT_EQ(tmp[1], (uint8_t)0x056);
		EXPECT_EQ(tmp[2], (uint8_t)0x034);
		EXPECT_EQ(tmp[3], (uint8_t)0x012);
		EXPECT_TRUE(buf.compact());
	}

	g_xslab_destroy(slab);
}

TEST(buffer, put_get_ints)
{
	g_xslab_t* slab = g_xslab_init(1 + BLOCK_HEADER_SIZE);

	{
		buffer buf(slab);

		uint8_t a1 = 46;
		uint16_t a2 = 44861;
		uint32_t a3 = 456478156;
		uint64_t a4 = 98942185786156789LL;

		buf.put(a1);
		buf.put(a2);
		buf.put(a3);
		buf.put(a4);

		buf.flip();

		uint8_t b1;
		uint16_t b2;
		uint32_t b3;
		uint64_t b4;

		buf.get(b1);
		buf.get(b2);
		buf.get(b3);
		buf.get(b4);

		EXPECT_EQ(a1, b1);
		EXPECT_EQ(a2, b2);
		EXPECT_EQ(a3, b3);
		EXPECT_EQ(a4, b4);
	}

	g_xslab_destroy(slab);
}

TEST(buffer, peek_ints)
{
	g_xslab_t* slab = g_xslab_init(1 + BLOCK_HEADER_SIZE);

	{
		buffer buf(slab);

		uint8_t a1 = 164;
		uint16_t a2 = 8861;
		uint32_t a3 = 145648156;
		uint64_t a4 = 9594218578156789LL;

		buf.put(a1);
		buf.put(a2, false);
		buf.put(a3, false);
		buf.put(a4, false);

		buf.flip();

		uint8_t b1;
		uint16_t b2;
		uint32_t b3;
		uint64_t b4;

		buf.peek(b1);
		EXPECT_EQ(a1, b1);
		b1 = 0;
		EXPECT_TRUE(buf.get(b1));
		EXPECT_EQ(a1, b1);

		buf.peek(b2, false);
		EXPECT_EQ(a2, b2);
		b2 = 0;
		EXPECT_TRUE(buf.get(b2, false));
		EXPECT_EQ(a2, b2);

		buf.peek(b3, false);
		EXPECT_EQ(a3, b3);
		b3 = 0;
		EXPECT_TRUE(buf.get(b3, false));
		EXPECT_EQ(a3, b3);

		buf.peek(b4, false);
		EXPECT_EQ(a4, b4);
		b4 = 0;
		EXPECT_TRUE(buf.get(b4, false));
		EXPECT_EQ(a4, b4);
	}

	g_xslab_destroy(slab);
}

TEST(buffer, memory_boundary)
{
	{
		g_xslab_t* slab = g_xslab_init(20 + BLOCK_HEADER_SIZE);

		uint8_t tmp[100];

		for(int i=0;i<30;i++) {
			tmp[i] = (char)i;
		}

		{
			buffer buf(slab);
			EXPECT_TRUE(buf.put(tmp, 30));
			EXPECT_TRUE(buf.flip());
			EXPECT_TRUE(buf.get(tmp, 20));

			for (int i=0;i<20;i++) {
				ASSERT_EQ(i, tmp[i]);
			}

			EXPECT_TRUE(buf.get(tmp, 5));

			for (int i=0;i<5;i++) {
				ASSERT_EQ(i+20, tmp[i]);
			}

			EXPECT_TRUE(buf.compact());
			EXPECT_EQ(5, buf.position());

			EXPECT_TRUE(buf.put(tmp, 10));
			EXPECT_TRUE(buf.put(tmp, 21));
			EXPECT_EQ(60, buf.capacity());
			EXPECT_EQ(36, buf.position());
		}

		EXPECT_EQ(3, g_xslab_usable_amount(slab));

		g_xslab_destroy(slab);
	}

	{
		g_xslab_t* slab = g_xslab_init(100 + BLOCK_HEADER_SIZE);
		{
			buffer buf(slab);

			uint8_t tmp[100];
			EXPECT_TRUE(buf.put(tmp, 100));
			EXPECT_TRUE(buf.flip());

			EXPECT_TRUE(buf.get(tmp, 100));
			EXPECT_TRUE(buf.compact());

			EXPECT_TRUE(buf.put(tmp, 50));	// no more new alloc memory
		}
		EXPECT_EQ(2, g_xslab_usable_amount(slab));

		g_xslab_destroy(slab);
	}
}

TEST(buffer, reserve)
{
	g_xslab_t* slab = g_xslab_init(100 + BLOCK_HEADER_SIZE);

	{
		buffer buf(slab);
		buf.reserve(320);	// should alloc 4 block for 100 * 4 = 400 bytes
		EXPECT_EQ(400, buf.capacity());
		EXPECT_EQ(0, buf.position());
	}

	EXPECT_EQ(4, g_xslab_usable_amount(slab));

	{
		buffer buf(slab);
		buf.reserve(320);
		EXPECT_EQ(0, g_xslab_usable_amount(slab));

		for(uint32_t i=0;i<320/sizeof(i);i++) {
			buf.put(i);
		}
	}

	// no more new block allocated
	EXPECT_EQ(4, g_xslab_usable_amount(slab));

	g_xslab_destroy(slab);
}


int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    srand((unsigned)time(NULL));

    return RUN_ALL_TESTS();
}
