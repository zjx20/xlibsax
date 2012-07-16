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


slab_t::slab_t(size_t size)
{
	_list_length = 0;
	_total_amount = 0;
	_shrink_amount = 0;
	_alloc_size = size > sizeof(fake_node_t) ? size : sizeof(fake_node_t);
	_next = NULL;
	_prev = NULL;

	slab_mgr::get_instance()->register_slab(const_cast<slab_t*>(this));
}

slab_t::~slab_t()
{
	assert(_list_length == _total_amount);

	fake_node_t* node = _free_list.head();
	while (node != NULL) {
		fake_node_t* tmp = node;
		node = node->_next;
		::free((void*) tmp);
	}

	slab_mgr::get_instance()->unregister_slab(const_cast<slab_t*>(this));
}

void slab_t::shrink(double keep)
{
	assert(keep <= 1.0);
	auto_lock<spin_type> scoped_lock(_lock);
	_shrink_amount = _total_amount - (size_t) (_total_amount * keep);
	if (_shrink_amount > _list_length) _shrink_amount = _list_length;
}

void* slab_t::alloc()
{
	auto_lock<spin_type> scoped_lock(_lock);
	if (_list_length > 0) {
		fake_node_t* head = _free_list.head();
		_free_list.erase(head);	// TODO: implement a pop_front() for linkedlist
		_list_length -= 1;
		return (void*) head;
	}
	else {
		void* ret = malloc(_alloc_size);
		if (ret) {
			_total_amount += 1;
		}
		return ret;
	}
}

void slab_t::free(void* ptr)
{
	auto_lock<spin_type> scoped_lock(_lock);
	if (_shrink_amount > 0) {
		::free(ptr);
		_shrink_amount -= 1;
		_total_amount -= 1;
	}
	else {
		_free_list.push_front((fake_node_t*) ptr);
		_list_length += 1;
	}
}

} // namespace
