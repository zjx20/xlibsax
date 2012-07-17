/*
 * t_slab_benchmark.cpp
 *
 *  Created on: 2012-7-17
 *      Author: x
 */

#include <cstdlib>
#include <cstdio>
#include <vector>
#include <algorithm>

#include "sax/slab.h"
#include "sax/os_api.h"
#include "sax/mempool.h"

#define ALLOC_MAX_SIZE 1024
#define ALLOC_RATE 0.7
const int ALLOC_RAND_LIMIT = (int)(RAND_MAX * ALLOC_RATE);

void* malloc_wrap(size_t size)
{
	return malloc(size);
}

void free_wrap(void* ptr)
{
	free(ptr);
}

//void* test_malloc(void* param)
//{
//	int seed = *(int*)(((void**)param)[0]);
//	int count = *(int*)(((void**)param)[1]);
//	srand(seed);
//	std::vector<void*> ptrs;
//	ptrs.reserve(count);
//
//	for(int i=0;i<count;i++) {
//		if ((rand() > ALLOC_RAND_LIMIT) && ptrs.size() > 0) {
//			int rm = rand() % ptrs.size();
//			std::swap(ptrs[rm], ptrs.back());
//			free_wrap(ptrs.back());
//			ptrs.pop_back();
//		}
//		else {
//			int size = (rand() % ALLOC_MAX_SIZE) + 1;
//			ptrs.push_back(malloc_wrap(size));
//		}
//	}
//
//	std::for_each(ptrs.begin(), ptrs.end(), free);
//
//	return 0;
//}

void* test_malloc(void* param)
{
	int seed = *(int*)(((void**)param)[0]);
	int count = *(int*)(((void**)param)[1]);
	void** ptrs = (void**) malloc(sizeof(void*) * count);
	for(int i=0;i<count;i++) {
		ptrs[i] = malloc_wrap(seed);
	}

	for(int i=0;i<count;i++) {
		free_wrap(ptrs[i]);
	}

	return 0;
}

sax::slab_t** slabs = (sax::slab_t**) malloc(sizeof(sax::slab_t*) * (ALLOC_MAX_SIZE + 1));

//void* test_slab(void* param)
//{
//	int seed = *(int*)((void**)param)[0];
//	int count = *(int*)((void**)param)[1];
//
//	srand(seed);
//	std::vector< std::pair<int, void*> > ptrs;
//	ptrs.reserve(count);
//
//	for(int i=0;i<count;i++) {
//		if ((rand() > ALLOC_RAND_LIMIT) && ptrs.size() > 0) {
//			int rm = rand() % ptrs.size();
//			std::swap(ptrs[rm], ptrs.back());
//			std::pair<int, void*>& x = ptrs.back();
//			slabs[x.first]->free(x.second);
//			ptrs.pop_back();
//		}
//		else {
//			int size = (rand() % ALLOC_MAX_SIZE) + 1;
//			ptrs.push_back(std::make_pair(size, slabs[size]->alloc()));
//		}
//	}
//
//	for(std::vector< std::pair<int, void*> >::iterator it = ptrs.begin();
//			it != ptrs.end(); ++it) {
//		slabs[it->first]->free(it->second);
//	}
//
//	return 0;
//}

void* test_slab(void* param)
{
	int seed = *(int*)(((void**)param)[0]);
	int count = *(int*)(((void**)param)[1]);
	void** ptrs = (void**) malloc(sizeof(void*) * count);
	for(int i=0;i<count;i++) {
		ptrs[i] = slabs[seed]->alloc();
	}

	for(int i=0;i<count;i++) {
		slabs[seed]->free(ptrs[i]);
	}

	return 0;
}

slab_pool_t* pool;
g_spin_t* spin;

void* test_nginx(void* param)
{
	int seed = *(int*)(((void**)param)[0]);
	int count = *(int*)(((void**)param)[1]);
	srand(seed);
	std::vector<void*> ptrs;
	ptrs.reserve(count);

	for(int i=0;i<count;i++) {
		if ((rand() > ALLOC_RAND_LIMIT) && ptrs.size() > 0) {
			int rm = rand() % ptrs.size();
			std::swap(ptrs[rm], ptrs.back());
			g_spin_enter(spin, 8);
			g_slab_free(pool, ptrs.back());
			g_spin_leave(spin);
			ptrs.pop_back();
		}
		else {
			int size = (rand() % ALLOC_MAX_SIZE) + 1;
			g_spin_enter(spin, 8);
			void* ptr = g_slab_alloc(pool, size);
			g_spin_leave(spin);
			ptrs.push_back(ptr);
		}
	}

	for(std::vector<void*>::iterator it = ptrs.begin();
			it != ptrs.end(); ++it) {
		g_spin_enter(spin, 8);
		g_slab_free(pool, *it);
		g_spin_leave(spin);
	}

	return 0;
}

int main(int argc, char* argv[])
{
	if(argc < 4) {
		printf("usage: %s [0|1|2] seed count thread_num\n 0 for malloc, 1 for slab, 2 for nginx.\n", argv[0]);
		return 1;
	}

	int seed = std::atoi(argv[2]);
	int count = std::atoi(argv[3]);
	void* param[] = {(void*) &seed, (void*) &count};

	int thread_num = std::atoi(argv[4]);
	g_thread_t threads[thread_num];

	for(int i=1;i<=ALLOC_MAX_SIZE;i++) {
		slabs[i] = new sax::slab_t(i);
	}

	size_t total = g_shm_unit() * 102400;
	printf("%llu\n", total);
	g_slab_setup(g_shm_unit());
	void* addr = malloc(total);
	pool = g_slab_init(addr, total);

	spin = g_spin_init(PURE);

	std::vector<void*> ptrs;
	ptrs.reserve(thread_num * count);
	for(int i=thread_num * count;i>0;i--) {
		ptrs.push_back(slabs[seed]->alloc());
	}

	while(!ptrs.empty()) {
		slabs[seed]->free(ptrs.back());
		ptrs.pop_back();
	}

	if(argv[1][0] == '0') {
		for(int i=0;i<thread_num;i++) {
			threads[i] = g_thread_start(test_malloc, param);
		}
	}
	else if(argv[1][0] == '1') {
		for(int i=0;i<thread_num;i++) {
			threads[i] = g_thread_start(test_slab, param);
		}
	}
	else if(argv[1][0] == '2') {
		for(int i=0;i<thread_num;i++) {
			threads[i] = g_thread_start(test_nginx, param);
		}
	}

	for(int i=0;i<thread_num;i++) {
		long ret;
		g_thread_join(threads[i], &ret);
	}

	return 0;
}
