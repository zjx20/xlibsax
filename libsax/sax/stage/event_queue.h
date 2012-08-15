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

namespace sax {

class event_queue
{
private:
	struct event_header
	{
		enum state {INITIAL, ALLOCATED, COMMITTED, SKIPPED};
		volatile state block_state;
		uint32_t block_size;
	};

	friend class stage;

public:
	event_queue(uint32_t cap) throw(std::bad_alloc) :
		_buf(NULL), _lock(128), _cap(cap)
	{
		assert(_cap > sizeof(event_header));
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
		if (UNLIKELY(_cap - _free_pos <= sizeof(event_header))) {
			_free_pos = 0;
		}

		do {
			event_header::state s = ((event_header*) (_buf + _free_pos))->block_state;
			if (LIKELY(s == event_header::COMMITTED)) {
				return (event_type*) (_buf + _free_pos + sizeof(event_header));
			}
			else if (UNLIKELY(s == event_header::SKIPPED)) {
				_free_pos = 0;
				continue;
			}
			else {
				return NULL;
			}
		} while(1);

		// never reach
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
				return false;
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
	void* allocate(uint32_t length)
	{
		void* ptr = NULL;
		uint32_t new_alloc_pos(-1);

		// add header size and align memory
		uint32_t new_len = (length + sizeof(event_header) + sizeof(intptr_t) - 1) &
				(~(sizeof(intptr_t) - 1));

		if (LIKELY(_alloc_pos >= _free_pos)) {
			/*
			 * "_alloc_pos > _free_pos" means:
			 *
			 *   .......###########........
			 *         ^           ^
			 *    _free_pos   _alloc_pos
			 *
			 *   so there are two space blocks for allocation.
			 *
			 * or it is just an entire empty block, then "_free_pos == _alloc_pos".
			 *
			 * when "_free_pos == _alloc_pos", _alloc_pos may point to any position of the buffer.
			 */

			// try to allocate from the rear space block
			if (LIKELY(_cap - _alloc_pos >= new_len)) {
				ptr = _buf + _alloc_pos;
				new_alloc_pos = _alloc_pos + new_len;
			}
			// try to allocate from the front space block
			else if (LIKELY(_free_pos > new_len)) {	// can not be "_free_pos >= _new_len",
													// _alloc_pos == _free_pos means empty, not full

				// set the last block as end of 'line'
				if (_cap - _alloc_pos >= sizeof(event_header)) {
					event_header* last_block = (event_header*) (_buf + _alloc_pos);
					last_block->block_state = event_header::SKIPPED;
				}

				ptr = _buf;
				new_alloc_pos = new_len;
			}
			else {
				return NULL;
			}
		}
		else {	// _alloc_pos < _free_pos
			/*
			 * "_alloc_pos < _free_pos" means:
			 *
			 *   ###..............#########...
			 *      ^            ^         ^
			 *   _alloc_pos  _free_pos   too small to allocate
			 */

			if (_free_pos - _alloc_pos > new_len) {	// can not be "_free_pos - _alloc_pos >= new_len"
				ptr = _buf + _alloc_pos;
				new_alloc_pos = _alloc_pos + new_len;
			}
			else {
				return NULL;
			}
		}

		event_header* block = (event_header*) ptr;
		block->block_state = event_header::ALLOCATED;
		block->block_size = new_len;

		//assert(new_alloc_pos != (uint32_t) -1);
		_alloc_pos = new_alloc_pos;

		return ptr;
	}

	void free(void* ptr)
	{
		event_header* block = (event_header*) ptr;
		uint32_t length = block->block_size;

		if (LIKELY(_free_pos + length <= _cap)) {
			assert((char*) ptr - _buf == _free_pos);
			_free_pos += length;
		}
		else {
			assert(ptr == _buf);
			_free_pos = length;
		}
	}

private:
	char* _buf;
	spin_type _lock;
	uint32_t _cap;
	uint32_t _alloc_pos;
	uint32_t _free_pos;
};

} // namespace sax

#endif /* EVENT_QUEUE_H_ */
