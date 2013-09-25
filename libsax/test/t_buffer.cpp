/*
 * t_buffer.cpp
 *
 *  Created on: 2012-4-4
 *      Author: X
 */

#include "gtest/gtest.h"
#include "sax/net/buffer.h"

using namespace sax;

const uint32_t BLOCK_HEADER_SIZE = 0;
const uint32_t BLOCK_SIZE = g_shm_unit() - BLOCK_HEADER_SIZE;

TEST(buffer, empty)
{
	buffer buf;

	uint8_t tmp[100];

	buf.mark();
	EXPECT_TRUE(buf.reset());

	EXPECT_TRUE(buf.flip());
	EXPECT_TRUE(buf.get(tmp, 0));
	EXPECT_TRUE(buf.compact());

	EXPECT_EQ(buffer::INVALID_VALUE, buf.remaining());
	EXPECT_EQ(uint32_t(0), buf.data_length());

	EXPECT_TRUE(buf.put(tmp, 0));
	EXPECT_TRUE(buf.flip());

	EXPECT_FALSE(buf.get(tmp, 1));

	EXPECT_EQ(BLOCK_SIZE/*alloc one block in constructor*/, buf.capacity());
	EXPECT_EQ(0, buf.position());

	EXPECT_EQ(0, buf.remaining());
	EXPECT_EQ(buffer::INVALID_VALUE, buf.data_length());

	EXPECT_FALSE(buf.peek(tmp[0]));
}

TEST(buffer, writing_mode)
{
	const char* str = "this is a test string.";

	buffer buf;

	EXPECT_TRUE(buf.put((uint8_t*)str, strlen(str)));
	EXPECT_EQ(strlen(str), buf.position());

	EXPECT_TRUE(buf.put((uint8_t*)str, strlen(str)));
	EXPECT_EQ(strlen(str) * 2, buf.position());

	EXPECT_FALSE(buf.compact());

	EXPECT_EQ(strlen(str) * 2, buf.data_length());

	EXPECT_EQ(buffer::INVALID_VALUE, buf.remaining());

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
	EXPECT_EQ(3 * BLOCK_SIZE, buf.capacity());	// no shrink
}

TEST(buffer, reading_mode)
{
	const char* str = "this is a test string.";
	uint8_t tmp[100];

	buffer buf;

	EXPECT_TRUE(buf.put((uint8_t*)str, strlen(str)));
	EXPECT_EQ(strlen(str), buf.position());

	EXPECT_TRUE(buf.flip());
	EXPECT_EQ(0, buf.position());

	EXPECT_FALSE(buf.put(tmp, 1));
	EXPECT_FALSE(buf.get(tmp, 10000));

	EXPECT_TRUE(buf.get(tmp, 4));
	EXPECT_EQ(0, memcmp(tmp, str, 4));

	EXPECT_EQ(strlen(str) - 4, buf.remaining());
	EXPECT_EQ(buffer::INVALID_VALUE, buf.data_length());

	EXPECT_TRUE(buf.compact());
	EXPECT_FALSE(buf.compact());
	EXPECT_EQ(strlen(str) - 4, buf.position());

	EXPECT_TRUE(buf.put((uint8_t*)"have fun", 8));
	EXPECT_TRUE(buf.flip());

	EXPECT_EQ(0, buf.position());
	EXPECT_TRUE(buf.get(tmp, strlen(str) - 4 + 8));
	EXPECT_EQ(0, memcmp(tmp, " is a test string.have fun", strlen(str) - 4 + 8));
}

TEST(buffer, seeking)	// skip rewind
{
	const char* str = "this is a test string";
	uint8_t tmp[100];

	// mark reset
	{
		buffer buf;

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
		buffer buf;

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
		buffer buf;

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
	buffer buf;

	buf.skip(BLOCK_SIZE * 3 + 100);
	buf.flip();

	EXPECT_EQ(4 * BLOCK_SIZE, buf.capacity());

	buf.clear();

	EXPECT_EQ(0, buf.position());
	EXPECT_TRUE(buf.data_length() == 0);
	EXPECT_TRUE(buf.remaining() == buffer::INVALID_VALUE);

	EXPECT_EQ(4 * BLOCK_SIZE, buf.capacity());	// no shrink
}

TEST(buffer, endianness)
{
	buffer buf;

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
	buffer buf;

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
	buffer buf;

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
	buffer buf;
	EXPECT_TRUE(buf.skip(BLOCK_SIZE - 1));
	EXPECT_EQ(BLOCK_SIZE, buf.capacity());
	EXPECT_TRUE(buf.put((uint8_t)'a'));
	EXPECT_EQ(BLOCK_SIZE, buf.capacity());
	EXPECT_EQ(BLOCK_SIZE, buf.position());

	EXPECT_TRUE(buf.put((uint8_t)'a'));
	EXPECT_EQ(2 * BLOCK_SIZE, buf.capacity());

	EXPECT_TRUE(buf.flip());

	EXPECT_TRUE(buf.skip(BLOCK_SIZE - 1));
	uint8_t tmp;
	EXPECT_TRUE(buf.get(tmp));
	EXPECT_EQ((uint8_t)'a', tmp);
	EXPECT_TRUE(buf.compact());
	EXPECT_EQ(2 * BLOCK_SIZE, buf.capacity());
	EXPECT_EQ(1, buf.position());
}

TEST(buffer, limit)
{
	buffer buf;
	char str[] = "0123456789 testing string";
	ASSERT_TRUE(buf.put((uint8_t*)str, strlen(str)));
	ASSERT_TRUE(buf.flip());

	ASSERT_EQ(strlen(str), buf.limit(10));

	char tmp[100];
	EXPECT_FALSE(buf.get((uint8_t*)tmp, strlen(str)));
	EXPECT_TRUE(buf.get((uint8_t*)tmp, 10));
	EXPECT_EQ("0123456789", std::string(tmp, 10));

	ASSERT_EQ(10, buf.limit(strlen(str)));
	EXPECT_TRUE(buf.get((uint8_t*)tmp, strlen(str)-10));
	EXPECT_EQ(str + 10, std::string(tmp, strlen(str)-10));
}

TEST(buffer, expend)
{
	{
		buffer buf;
		EXPECT_EQ(BLOCK_SIZE, buf.capacity());
		EXPECT_TRUE(buf.skip(BLOCK_SIZE + 1));
		EXPECT_EQ(2 * BLOCK_SIZE, buf.capacity());
	}

	{
		buffer buf;
		char tmp[200];
		std::string res = "";

		uint32_t total = 0;
		while (total <= BLOCK_SIZE) {
			sprintf(tmp, "%d", rand());
			total += strlen(tmp);
			res += tmp;
			ASSERT_TRUE(buf.put((uint8_t*)tmp, strlen(tmp)));
		}

		ASSERT_TRUE(buf.flip());
		EXPECT_EQ(res.size(), buf.remaining());

		char str[2*BLOCK_SIZE];
		EXPECT_TRUE(buf.get((uint8_t*)str, buf.remaining()));
		EXPECT_EQ(res, std::string(str, res.size()));
	}
}

TEST(buffer, moving)
{
	{
		buffer buf;
		char str[] = "f486gv48dhg7534h86sr4g56w7t5566877ty53j4878t6k1mh23dhj ilfjdlk flafjksdlvnz;vh";
		buf.put((uint8_t*)str, strlen(str));
		buf.flip();
		char tmp[200];

		buf.get((uint8_t*)tmp, 10);
		EXPECT_EQ(std::string(str, 10), std::string(tmp, 10));

		buf.compact();
		buf.flip();

		buf.get((uint8_t*)tmp, 10);
		EXPECT_EQ(std::string(str+10, 10), std::string(tmp, 10));

		buf.compact();
		buf.flip();

		buf.get((uint8_t*)tmp, 10);
		EXPECT_EQ(std::string(str+20, 10), std::string(tmp, 10));
		EXPECT_EQ(strlen(str) - 30, buf.remaining());
	}
}

TEST(buffer, direct_put)
{
	buffer buf;
	char* ptr = buf.direct_put(100);
	memcpy(ptr, "hello", 5);
	EXPECT_TRUE(buf.commit_put(ptr, 5));
	EXPECT_FALSE(buf.commit_put(ptr, 5));
	EXPECT_TRUE(buf.flip());

	char tmp[200] = {0};
	EXPECT_TRUE(buf.get((uint8_t*)tmp, buf.remaining()));
	EXPECT_STREQ("hello", tmp);

	EXPECT_FALSE(buf.commit_put(ptr, 123));
	EXPECT_EQ(NULL, buf.direct_put(10));

	EXPECT_TRUE(buf.compact());
	ptr = buf.direct_put(100);
	EXPECT_FALSE(buf.commit_put(ptr, BLOCK_SIZE + 100));
}

TEST(buffer, direct_get)
{
	buffer buf;

	EXPECT_EQ(NULL, buf.direct_get());
	EXPECT_FALSE(buf.commit_get(NULL, 213));

	buf.put((uint8_t*) "something", 9);
	buf.put((uint32_t) 123456);
	buf.flip();
	char* ptr = buf.direct_get();
	char tmp[200] = {0};
	memcpy(tmp, ptr, 9);
	EXPECT_STREQ("something", tmp);
	EXPECT_FALSE(buf.commit_get(ptr, 100));
	EXPECT_TRUE(buf.commit_get(ptr, 9));
	EXPECT_FALSE(buf.commit_get(ptr, 1));

	uint32_t tmp_num;
	EXPECT_TRUE(buf.get(tmp_num));
	EXPECT_EQ(123456U, tmp_num);
}

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    srand((unsigned)time(NULL));

    return RUN_ALL_TESTS();
}
