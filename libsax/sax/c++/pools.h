#ifndef __POOLS_H_2011__
#define __POOLS_H_2011__

/**
 * @file pools.h
 * @brief memory pool, allocate/deallocate memory
 *
 * @note Allocators are classes that define memory models to 
 * be used by some parts of the Standard Library, and most 
 * specifically, by STL containers.
 *
 * @author Qingshan
 * @date 2011-8-30
 */

#include <sax/sysutil.h>
#include <sax/mempool.h>

#include "nocopy.h"
#include <new>

namespace sax {

/// template of single-size pool
template <typename T>
class spool : public nocopy
{
public:
	inline spool() : _addr(NULL), _pool(NULL) {}
	
	inline ~spool() { this->destroy(); }
	
	inline void destroy() {
		if (_addr) {delete[] _addr; _addr = NULL;}
	}
	
	bool init(void *addr, size_t total)
	{
		long block_size = sizeof(T);
		long block_count = g_fsb_available(total, block_size);
		if (block_count > 0) {
			if (!addr) addr = this->_addr = new char [total];
			_pool = g_fsb_init(addr, total, block_size, block_count);
			return (_pool != NULL);
		}
		return false;
	}
	
	bool init2(void *addr, long block_count)
	{
		long block_size = sizeof(T);
		long total = g_fsb_needed(block_size, block_count);
		if (total > 0) {
			if (!addr) addr = this->_addr = new char [total];
			_pool = g_fsb_init(addr, total, block_size, block_count);
			return (_pool != NULL);
		}
		return false;
	}
	
    inline int capacity() const { return g_fsb_count(_pool); }
    
    inline int used() const { return g_fsb_used(_pool); }

    inline uint64_t get_key(T *block) const {
    	return g_fsb_getkey(_pool, block);
    }
    
    inline T *get_block(uint64_t key) const {
    	return (T *) g_fsb_getblock(_pool, key);
    }
	
	inline bool check(T *block) const {
		return (_pool && g_fsb_getkey(_pool, block)>0);
	}
	
	inline T* alloc_obj()
	{
		void *obj = this->_alloc();
		if (obj) return new (obj) T();
		return NULL;
	}
	
	template <class P1>
	inline T* alloc_obj(const P1 &p1)
	{
		void *obj = this->_alloc();
		if (obj) return new (obj) T(p1);
		return NULL;
	}

	template <class P1, class P2>
	inline T* alloc_obj(const P1 &p1, const P2 &p2)
	{
		void *obj = this->_alloc();
		if (obj) return new (obj) T(p1, p2);
		return NULL;
	}

	template <class P1, class P2, class P3>
	inline T* alloc_obj(const P1 &p1, const P2 &p2, const P3 &p3)
	{
		void *obj = this->_alloc();
		if (obj) return new (obj) T(p1, p2, p3);
		return NULL;
	}
	
	inline bool free_obj(T *obj)
	{
		obj->~T();
		auto_mutex lock(&_free_mtx);
		return (0 == g_fsb_free(_pool, obj));
	}

protected:
	inline void *_alloc() {
		if (!_pool) return NULL;
		auto_mutex lock(&_alloc_mtx);
		return g_fsb_alloc(_pool);
	}
	mutex_type _alloc_mtx;
	mutex_type _free_mtx;
	char *_addr;
	struct fsb_pool_t *_pool;
};

/// template of multiple-size pool
class mpool : public nocopy
{
public:
	inline mpool() : _addr(NULL), _pool(NULL) {
		// NOTE: it is not thread-safe
		g_slab_setup(g_shm_unit());
	}
	
	inline ~mpool() { this->destroy(); }

	inline void destroy() {
		if (_addr) {delete[] _addr; _addr = NULL;}
	}

	inline bool init(void *addr, size_t total)
	{
		if (!addr) {
			size_t s2 = g_shm_unit()*2;
			if (total < s2) total = s2;
			addr = this->_addr = new char [total];
		}
		_pool = g_slab_init(addr, total);
		return (_pool != NULL);
	}
	
	inline bool check(void *p)
	{
		return (_pool && g_slab_check(_pool, p));
	}
	
	template <typename T>
	inline T* alloc_obj()
	{
		void *obj = this->_alloc(sizeof(T));
		if (obj) return new (obj) T();
		return NULL;
	}
	
	template <typename T, class P1>
	inline T* alloc_obj(const P1 &p1)
	{
		void *obj = this->_alloc(sizeof(T));
		if (obj) return new (obj) T(p1);
		return NULL;
	}

	template <typename T, class P1, class P2>
	inline T* alloc_obj(const P1 &p1, const P2 &p2)
	{
		void *obj = this->_alloc(sizeof(T));
		if (obj) return new (obj) T(p1, p2);
		return NULL;
	}

	template <typename T, class P1, class P2, class P3>
	inline T* alloc_obj(const P1 &p1, const P2 &p2, const P3 &p3)
	{
		void *obj = this->_alloc(sizeof(T));
		if (obj) return new (obj) T(p1, p2, p3);
		return NULL;
	}

	template <typename T>
	inline void free_obj(T *obj)
	{
		obj->~T();
		auto_mutex lock(&_mtx);
		g_slab_free(_pool, obj);
	}
	
protected:
	inline void *_alloc(size_t sz) {
		if (!_pool) return NULL;
		auto_mutex lock(&_mtx);
		return g_slab_alloc(_pool, sz);
	}
	mutex_type _mtx;
	char *_addr;
	struct slab_pool_t *_pool;
};

} // namespace

#endif //__POOLS_H_2011__

