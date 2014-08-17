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

class limited_memory_pool : public block_mempool {
public:
	limited_memory_pool(block_mempool* mempool, size_t alloc_limit_ = (size_t)-1) :
		block_mempool(mempool->get_block_size(), (size_t)-1),
		count(0), inner_pool(mempool), alloc_limit(alloc_limit_) {}

	virtual void* allocate() {
		if(count < alloc_limit) {
			count++;
			return inner_pool->allocate();
		}
		return NULL;
	}

	virtual void recycle(void* ptr) {
		inner_pool->recycle(ptr);
	}

	size_t count;
	block_mempool* inner_pool;
	size_t alloc_limit;
};

const int32_t BLOCK_HEADER_SIZE = 16;

TEST(linked_list, push_pop)
{

#define ASIZE(a) (sizeof(a)/sizeof(a[0]))

	A arr[100];

	for(size_t i=0;i<ASIZE(arr);i++) {
		arr[i].a = i;
	}

	_linked_list<A> l;

	EXPECT_EQ(NULL, l.get_head());
	EXPECT_EQ(NULL, l.get_tail());
	EXPECT_EQ(NULL, l.pop_back());
	EXPECT_EQ(NULL, l.pop_front());

	l.push_back(&arr[0]);

	EXPECT_EQ(&arr[0], l.get_head());
	EXPECT_EQ(&arr[0], l.get_tail());
	EXPECT_EQ(&arr[0], l.pop_front());

	l.push_front(&arr[1]);

	EXPECT_EQ(&arr[1], l.get_head());
	EXPECT_EQ(&arr[1], l.get_tail());
	EXPECT_EQ(&arr[1], l.pop_back());

	for(size_t i=0;i<ASIZE(arr);i++) {
		l.push_back(&arr[i]);
	}

	for(size_t i=0;i<ASIZE(arr);i++) {
		A* tmp = l.pop_front();
		EXPECT_EQ(i, tmp->a);
	}

	////////////////////////////////////////

	for(size_t i=0;i<ASIZE(arr);i++) {
		l.push_back(&arr[i]);
	}

	for(size_t i=0;i<10;i++) {
		l.pop_front();
		l.pop_back();
	}

	A* h = l.get_head();
	int cc = 0;
	while(h != NULL) {
		cc++;
		h = h->next;
	}
	EXPECT_EQ(cc, 80);

	A* t = l.get_tail();
	cc = 0;
	while(t != NULL) {
		cc++;
		t = t->prev;
	}
	EXPECT_EQ(cc, 80);

#undef ASIZE
}

TEST(linked_list, split)
{

#define ASIZE(a) (sizeof(a)/sizeof(a[0]))

	A arr[100];

	for(size_t i=0;i<ASIZE(arr);i++) {
		arr[i].a = i;
	}

	_linked_list<A> l;
	_linked_list<A> l2;

	l.push_back(&arr[0]);
	l.split(&arr[0], l2);

	EXPECT_EQ(NULL, l.get_head());
	EXPECT_EQ(NULL, l.get_tail());

	EXPECT_EQ(&arr[0], l2.get_head());
	EXPECT_EQ(&arr[0], l2.get_tail());
	EXPECT_EQ(&arr[0], l2.pop_back());

	////////////////////////////////////////

	for(size_t i=0;i<ASIZE(arr);i++) {
		l.push_back(&arr[i]);
	}

	l.split(&arr[0], l2);

	EXPECT_EQ(NULL, l.get_head());
	EXPECT_EQ(NULL, l.get_tail());

	for(size_t i=0;i<ASIZE(arr);i++) {
		A* tmp = l2.pop_front();
		EXPECT_EQ(i, tmp->a);
	}

	////////////////////////////////////////

	for(size_t i=0;i<ASIZE(arr);i++) {
		l.push_back(&arr[i]);
	}

	l.split(&arr[50], l2);

	EXPECT_EQ(NULL, arr[0].prev);
	EXPECT_EQ(NULL, arr[49].next);
	EXPECT_EQ(NULL, arr[50].prev);
	EXPECT_EQ(NULL, arr[99].next);

	for(size_t i=0;i<50;i++) {
		A* tmp = l.pop_front();
		EXPECT_EQ(i, tmp->a);
	}

	for(size_t i=50;i<ASIZE(arr);i++) {
		A* tmp = l2.pop_front();
		EXPECT_EQ(i, tmp->a);
	}

	////////////////////////////////////////

	// test split last element
	l.push_back(&arr[0]);
	l.push_back(&arr[1]);

	l.split(&arr[1], l2);
	EXPECT_EQ(&arr[0], l.pop_back());
	EXPECT_EQ(&arr[1], l2.pop_back());

#undef ASIZE
}

TEST(linked_list, splice)
{

#define ASIZE(a) (sizeof(a)/sizeof(a[0]))

	A arr[100];

	for(size_t i=0;i<ASIZE(arr);i++) {
		arr[i].a = i;
	}

	_linked_list<A> l;
	_linked_list<A> l2;

	l.splice(l2);

	EXPECT_EQ(NULL, l.get_head());
	EXPECT_EQ(NULL, l.get_tail());
	EXPECT_EQ(NULL, l2.get_head());
	EXPECT_EQ(NULL, l2.get_tail());

	////////////////////////////////////////

	for(size_t i=0;i<50;i++) {
		l2.push_back(&arr[i]);
	}

	l.splice(l2);

	EXPECT_EQ(&arr[0], l.get_head());
	EXPECT_EQ(&arr[49], l.get_tail());
	EXPECT_EQ(NULL, l2.get_head());
	EXPECT_EQ(NULL, l2.get_tail());

	while(l.pop_front() != NULL);

	////////////////////////////////////////

	for(size_t i=0;i<50;i++) {
		l.push_back(&arr[i]);
	}

	l.splice(l2);

	EXPECT_EQ(&arr[0], l.get_head());
	EXPECT_EQ(&arr[49], l.get_tail());
	EXPECT_EQ(NULL, l2.get_head());
	EXPECT_EQ(NULL, l2.get_tail());

	while(l.pop_front() != NULL);

	////////////////////////////////////////

	for(size_t i=0;i<50;i++) {
		l.push_back(&arr[i]);
	}

	for(size_t i=50;i<ASIZE(arr);i++) {
		l2.push_back(&arr[i]);
	}

	EXPECT_EQ(NULL, arr[49].next);
	EXPECT_EQ(NULL, arr[50].prev);

	l.splice(l2);

	EXPECT_EQ(&arr[0], l.get_head());
	EXPECT_EQ(&arr[99], l.get_tail());
	EXPECT_EQ(&arr[49], arr[50].prev);
	EXPECT_EQ(&arr[50], arr[49].next);
	EXPECT_EQ(NULL, l2.get_head());
	EXPECT_EQ(NULL, l2.get_tail());


#undef ASIZE
}

TEST(linked_list, swap)
{
	A a, b;
	_linked_list<A> l;
	_linked_list<A> l2;

	l.push_back(&a);
	l2.push_front(&b);

	l.swap(l2);

	EXPECT_EQ(&b, l.get_head());
	EXPECT_EQ(&b, l.get_tail());

	EXPECT_EQ(&a, l2.get_head());
	EXPECT_EQ(&a, l2.get_tail());
}

TEST(simple_memory_pool, alloc_recycle)
{
	printf("please check memory leak with valgrind.\n");
	printf("valgrind --tool=memcheck --leak-check=full --show-reachable=yes ./program_name\n");

	{
		simple_block_mempool pool(16, 0);
		EXPECT_EQ(16, pool.get_block_size());

		char* ptr = (char*) pool.allocate();
		for(int i=0;i<10;i++) {
			ptr[i] = (char)i;
		}

		printf("normally, you will get a error in valgrind, in %s: %d.\n", __FILE__, __LINE__ + 1);
		if(ptr[16] == 0) {
			ptr[0] = (char)255;
		}

		pool.recycle(reinterpret_cast<void*>(ptr));
	}

	{
		simple_block_mempool pool(100, 512);
		void* ptr[100];
		for(int i=0;i<100;i++) {
			ptr[i] = pool.allocate();
		}

		for(int i=0;i<100;i++) {
			pool.recycle(ptr[i]);
		}

		EXPECT_EQ(600, pool.get_holding_size());
	}
}

TEST(buffer, empty)
{
	simple_block_mempool pool(1 + BLOCK_HEADER_SIZE);
	limited_memory_pool pool2(&pool, 0);
	buffer buf(&pool2);
	buffer buf2(&pool2);

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

	EXPECT_EQ(0, buf.capacity());
	EXPECT_EQ(0, buf.position());

	EXPECT_EQ(0, buf.remaining());
	EXPECT_EQ(0, buf.data_length());
	EXPECT_FALSE(buf.has_remaining());
	EXPECT_FALSE(buf.has_data());

	EXPECT_FALSE(buf.peek(tmp[0]));
}

TEST(buffer, writing_mode)
{
	simple_block_mempool pool(1 + BLOCK_HEADER_SIZE);

	const char* str = "this is a test string.";

	buffer buf(&pool);

	EXPECT_TRUE(buf.put((uint8_t*)str, strlen(str)));
	EXPECT_EQ(strlen(str), buf.position());

	EXPECT_TRUE(buf.put((uint8_t*)str, strlen(str)));
	EXPECT_EQ(strlen(str) * 2, buf.position());

	EXPECT_EQ(strlen(str) * 2, buf.capacity());
	EXPECT_FALSE(buf.compact());

	EXPECT_EQ(strlen(str) * 2, buf.data_length());
	EXPECT_TRUE(buf.has_data());

	EXPECT_EQ(0, buf.remaining());
	EXPECT_FALSE(buf.has_remaining());

	EXPECT_TRUE(buf.flip());

	EXPECT_EQ(0, buf.position());
}

TEST(buffer, reading_mode)
{
	simple_block_mempool pool(1 + BLOCK_HEADER_SIZE);

	const char* str = "this is a test string.";
	uint8_t tmp[100];

	buffer buf(&pool);

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
	EXPECT_EQ(0, buf.data_length());
	EXPECT_FALSE(buf.has_data());

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
	simple_block_mempool pool(2 + BLOCK_HEADER_SIZE);

	buffer buf1(&pool);
	buffer buf2(&pool);

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

TEST(buffer, seeking)	// preappend rewind
{
	simple_block_mempool pool(1 + BLOCK_HEADER_SIZE);

	const char* str = "this is a test string";
	uint8_t tmp[100];

	// mark reset
	{
		buffer buf(&pool);

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
		buffer buf(&pool);

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
		buffer buf(&pool);

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
}

TEST(buffer, clear)
{
	simple_block_mempool pool(1 + BLOCK_HEADER_SIZE);

	{
		buffer buf(&pool);

		buf.put((uint8_t*)"testing", 7);
		buf.flip();

		buf.clear();
		EXPECT_EQ(7 * (1 + BLOCK_HEADER_SIZE), pool.get_holding_size());
		EXPECT_EQ(0, buf.position());
		EXPECT_EQ(0, buf.capacity());
	}
}

TEST(buffer, endianness)
{
	simple_block_mempool pool(1 + BLOCK_HEADER_SIZE);

	buffer buf(&pool);

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
	simple_block_mempool pool(1 + BLOCK_HEADER_SIZE);

	buffer buf(&pool);

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
	simple_block_mempool pool(1 + BLOCK_HEADER_SIZE);

	buffer buf(&pool);

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
	{
		simple_block_mempool pool(20 + BLOCK_HEADER_SIZE);

		uint8_t tmp[100];

		for(int i=0;i<30;i++) {
			tmp[i] = (char)i;
		}

		{
			buffer buf(&pool);
			EXPECT_TRUE(buf.put(tmp, 30));
			EXPECT_TRUE(buf.flip());
			EXPECT_TRUE(buf.get(tmp, 10));
			EXPECT_TRUE(buf.compact());
			EXPECT_EQ(20, buf.position());
			EXPECT_TRUE(buf.put(tmp, 11));
			EXPECT_EQ(60, buf.capacity());
			EXPECT_EQ(31, buf.position());
		}

		EXPECT_EQ((20 + BLOCK_HEADER_SIZE) * 3, pool.get_holding_size());
	}

	{
		simple_block_mempool pool(100 + 24);
		{
			buffer buf(&pool);

			uint8_t tmp[100];
			EXPECT_TRUE(buf.put(tmp, 100));
			EXPECT_TRUE(buf.flip());

			EXPECT_TRUE(buf.get(tmp, 100));
			EXPECT_TRUE(buf.compact());

			EXPECT_TRUE(buf.put(tmp, 50));	// no more new alloc memory
		}
		EXPECT_EQ(124, pool.get_holding_size());
	}
}

TEST(buffer, limit_memory)
{
	simple_block_mempool pool(1 + BLOCK_HEADER_SIZE);

	{
		limited_memory_pool lpool(&pool, 10);

		buffer buf(&lpool);

		EXPECT_FALSE(buf.put((uint8_t*)"long long long string", 21));
	}

	{
		limited_memory_pool lpool(&pool, 10);

		buffer buf(&lpool);

		EXPECT_TRUE(buf.put((uint8_t*)"short str", 9));
	}
}

TEST(buffer, reserve)
{
	simple_block_mempool pool(100 + BLOCK_HEADER_SIZE);

	{
		buffer buf(&pool);
		buf.reserve(320);	// should alloc 4 block for 100 * 4 = 400 bytes
		EXPECT_EQ(400, buf.capacity());
		EXPECT_EQ(0, buf.position());
	}

	EXPECT_EQ((100 + BLOCK_HEADER_SIZE) * 4, pool.get_holding_size());

	{
		buffer buf(&pool);
		buf.reserve(320);
		EXPECT_EQ(0, pool.get_holding_size());

		for(uint32_t i=0;i<320/sizeof(i);i++) {
			buf.put(i);
		}
	}

	// no more new block allocated
	EXPECT_EQ((100 + BLOCK_HEADER_SIZE) * 4, pool.get_holding_size());
}


int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    srand((unsigned)time(NULL));

    return RUN_ALL_TESTS();
}
