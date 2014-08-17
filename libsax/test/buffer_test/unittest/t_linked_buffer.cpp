/*
 * t_linked_buffer.cpp
 *
 *  Created on: 2012-4-4
 *      Author: X
 */

#include "gtest/gtest.h"
#include "sax/net/linked_buffer.h"

using namespace sax;

struct A {
	A* next;
	A* prev;
	int a;
};

const int32_t BLOCK_HEADER_SIZE = 8;
const int32_t BLOCK_SIZE = g_shm_unit() - BLOCK_HEADER_SIZE;

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

TEST(buffer, empty)
{
	linked_buffer buf;
	linked_buffer buf2;

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

	EXPECT_EQ(BLOCK_SIZE/*alloc one block in constructor*/, buf.capacity());
	EXPECT_EQ(0, buf.position());

	EXPECT_EQ(0, buf.remaining());
	EXPECT_EQ(linked_buffer::INVALID_VALUE, buf.data_length());

	EXPECT_FALSE(buf.peek(tmp[0]));
}

TEST(buffer, writing_mode)
{
	const char* str = "this is a test string.";

	linked_buffer buf;

	EXPECT_TRUE(buf.put((uint8_t*)str, strlen(str)));
	EXPECT_EQ(strlen(str), buf.position());

	EXPECT_TRUE(buf.put((uint8_t*)str, strlen(str)));
	EXPECT_EQ(strlen(str) * 2, buf.position());

	EXPECT_FALSE(buf.compact());

	EXPECT_EQ(strlen(str) * 2, buf.data_length());

	EXPECT_EQ(linked_buffer::INVALID_VALUE, buf.remaining());

	EXPECT_TRUE(buf.flip());

	EXPECT_EQ(0, buf.position());

	EXPECT_TRUE(buf.compact());

	EXPECT_TRUE(buf.skip(BLOCK_SIZE * 2));
	EXPECT_EQ(2 * BLOCK_SIZE + strlen(str) * 2, buf.position());
	EXPECT_EQ(3 * BLOCK_SIZE, buf.capacity());

	EXPECT_TRUE(buf.flip());
	EXPECT_TRUE(buf.skip(BLOCK_SIZE));
	EXPECT_EQ(BLOCK_SIZE, buf.position());
	EXPECT_TRUE(buf.compact());
	EXPECT_EQ(2 * BLOCK_SIZE, buf.capacity());
}

TEST(buffer, reading_mode)
{
	const char* str = "this is a test string.";
	uint8_t tmp[100];

	linked_buffer buf;

	EXPECT_TRUE(buf.put((uint8_t*)str, strlen(str)));
	EXPECT_EQ(strlen(str), buf.position());

	EXPECT_TRUE(buf.flip());
	EXPECT_EQ(0, buf.position());

	EXPECT_FALSE(buf.put(tmp, 1));
	EXPECT_FALSE(buf.get(tmp, 10000));

	EXPECT_TRUE(buf.get(tmp, 4));
	EXPECT_EQ(0, memcmp(tmp, str, 4));

	EXPECT_EQ(strlen(str) - 4, buf.remaining());
	EXPECT_EQ(linked_buffer::INVALID_VALUE, buf.data_length());

	EXPECT_TRUE(buf.compact());
	EXPECT_FALSE(buf.compact());
	EXPECT_EQ(strlen(str) - 4, buf.position());

	EXPECT_TRUE(buf.put((uint8_t*)"have fun", 8));
	EXPECT_TRUE(buf.flip());

	EXPECT_EQ(0, buf.position());
	EXPECT_TRUE(buf.get(tmp, strlen(str) - 4 + 8));
	EXPECT_EQ(0, memcmp(tmp, " is a test string.have fun", strlen(str) - 4 + 8));
}

TEST(buffer, put_get_buffer)
{
	linked_buffer buf1;
	linked_buffer buf2;

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

TEST(buffer, seeking)	// skip rewind
{
	const char* str = "this is a test string";
	uint8_t tmp[100];

	// mark reset
	{
		linked_buffer buf;

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
		linked_buffer buf;

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
		linked_buffer buf;

		uint8_t tmp[100];

		EXPECT_TRUE(buf.put((uint8_t*)"what ", 5));
		EXPECT_TRUE(buf.skip(2));
		EXPECT_EQ(7, buf.position());
		EXPECT_TRUE(buf.put((uint8_t*)"wonderful day", 13));
		EXPECT_TRUE(buf.reset());
		EXPECT_TRUE(buf.put((uint8_t*)"a ", 2));
		EXPECT_TRUE(buf.reset());
		EXPECT_TRUE(buf.flip());

		EXPECT_TRUE(buf.skip(5));
		EXPECT_TRUE(buf.get(tmp, buf.remaining()));
		EXPECT_EQ(0, memcmp(tmp, "a wonderful day", 15));
	}
}

TEST(buffer, clear)
{
	linked_buffer buf;

	buf.skip(BLOCK_SIZE * 3 + 100);
	buf.flip();

	EXPECT_EQ(4 * BLOCK_SIZE, buf.capacity());

	buf.clear();

	EXPECT_EQ(0, buf.position());
	EXPECT_TRUE(buf.data_length() == 0);
	EXPECT_TRUE(buf.remaining() == linked_buffer::INVALID_VALUE);

	EXPECT_EQ(BLOCK_SIZE, buf.capacity());
}

TEST(buffer, endianness)
{
	linked_buffer buf;

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

TEST(buffer, put_get_ints)
{
	linked_buffer buf;

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

TEST(buffer, peek_ints)
{
	linked_buffer buf;

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

TEST(buffer, memory_boundary)
{
	linked_buffer buf;
	EXPECT_TRUE(buf.skip(BLOCK_SIZE - 1));
	EXPECT_EQ(BLOCK_SIZE, buf.capacity());
	EXPECT_TRUE(buf.put((uint8_t)'a'));
	EXPECT_EQ(BLOCK_SIZE * 2, buf.capacity());
	EXPECT_EQ(BLOCK_SIZE, buf.position());

	EXPECT_TRUE(buf.flip());

	EXPECT_TRUE(buf.skip(BLOCK_SIZE - 1));
	uint8_t tmp;
	EXPECT_TRUE(buf.get(tmp));
	EXPECT_EQ((uint8_t)'a', tmp);
	EXPECT_TRUE(buf.compact());
	EXPECT_EQ(BLOCK_SIZE, buf.capacity());
	EXPECT_EQ(0, buf.position());
}

TEST(buffer, reserve)
{
	linked_buffer buf;
	buf.reserve(BLOCK_SIZE * 3 - 1);
	EXPECT_EQ(3 * BLOCK_SIZE, buf.capacity());
	buf.reserve(BLOCK_SIZE * 3);
	EXPECT_EQ(4 * BLOCK_SIZE, buf.capacity());
	buf.reserve(BLOCK_SIZE * 3 + 1);
	EXPECT_EQ(4 * BLOCK_SIZE, buf.capacity());
	EXPECT_EQ(0, buf.position());
}

TEST(buffer, direct_get)
{
	linked_buffer buf;
	uint32_t limit_length;
	EXPECT_EQ(NULL, buf.direct_get(limit_length));
	EXPECT_FALSE(buf.commit_get(NULL, 100));

	buf.put((uint8_t*)"hello", 5);
	buf.flip();

	char* ptr = buf.direct_get(limit_length);
	EXPECT_EQ(5u, limit_length);
	EXPECT_EQ("hello", std::string(ptr, limit_length));

	EXPECT_FALSE(buf.commit_get(ptr, 100));
	EXPECT_TRUE(buf.commit_get(ptr, 3));

	char tmp[BLOCK_SIZE + 100];
	EXPECT_TRUE(buf.get((uint8_t*)tmp, 2));
	EXPECT_EQ("lo", std::string(tmp, 2));

	buf.compact();

	buf.put((uint8_t*)tmp, sizeof(tmp));
	buf.flip();

	ptr = buf.direct_get(limit_length);
	EXPECT_EQ(BLOCK_SIZE, limit_length);
	EXPECT_FALSE(buf.commit_get(ptr, BLOCK_SIZE + 1));
	EXPECT_TRUE(buf.commit_get(ptr, BLOCK_SIZE - 10));

	ptr = buf.direct_get(limit_length);
	EXPECT_EQ(10, limit_length);
	EXPECT_TRUE(buf.commit_get(ptr, 10));

	ptr = buf.direct_get(limit_length);
	EXPECT_EQ(100, limit_length);
	EXPECT_TRUE(buf.commit_get(ptr, 100));

	ptr = buf.direct_get(limit_length);
	EXPECT_EQ(0, limit_length);
}


int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    srand((unsigned)time(NULL));

    return RUN_ALL_TESTS();
}
