#include <assert.h>
#include <stdlib.h>

#include "slab.h"

namespace sax {

slab_mgr* slab_mgr::get_instance()
{
	static slab_mgr instance;
	return &instance;
}

void slab_mgr::register_slab(slab_t* slab)
{
	auto_lock<spin_type> scoped_lock(_lock);
	_slab_list.push_back(slab);
	_slabs_size += 1;
}

void slab_mgr::unregister_slab(slab_t* slab)
{
	auto_lock<spin_type> scoped_lock(_lock);
	_slab_list.erase(slab);
	_slabs_size -= 1;
}

void slab_mgr::shrink_slabs(double keep/* = 0.9*/)
{
	auto_lock<spin_type> scoped_lock(_lock);
	slab_t* node = _slab_list.head();
	while (node != NULL) {
		node->shrink(keep);
		node = node->_next;
	}
}


slab_t::slab_t(size_t size) :
		_free_list_head(NULL)
{
	_list_length = 0;
	_shrink_amount = 0;
	_alloc_size = size > sizeof(void**) ? size : sizeof(void**);
	_next = NULL;
	_prev = NULL;

	slab_mgr::get_instance()->register_slab(const_cast<slab_t*>(this));
}

slab_t::~slab_t()
{
	slab_mgr::get_instance()->unregister_slab(const_cast<slab_t*>(this));

	while (_list_length > 0) {
		::free(pop_front());
	}
}

void slab_t::shrink(double keep)
{
	assert(keep <= 1.0);
	_lock.enter();
	_shrink_amount = _list_length - (size_t) (_list_length * keep);
	_lock.leave();
}

void* slab_t::alloc()
{
	_lock.enter();
	if (_list_length > 0) {
		void* tmp = pop_front();
		_lock.leave();
		return tmp;
	}
	_lock.leave();
	return malloc(_alloc_size);
}

void slab_t::free(void* ptr)
{
	_lock.enter();
	if (LIKELY(_shrink_amount == 0)) {
		push_front((void**) ptr);
	}
	else {
		::free(ptr);
		_shrink_amount -= 1;
	}
	_lock.leave();
}

} // namespace
