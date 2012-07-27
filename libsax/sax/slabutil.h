#ifndef SLAB_H_
#define SLAB_H_

#include <stddef.h>
#include <assert.h>
#include "os_types.h"
#include "mempool.h"
#include "sysutil.h"
#include "compiler.h"
#include "c++/linkedlist.h"

namespace sax {

class slab_t
{
public:
	slab_t(int32_t size);
	~slab_t();

	inline void shrink(double keep)
	{
		g_spin_enter(_lock, 4);
		g_xslab_shrink(_slab, keep);
		g_spin_leave(_lock);
	}

	inline void* alloc()
	{
		g_spin_enter(_lock, 4);
		void* ptr = g_xslab_alloc(_slab);
		g_spin_leave(_lock);
		return ptr;
	}

	inline void free(void* ptr)
	{
		g_spin_enter(_lock, 4);
		g_xslab_free(_slab, ptr);
		g_spin_leave(_lock);
	}

	inline int32_t get_alloc_size() { return g_xslab_alloc_size(_slab); }
	inline int32_t get_usable_amount() { return g_xslab_usable_amount(_slab); }
	inline int32_t get_shrink_amount() { return g_xslab_shrink_amount(_slab); }

private:
	g_xslab_t* _slab;
	g_spin_t* _lock;

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
	static slab_mgr* get_instance()
	{
		static slab_mgr instance;
		return &instance;
	}

	// try to shrunk every slab
	void shrink_slabs(double keep = 0.9)
	{
		auto_lock<spin_type> scoped_lock(_lock);
		slab_t* node = _slab_list.head();
		while (node != NULL) {
			node->shrink(keep);
			node = node->_next;
		}
	}

	// for test
	inline size_t get_slabs_size() {return _slabs_size;}

private:

	slab_mgr() {_slabs_size = 0;}

	void register_slab(slab_t* slab)
	{
		auto_lock<spin_type> scoped_lock(_lock);
		_slab_list.push_back(slab);
		_slabs_size += 1;
	}

	void unregister_slab(slab_t* slab)
	{
		auto_lock<spin_type> scoped_lock(_lock);
		_slab_list.erase(slab);
		_slabs_size -= 1;
	}

	spin_type _lock;
	linkedlist<slab_t> _slab_list;
	size_t _slabs_size;
};

inline slab_t::slab_t(int32_t size)
{
	_slab = g_xslab_init(size);
	_lock = g_spin_init();

	assert(_slab);
	assert(_lock);

	_next = _prev = NULL;

	slab_mgr::get_instance()->register_slab(const_cast<slab_t*>(this));
}

inline slab_t::~slab_t()
{
	slab_mgr::get_instance()->unregister_slab(const_cast<slab_t*>(this));

	g_xslab_destroy(_slab);
	g_spin_free(_lock);
	_slab = NULL;
	_lock = NULL;
}

/*********************************************************************/

#ifndef NO_SLAB_NEW

template <size_t ALLOC_SIZE>
struct slab_holder
{
	static slab_t& get_slab()
	{
		static slab_t slab(ALLOC_SIZE);
		return slab;
	}
};

template <typename T>
inline T* slab_new()
{
	slab_t& slab = slab_holder<sizeof(T)>::get_slab();
	void* ptr = slab.alloc();
	if (LIKELY(ptr)) {
		return new (ptr) T();
	}
	return NULL;
}

template <typename T, typename P1>
inline T* slab_new(P1 p1)
{
	void* ptr = slab_holder<sizeof(T)>::get_slab().alloc();
	if (LIKELY(ptr)) {
		return new (ptr) T(p1);
	}
	return NULL;
}

template <typename T, typename P1, typename P2>
inline T* slab_new(P1 p1, P2 p2)
{
	void* ptr = slab_holder<sizeof(T)>::get_slab().alloc();
	if (LIKELY(ptr)) {
		return new (ptr) T(p1, p2);
	}
	return NULL;
}

template <typename T, typename P1, typename P2, typename P3>
inline T* slab_new(P1 p1, P2 p2, P3 p3)
{
	void* ptr = slab_holder<sizeof(T)>::get_slab().alloc();
	if (LIKELY(ptr)) {
		return new (ptr) T(p1, p2, p3);
	}
	return NULL;
}

template <typename T>
inline void slab_delete(T* obj)
{
	obj->~T();
	slab_holder<sizeof(T)>::get_slab().free((void*) obj);
}

#define SLAB_NEW(type)					slab_new<type>()
#define SLAB_NEW1(type, p1)				slab_new<type>(p1)
#define SLAB_NEW2(type, p1, p2)			slab_new<type>(p1, p2)
#define SLAB_NEW3(type, p1, p2, p3)		slab_new<type>(p1, p2, p3)

// BE CAREFUL to use this macro
#define SLAB_DELETE(type, obj_ptr) slab_delete<type>(static_cast<type*>(obj_ptr))


// implement all interfaces of std::allocator in gcc4.6
// reference: http://www.cplusplus.com/reference/std/memory/allocator/
template <typename _Tp>
class slab_stl_allocator
{
public:
    typedef size_t     size_type;
    typedef ptrdiff_t  difference_type;
    typedef _Tp*       pointer;
    typedef const _Tp* const_pointer;
    typedef _Tp&       reference;
    typedef const _Tp& const_reference;
    typedef _Tp        value_type;

	template <typename _Tp1>
	struct rebind
	{typedef slab_stl_allocator<_Tp1> other;};

	slab_stl_allocator() throw() { }

	slab_stl_allocator(const slab_stl_allocator& __a) throw() { }

	template<typename _Tp1>
	slab_stl_allocator(const slab_stl_allocator<_Tp1>&) throw() { }

	~slab_stl_allocator() throw() { }

	pointer address(reference __x) const { return &__x; }

	const_pointer address(const_reference __x) const { return &__x; }

	// NB: __n is permitted to be 0.  The C++ standard says nothing
	// about what the return value is when __n == 0.
	pointer allocate(size_type __n, const void* = 0) throw()
	{
		//printf("type %s size %lu in slab_stl_allocate __n = %lu.\n", typeid(_Tp).name(),
		//		sizeof(_Tp), (size_t) __n);
		if (LIKELY(__n == 1)) {
			//printf("use slab.\n");
			return static_cast<pointer>(sax::slab_holder<sizeof(_Tp)>::get_slab().alloc());
		}

		//printf("use malloc.\n");
		return static_cast<_Tp*>(::malloc(__n * sizeof(_Tp)));
	}

	// __p is not permitted to be a null pointer.
	void deallocate(pointer __p, size_type __n) throw()
	{
		if (LIKELY(__n == 1)) {
			sax::slab_holder<sizeof(_Tp)>::get_slab().free(static_cast<void*>(__p));
		}
		else {
			::free(__p);
		}
	}

	size_type max_size() const throw()
	{ return size_t(-1) / sizeof(_Tp); }

	// _GLIBCXX_RESOLVE_LIB_DEFECTS
	// 402. wrong new expression in [some_] allocator::construct
	void construct(pointer __p, const _Tp& __val)
	{ ::new((void *)__p) _Tp(__val); }

	void destroy(pointer __p) { __p->~_Tp(); }
};

#else

#define SLAB_NEW(type) new type()
#define SLAB_NEW1(type, p1) new type(p1)
#define SLAB_NEW2(type, p1, p2) new type(p1, p2)
#define SLAB_NEW3(type, p1, p2, p3) new type(p1, p2, p3)

#define SLAB_DELETE(type, obj_ptr) delete static_cast<type*>(obj_ptr)

typedef std::allocator slab_stl_allocator;

#endif

} //namespace

#endif /* SLAB_H_ */
