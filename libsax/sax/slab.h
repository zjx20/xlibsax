#ifndef SLAB_H_
#define SLAB_H_

/**
 * @file slab.h
 * @brief a memory holder layer between user and system
 *
 * @author X
 * @date 2012.7.13
 */

#include <stddef.h>
#include "os_types.h"
#include "sysutil.h"
#include "compiler.h"
#include "c++/linkedlist.h"

namespace sax {

class slab_t
{
public:
	slab_t(size_t size);
	~slab_t();

	void shrink(double keep);

	void* alloc();
	void free(void* ptr);

	// for test
	inline size_t get_list_length() {return _list_length;}
	inline size_t get_shrink_amount() {return _shrink_amount;}
	inline size_t get_alloc_size() {return _alloc_size;}

private:

	inline void push_front(void** node)
	{
		node[0] = _free_list_head;
		_free_list_head = node;
		_list_length += 1;
	}

	inline void** pop_front()
	{
		void** tmp = (void**) _free_list_head;
		_free_list_head = (void**) _free_list_head[0];
		_list_length -= 1;
		return tmp;
	}

	void** _free_list_head;
	spin_type _lock;
	size_t _list_length;
	size_t _shrink_amount;
	size_t _alloc_size;

	// declare for linkedlist
	friend class slab_mgr;
	friend class linkedlist<slab_t>;
	slab_t* _next;
	slab_t* _prev;
};

class slab_mgr
{
	friend class slab_t;

public:
	static slab_mgr* get_instance();

	// try to shrunk every slab
	void shrink_slabs(double keep = 0.9);

	// for test
	inline size_t get_slabs_size() {return _slabs_size;}

private:

	slab_mgr() {_slabs_size = 0;}

	void register_slab(slab_t* slab);
	void unregister_slab(slab_t* slab);

	spin_type _lock;
	linkedlist<slab_t> _slab_list;
	size_t _slabs_size;
};

/*********************************************************************/

#ifndef NO_SLAB_NEW

template <typename T>
struct slab_holder
{
	static slab_t& get_slab()
	{
		static slab_t slab(sizeof(T));
		return slab;
	}
};

template <typename T>
inline T* slab_new()
{
	slab_t& slab = slab_holder<T>::get_slab();
	void* ptr = slab.alloc();
	if (LIKELY(ptr != NULL)) {
		return new (ptr) T();
	}
	return NULL;
}

template <typename T, typename P1>
inline T* slab_new(P1 p1)
{
	void* ptr = slab_holder<T>::get_slab().alloc();
	if (LIKELY(ptr)) {
		return new (ptr) T(p1);
	}
	return NULL;
}

template <typename T, typename P1, typename P2>
inline T* slab_new(P1 p1, P2 p2)
{
	void* ptr = slab_holder<T>::get_slab().alloc();
	if (LIKELY(ptr)) {
		return new (ptr) T(p1, p2);
	}
	return NULL;
}

template <typename T, typename P1, typename P2, typename P3>
inline T* slab_new(P1 p1, P2 p2, P3 p3)
{
	void* ptr = slab_holder<T>::get_slab().alloc();
	if (LIKELY(ptr)) {
		return new (ptr) T(p1, p2, p3);
	}
	return NULL;
}

template <typename T>
inline void slab_delete(T* obj)
{
	obj->~T();
	slab_holder<T>::get_slab().free((void*) obj);
}

#define SLAB_NEW(type)					slab_new<type>()
#define SLAB_NEW1(type, p1)				slab_new<type>(p1)
#define SLAB_NEW2(type, p1, p2)			slab_new<type>(p1, p2)
#define SLAB_NEW3(type, p1, p2, p3)		slab_new<type>(p1, p2, p3)

// BE CAREFUL to use this macro
#define SLAB_DELETE(type, obj_ptr) slab_delete<type>(static_cast<type*>(obj_ptr))

#else

#define SLAB_NEW(type) new type()
#define SLAB_NEW1(type, p1) new type(p1)
#define SLAB_NEW2(type, p1, p2) new type(p1, p2)
#define SLAB_NEW3(type, p1, p2, p3) new type(p1, p2, p3)

#define SLAB_DELETE(type, obj_ptr) delete static_cast<type*>(obj_ptr)

#endif

} //namespace

#endif /* SLAB_H_ */
