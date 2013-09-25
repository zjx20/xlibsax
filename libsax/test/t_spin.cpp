/*
 * t_spin.cpp
 *
 *  Created on: 2012-7-9
 *      Author: x
 */

#include <cstdio>
#include <cstdlib>
#include <pthread.h>

#include "sax/os_api.h"

#if !defined(DEBUG)
#define DEBUG 0
#endif

#if !defined(SPIN_PURE) && !defined(SPIN_RWLOCK_PREFER_READER) && !defined(SPIN_RWLOCK_PREFER_WRITER) &&	\
	!defined(PTHREAD_MUTEX) && !defined(PTHREAD_SPIN) && \
	!defined(PTHREAD_RWLOCK_PREFER_READER) && !defined(PTHREAD_RWLOCK_PREFER_WRITER)
#warning "using self define lock type"
#define SPIN_RWLOCK_PREFER_READER
#endif

#if DEBUG
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

volatile int counter = 0;
volatile int thread_count;
int thread_total;
g_mutex_t* g_lock = NULL;

void inc_thread_count()
{
	g_mutex_enter(g_lock);
	thread_count++;
	g_mutex_leave(g_lock);
}

#if defined(SPIN_RWLOCK_PREFER_READER) || defined(SPIN_RWLOCK_PREFER_WRITER)
void* reader_func(void* param)
{
	inc_thread_count();
	while (thread_count != thread_total) g_thread_yield();

	void** params = reinterpret_cast<void**>(param);
	g_spin_rw_t* spin = (g_spin_rw_t*) params[0];
	int spin_times = *((int*) params[1]);
	int read_count = *((int*) params[2]);
	int junk_sum = 0;
	DEBUG_PRINT("begin reader work tid: %ld\n", g_thread_id());
	for (int i = 0; i < read_count; i++) {
		g_spin_lockr(spin, spin_times);
		DEBUG_PRINT("get lockr tid: %ld processed: %d\n", g_thread_id(), i);
		junk_sum += counter;
		g_spin_unlockr(spin);
	}
	return 0;
}

void* writer_func(void* param)
{
	inc_thread_count();
	while (thread_count != thread_total) g_thread_yield();

	void** params = reinterpret_cast<void**>(param);
	g_spin_rw_t* spin = (g_spin_rw_t*) params[0];
	int spin_times = *((int*) params[1]);
	int write_count = *((int*) params[2]);
	DEBUG_PRINT("begin writer work tid: %ld\n", g_thread_id());
	for (int i = 0; i < write_count; i++) {
		g_spin_lockw(spin, spin_times);
		DEBUG_PRINT("get lockw tid: %ld processed: %d\n", g_thread_id(), i);
		counter += 1;
		g_spin_unlockw(spin);
	}
	return 0;
}

void* init_lock()
{
	int type;
#if defined(SPIN_RWLOCK_PREFER_READER)
	type = 0;
#elif defined(SPIN_RWLOCK_PREFER_WRITER)
	type = 1;
#else
#error error!
#endif
	g_spin_rw_t* s = g_spin_rw_init(type);
	return s;
}
#endif

#if defined(SPIN_PURE)
void* reader_func(void* param)
{
	inc_thread_count();
	while (thread_count != thread_total) g_thread_yield();

	void** params = reinterpret_cast<void**>(param);
	g_spin_t* spin = (g_spin_t*) params[0];
	int spin_times = *((int*) params[1]);
	int read_count = *((int*) params[2]);
	int junk_sum = 0;
	DEBUG_PRINT("begin reader work tid: %ld\n", g_thread_id());
	for (int i = 0; i < read_count; i++) {
		g_spin_enter(spin, spin_times);
		DEBUG_PRINT("get spin tid: %ld processed: %d\n", g_thread_id(), i);
		junk_sum += counter;
		g_spin_leave(spin);
	}
	return 0;
}

void* writer_func(void* param)
{
	inc_thread_count();
	while (thread_count != thread_total) g_thread_yield();

	void** params = reinterpret_cast<void**>(param);
	g_spin_t* spin = (g_spin_t*) params[0];
	int spin_times = *((int*) params[1]);
	int write_count = *((int*) params[2]);
	DEBUG_PRINT("begin writer work tid: %ld\n", g_thread_id());
	for (int i = 0; i < write_count; i++) {
		g_spin_enter(spin, spin_times);
		DEBUG_PRINT("get spin tid: %ld processed: %d\n", g_thread_id(), i);
		counter += 1;
		g_spin_leave(spin);
	}
	return 0;
}

void* init_lock()
{
	g_spin_t* s = g_spin_init();
	return s;
}
#endif

#if defined(PTHREAD_MUTEX)
void* reader_func(void* param)
{
	inc_thread_count();
	while (thread_count != thread_total) g_thread_yield();

	void** params = reinterpret_cast<void**>(param);
	pthread_mutex_t* lock = (pthread_mutex_t*) params[0];
	//int spin_times = *((int*) params[1]);
	int read_count = *((int*) params[2]);
	int junk_sum = 0;
	DEBUG_PRINT("begin reader work tid: %ld\n", g_thread_id());
	for (int i = 0; i < read_count; i++) {
		pthread_mutex_lock(lock);
		DEBUG_PRINT("get mutex tid: %ld processed: %d\n", g_thread_id(), i);
		junk_sum += counter;
		pthread_mutex_unlock(lock);
	}
	return 0;
}

void* writer_func(void* param)
{
	inc_thread_count();
	while (thread_count != thread_total) g_thread_yield();

	void** params = reinterpret_cast<void**>(param);
	pthread_mutex_t* lock = (pthread_mutex_t*) params[0];
	//int spin_times = *((int*) params[1]);
	int write_count = *((int*) params[2]);
	DEBUG_PRINT("begin writer work tid: %ld\n", g_thread_id());
	for (int i = 0; i < write_count; i++) {
		pthread_mutex_lock(lock);
		DEBUG_PRINT("get mutex tid: %ld processed: %d\n", g_thread_id(), i);
		counter += 1;
		pthread_mutex_unlock(lock);
	}
	return 0;
}

void* init_lock()
{
	pthread_mutex_t* lock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(lock, NULL);
	return lock;
}
#endif

#if defined(__APPLE_CC__) && defined(PTHREAD_SPIN)
#warning there is no pthread_spinlock_t in mac osx...
#undef PTHREAD_SPIN
void* reader_func(void* param) {return 0;}
void* writer_func(void* param) {return 0;}
void* init_lock() {return NULL;}
#endif

#if defined(PTHREAD_SPIN)
void* reader_func(void* param)
{
	inc_thread_count();
	while (thread_count != thread_total) g_thread_yield();

	void** params = reinterpret_cast<void**>(param);
	pthread_spinlock_t* lock = (pthread_spinlock_t*) params[0];
	//int spin_times = *((int*) params[1]);
	int read_count = *((int*) params[2]);
	int junk_sum = 0;
	DEBUG_PRINT("begin reader work tid: %ld\n", g_thread_id());
	for (int i = 0; i < read_count; i++) {
		pthread_spin_lock(lock);
		DEBUG_PRINT("get spin tid: %ld processed: %d\n", g_thread_id(), i);
		junk_sum += counter;
		pthread_spin_unlock(lock);
	}
	return 0;
}

void* writer_func(void* param)
{
	inc_thread_count();
	while (thread_count != thread_total) g_thread_yield();

	void** params = reinterpret_cast<void**>(param);
	pthread_spinlock_t* lock = (pthread_spinlock_t*) params[0];
	//int spin_times = *((int*) params[1]);
	int write_count = *((int*) params[2]);
	DEBUG_PRINT("begin writer work tid: %ld\n", g_thread_id());
	for (int i = 0; i < write_count; i++) {
		pthread_spin_lock(lock);
		DEBUG_PRINT("get spin tid: %ld processed: %d\n", g_thread_id(), i);
		counter += 1;
		pthread_spin_unlock(lock);
	}
	return 0;
}

void* init_lock()
{
	pthread_spinlock_t* lock = (pthread_spinlock_t*)malloc(sizeof(pthread_spinlock_t));
	pthread_spin_init(lock, PTHREAD_PROCESS_PRIVATE);
	return (void*)lock;
}
#endif


#if defined(__APPLE_CC__) && defined(PTHREAD_RWLOCK_PREFER_WRITER)
#warning the implementation of pthread rwlock in mac osx is writer preferred
#undef PTHREAD_RWLOCK_PREFER_WRITER
#define PTHREAD_RWLOCK_PREFER_READER
#endif



#if defined(PTHREAD_RWLOCK_PREFER_READER) || defined(PTHREAD_RWLOCK_PREFER_WRITER)
void* reader_func(void* param)
{
	inc_thread_count();
	while (thread_count != thread_total) g_thread_yield();

	void** params = reinterpret_cast<void**>(param);
	pthread_rwlock_t* lock = (pthread_rwlock_t*) params[0];
	//int spin_times = *((int*) params[1]);
	int read_count = *((int*) params[2]);
	int junk_sum = 0;
	DEBUG_PRINT("begin reader work tid: %ld\n", g_thread_id());
	for (int i = 0; i < read_count; i++) {
		pthread_rwlock_rdlock(lock);
		DEBUG_PRINT("get spin tid: %ld processed: %d\n", g_thread_id(), i);
		junk_sum += counter;
		pthread_rwlock_unlock(lock);
	}
	return 0;
}

void* writer_func(void* param)
{
	inc_thread_count();
	while (thread_count != thread_total) g_thread_yield();

	void** params = reinterpret_cast<void**>(param);
	pthread_rwlock_t* lock = (pthread_rwlock_t*) params[0];
	//int spin_times = *((int*) params[1]);
	int write_count = *((int*) params[2]);
	DEBUG_PRINT("begin writer work tid: %ld\n", g_thread_id());
	for (int i = 0; i < write_count; i++) {
		pthread_rwlock_wrlock(lock);
		DEBUG_PRINT("get spin tid: %ld processed: %d\n", g_thread_id(), i);
		counter += 1;
		pthread_rwlock_unlock(lock);
	}
	return 0;
}

void* init_lock()
{
	pthread_rwlock_t* lock = (pthread_rwlock_t*)malloc(sizeof(pthread_rwlock_t));
#if defined(PTHREAD_RWLOCK_PREFER_READER)
	pthread_rwlock_init(lock, NULL);
#elif defined(PTHREAD_RWLOCK_PREFER_WRITER)
	pthread_rwlockattr_t attr;
	pthread_rwlockattr_init(&attr);
	pthread_rwlockattr_setkind_np (&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
	pthread_rwlock_init(lock, &attr);
	pthread_rwlockattr_destroy(&attr);
#endif
	return lock;
}
#endif

int main(int argc, char* argv[])
{
	if (argc != 4) {
		printf("usage: %s reader_num write_num count\n", argv[0]);
		return 1;
	}

	int reader_num = atoi(argv[1]);
	int writer_num = atoi(argv[2]);


	int spin_times = 16;
	int count = atoi(argv[3]);

	void* params[] = {init_lock(), &spin_times, &count};

	g_lock = g_mutex_init();

	thread_count = 0;
	thread_total = reader_num + writer_num;

	g_thread_t threads[reader_num + writer_num];

	for (int i = 0; i < reader_num; i++) {
		threads[i] = g_thread_start(reader_func, (void*) params);
	}

	for (int i = 0; i < writer_num; i++) {
		threads[i + reader_num] = g_thread_start(writer_func, (void*) params);
	}

	for (int i = 0; i < reader_num + writer_num; i++) {
		long ret;
		g_thread_join(threads[i], &ret);
	}

	printf("counter: %d expect: %d\n", counter, writer_num * count);

	g_mutex_free(g_lock);

	return 0;
}
