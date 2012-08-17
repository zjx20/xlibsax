/*
 * event_queue.h
 *
 *  Created on: 2012-8-15
 *      Author: x
 */

#ifndef EVENT_QUEUE_H_
#define EVENT_QUEUE_H_

#include <cstdlib>
#include <cassert>
#include <new>

#include "sax/compiler.h"
#include "sax/os_types.h"
#include "sax/sysutil.h"

//template <typename type>
//type ZERO_IF_X(bool x, type value) {return x?0:value;}
#define ZERO_IF_X(x, value, type) \
	((-((type)!(x))) & (type)(value))

//template <typename type>
//type SELECT(bool x, type a, type b) {return x?a:b;}
#define SELECT_X(x, a, b, type) \
	(ZERO_IF_X(!x, a, type) | ZERO_IF_X(x, b, type))

namespace sax {

class event_queue
{
private:
	struct event_header
	{
		enum state {INITIAL, ALLOCATED, COMMITTED, SKIPPED};
		volatile state block_state;
		int32_t block_size;
	};

	friend class stage;

public:
	event_queue(int32_t cap) throw(std::bad_alloc) :
		_buf(NULL), _lock(128), _cap(cap)
	{
		assert(_cap > (int32_t) sizeof(event_header));
		_buf = new char[_cap];	// just throw std::bad_alloc() if failed

		_alloc_pos = 0;
		_free_pos = 0;

		// set a initial state to first block to prevent bad thing
		// when calling pop_event() at first
		((event_header*) _buf)->block_state = event_header::INITIAL;
	}

	~event_queue()
	{
		delete[] _buf;
		_buf = NULL;
	}

	event_type* pop_event()
	{
		if (UNLIKELY(_free_pos == _alloc_pos)) return NULL;
		if (UNLIKELY(_cap - _free_pos <= (int32_t) sizeof(event_header))) {
			_free_pos = 0;
		}

		event_header::state s = ((event_header*) (_buf + _free_pos))->block_state;
		if (LIKELY(s == event_header::COMMITTED)) {
			return (event_type*) (_buf + _free_pos + sizeof(event_header));
		}
		else if (UNLIKELY(s == event_header::SKIPPED)) {
			_free_pos = 0;
			s = ((event_header*) (_buf))->block_state;
			if (LIKELY(s == event_header::COMMITTED)) {
				return (event_type*) (_buf + sizeof(event_header));
			}
		}

		return NULL;
	}

	void destroy_event(event_type* ev)
	{
		ev->destroy();
		this->free(((char*) ev) - sizeof(event_header));
	}

	template <class EVENT_TYPE>
	EVENT_TYPE* allocate_event(bool wait = true)
	{
		_lock.enter();
		void* ptr = allocate(sizeof(EVENT_TYPE));
		_lock.leave();

		if (UNLIKELY(ptr == NULL)) {
			if (UNLIKELY(wait == false)) {
				return NULL;
			}

			do {
				_lock.enter();
				ptr = allocate(sizeof(EVENT_TYPE));
				_lock.leave();
				g_thread_sleep(0.001);	// sleep 1ms
			} while(ptr == NULL);
		}

		return new (((char*) ptr) + sizeof(event_header)) EVENT_TYPE();
	}

private:
	void* allocate(int32_t length)
	{
		void* ptr = NULL;

		// add header size and align memory
		int32_t new_len = (length + sizeof(event_header) + sizeof(intptr_t) - 1) &
				(~(sizeof(intptr_t) - 1));

		int32_t new_alloc_pos = _alloc_pos;
		int32_t local_free_pos = _free_pos;	// _free_pos may be modified by other thread

		//
		// "_alloc_pos - _free_pos > 0" says:
		//
		//   .......###########........
		//         ^           ^
		//    _free_pos   _alloc_pos
		//
		//   may allocate from the rear or the front of the buffer.
		//
		// and then "_free_pos == _alloc_pos" means empty,
		// _alloc_pos may point to any position of the buffer.
		//

		bool cond1 = (_alloc_pos - local_free_pos) >= 0;
		bool cond2 = (_cap - _alloc_pos - new_len) >= 0;

		// there are not enough space to allocate in the rear,
		// so roll back to the front.
		bool skip_flag = cond1 & !cond2;
		if (UNLIKELY(skip_flag)) new_alloc_pos = 0;

		//
		// if (cond1 & cond2) == true, then it can allocate from the rear;
		// otherwise, it try to allocate from the front.
		//
		// "(_free_pos - new_alloc_pos - new_len) > 0" handles many cases:
		//   1). if new_alloc_pos == _alloc_pos && _alloc_pos >= _free_pos,
		//       the result of "_free_pos - new_alloc_pos - length" will be negative;
		//   2). new_alloc_pos just rolled back to the zero position,
		//       trying to allocate from the front;
		//   3). if new_alloc_pos == _alloc_pos && _alloc_pos < _free_pos,
		//       just a normal case of this algorithm.
		//
		// "_alloc_pos < _free_pos" means:
		//
		//   ###..............#########...
		//      ^            ^         ^
		//   _alloc_pos  _free_pos   too small to allocate
		//
		// so "allocated" is telling whether allocation is successful or not.
		//

		bool allocated = (cond1 & cond2) | ((local_free_pos - new_alloc_pos - new_len) > 0);

		if (LIKELY(allocated)) {
			ptr = (void*)(_buf + new_alloc_pos);
			event_header* block = (event_header*) ptr;
			block->block_state = event_header::ALLOCATED;
			block->block_size = new_len;

			if (UNLIKELY(skip_flag && (_cap - _alloc_pos > (int32_t) sizeof(event_header)))) {
				block = (event_header*) (_buf + _alloc_pos);
				block->block_state = event_header::SKIPPED;
			}

			_alloc_pos = new_alloc_pos + new_len;
		}

		return ptr;
	}

	void free(void* ptr)
	{
		//assert((char*)ptr - _buf == _free_pos);
		event_header* block = (event_header*) ptr;
		_free_pos += block->block_size;
	}

private:
	char* _buf;
	spin_type _lock;
	int32_t _cap;
	int32_t _alloc_pos;
	int32_t _free_pos;
};

} // namespace sax

#undef SELECT_X
#undef ZERO_IF_X

#endif /* EVENT_QUEUE_H_ */
