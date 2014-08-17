/*
 * t_slab_benchmark.cpp
 *
 *  Created on: 2012-7-17
 *      Author: x
 */

#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <vector>
#include <list>
#include <algorithm>
#include <typeinfo>

#include "sax/slabutil.h"
#include "sax/os_api.h"
#include "sax/mempool.h"

void* malloc_wrap(size_t size)
{
	return malloc(size);
}

void free_wrap(void* ptr)
{
	free(ptr);
}

clock_t test_malloc(int count)
{
	void** ptrs = new void*[count];
	clock_t start = clock();
	for (int i=0;i<count;i++) {
		ptrs[i] = malloc_wrap(24);
	}
	clock_t end = clock();

	for (int i=0;i<count;i++) {
		free_wrap(ptrs[i]);
	}

	delete[] ptrs;

	return end - start;
}

clock_t test_xslab(int count)
{
	void** ptrs = new void*[count];
	g_xslab_t* slab = g_xslab_init(24);
	for (int i=0;i<count;i++) {
		ptrs[i] = g_xslab_alloc(slab);
	}

	for (int i=0;i<count;i++) {
		g_xslab_free(slab, ptrs[i]);
	}

	clock_t start = clock();
	for (int i=0;i<count;i++) {
		ptrs[i] = g_xslab_alloc(slab);
	}
	clock_t end = clock();

	for (int i=0;i<count;i++) {
		g_xslab_free(slab, ptrs[i]);
	}

	g_xslab_destroy(slab);
	delete[] ptrs;

	return end - start;
}

clock_t test_slab(int count)
{
	void** ptrs = new void*[count];
	sax::slab_t& slab = sax::slab_holder<24>::get_slab();
	for (int i=0;i<count;i++) {
		ptrs[i] = slab.alloc();
	}

	for (int i=0;i<count;i++) {
		slab.free(ptrs[i]);
	}

	clock_t start = clock();
	for (int i=0;i<count;i++) {
		ptrs[i] = slab.alloc();
	}
	clock_t end = clock();

	for (int i=0;i<count;i++) {
		slab.free(ptrs[i]);
	}

	delete[] ptrs;

	return end - start;
}

clock_t test_std_allocator(int count)
{
	clock_t start, end;
	{
		std::list<int> l;
		start = clock();
		for (int i=0;i<count;i++) {
			l.push_back(i);
		}
		end = clock();
	}

	return end - start;
}

clock_t test_slab_stl_allocator(int count)
{
	clock_t start, end;

//	{
//		std::list<int, sax::slab_stl_allocator<int> > l;
//		for (int i=0;i<count;i++) {
//			l.push_back(i);
//		}
//	}

	{
		std::list<int, sax::slab_stl_allocator<int> > l;
		start = clock();
		for (int i=0;i<count;i++) {
			l.push_back(i);
		}
		end = clock();
	}

	return end - start;
}

int main(int argc, char* argv[])
{
	if(argc < 2) {
		printf("usage: %s count\n", argv[0]);
		return 1;
	}

	int count = std::atoi(argv[1]);

	printf("test_malloc: %lu\n", test_malloc(count));
	printf("test_slab: %lu\n", test_slab(count));
	printf("test_xslab: %lu\n", test_xslab(count));
	printf("test_std_allocator: %lu\n", test_std_allocator(count));
	printf("test_slab_stl_allocator: %lu\n", test_slab_stl_allocator(count));

	return 0;
}
