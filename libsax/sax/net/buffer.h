/*
 * buffer.h
 *
 *  Created on: 2011-8-24
 *      Author: x_zhou
 */

#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <algorithm>

#include "sax/os_types.h"
//#include "sax/sysutil.h"

#ifdef __GNUC__
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#endif

#undef SWAP_BYTE
#define SWAP_BYTE(a, p1, p2) a[p1] ^= a[p2]; a[p2] ^= a[p1]; a[p1] ^= a[p2]

namespace sax {

class block_mempool {
public:
	block_mempool(size_t block_size, size_t max_holding_size = (size_t)-1) :
		_block_size(block_size), _max_holding_size(max_holding_size) {}
	virtual ~block_mempool() {}

	// allocate a memory which has the size that equal to block_size
	// the value of "size" will be fixed to the size of returning memory
	virtual void* allocate() = 0;

	// recycle the memory which was allocated by this mempool,
	// and than set ptr to NULL
	virtual void recycle(void* ptr) = 0;

	inline size_t get_block_size() {
		return _block_size;
	}

protected:
	size_t _block_size;
	size_t _max_holding_size;
};


template <typename T>
class _linked_list {
public:
	_linked_list() :
		_head(NULL), _tail(NULL) {}
	virtual ~_linked_list() {}

	inline void push_front(T* node) {
		if(_head == NULL) {
			_head = _tail = node;
			node->next = NULL;
			node->prev = NULL;
		} else {
			node->next = _head;
			node->prev = NULL;
			_head->prev = node;
			_head = node;
		}
	}

	inline void push_back(T* node) {
		if(_tail == NULL) {
			_head = _tail = node;
			node->next = NULL;
			node->prev = NULL;
		} else {
			node->next = NULL;
			node->prev = _tail;
			_tail->next = node;
			_tail = node;
		}
	}

	inline T* pop_front() {
		if(_head == NULL) {
			return NULL;
		} else {
			T* ret = _head;
			_head = _head->next;
			if(_head != NULL) _head->prev = NULL;
			else _tail = NULL;
			return ret;
		}
	}

	inline T* pop_back() {
		if(_tail == NULL) {
			return NULL;
		} else {
			T* ret = _tail;
			_tail = _tail->prev;
			if(_tail != NULL) _tail->next = NULL;
			else _head = NULL;
			return ret;
		}
	}

	// use for traverse or peek, be careful in multithreaded environment
	inline T* get_head() {
		return _head;
	}

	// use for traverse or peek, be careful in multithreaded environment
	inline T* get_tail() {
		return _tail;
	}

	inline void split(T* pos, _linked_list<T>& l) {
		l._head = pos;
		l._tail = _tail;

		_tail = l._head->prev;
		l._head->prev = NULL;

		if(_head == pos) {
			_head = NULL;
		} else {
			_tail->next = NULL;
		}
	}

	inline void splice(_linked_list<T>& l) {
		if(_head == NULL) {
			_head = l._head;
			_tail = l._tail;
			l._head = NULL;
			l._tail = NULL;
		} else if(l._head != NULL) {
			_tail->next = l._head;
			l._head->prev = _tail;
			_tail = l._tail;
			l._head = NULL;
			l._tail = NULL;
		}
	}

	inline void swap(_linked_list<T>& l) {
		T* tmp = _tail;
		_tail = l._tail;
		l._tail = tmp;

		tmp = _head;
		_head = l._head;
		l._head = tmp;
	}

private:
	T* _head;
	T* _tail;
};

// memory pool base on malloc/free operator
// notice: block_size must greater than or equal to 16 in this implement
// not thread safe
class simple_block_mempool : public block_mempool {

	struct block_node {
		block_node* next;
		block_node* prev;
	};

	typedef _linked_list<block_node> freed_list_type;

public:
	simple_block_mempool(size_t block_size, size_t max_holding_size = (size_t)-1) :
		block_mempool(block_size, max_holding_size), _holding_size(0)
	{
		assert(block_size >= sizeof(block_node));
	}

	virtual ~simple_block_mempool() {
		block_node* node;
		while((node = _freed_list.pop_front()) != NULL) {
			free(reinterpret_cast<void*>(node));
		}
	}

	inline void* allocate() {
		block_node* node = _freed_list.pop_front();

		if(node == NULL) {
			// no more freed block
			return malloc(_block_size);
		} else {
			_holding_size -= _block_size;
		}

		return reinterpret_cast<void*>(node);
	}

	inline void recycle(void* ptr) {
		if(_holding_size > _max_holding_size) {
			free(ptr);
		} else {
			_freed_list.push_back(reinterpret_cast<block_node*>(ptr));
			_holding_size += _block_size;
		}
		ptr = NULL;
	}

	// for testing
	inline size_t get_holding_size() {
		return _holding_size;
	}

private:
	// no copy
	simple_block_mempool(const simple_block_mempool& obj);
	simple_block_mempool& operator= (const simple_block_mempool& obj);

private:
	size_t _holding_size;

	freed_list_type _freed_list;
};


class buffer {

#pragma pack(push, 1)

	struct buffer_block {
		buffer_block* next;
		buffer_block* prev;
		uint8_t buf[1];
	};

#pragma pack(pop)

	struct pos {
		inline pos() : block(NULL), curr_buf(NULL), remaining(0), position(0), invalid(true)
		{}

		inline pos(buffer_block* block_, uint8_t *curr_buf_, size_t position_,
				size_t remaining_ = 0, bool invalid_ = false) :
			block(block_), curr_buf(curr_buf_), remaining(remaining_),
			position(position_), invalid(invalid_)
		{}

		inline pos(const pos& p) {
			block = p.block;
			curr_buf = p.curr_buf;
			position = p.position;
			remaining = p.remaining;
			invalid = p.invalid;
		}

		inline pos& operator= (const pos& p) {
			if(LIKELY(&p != this)) {
				block = p.block;
				curr_buf = p.curr_buf;
				position = p.position;
				remaining = p.remaining;
				invalid = p.invalid;
			}
			return *this;
		}

		inline void set_invalid() {
			invalid = true;
		}

		buffer_block* block;
		uint8_t *curr_buf;
		size_t remaining;
		size_t position;
		bool invalid;
	};

	typedef _linked_list<buffer_block> buf_list_type;

	static uint32_t const __TEST_ENDIAN_UINT32_T__;
	static bool const IS_BIGENDIAN;

public:
	buffer(block_mempool* mem_pool) {
		// block size must large than the size of buffer_block in this implement
		assert(mem_pool->get_block_size() >= sizeof(buffer_block));

		_block_size = mem_pool->get_block_size() - offsetof(buffer::buffer_block, buf);

		_mem_pool = mem_pool;

		_capacity = 0;
		_usable = 0;

		_current = pos(NULL, NULL, 0);
		_zero = _current;
	}

	virtual ~buffer() {
		buffer_block* node = NULL;
		while((node = _buf_list.pop_front()) != NULL) {
			free_node(node);
		}
	}

	// use with reset()
	// mark the current position
	inline void mark() {
		_mark = _current;
	}

	// use with reset()
	// mark the current position and skip "length" bytes
	inline bool preappend(size_t length) {
		if(UNLIKELY(!_limit.invalid || !reserve(_current.position + length))) return false;
		_mark = _current;
		if(UNLIKELY(length == 0)) {
			return true;
		}
		else if(LIKELY(length < _current.remaining)) {
			// speed up
			_current.curr_buf += length;
			_current.remaining -= length;
			_current.position += length;
		} else {
			size_t remaining = length;
			do {
				size_t tmp = remaining < _current.remaining ? remaining : _current.remaining;
				remaining -= tmp;
				_current.curr_buf += tmp;
				_current.remaining -= tmp;
				if(_current.remaining == 0 && _current.block->next != NULL) {
					_current.block = _current.block->next;
					_current.curr_buf = _current.block->buf;
					_current.remaining = _block_size;
				}
			} while(remaining > 0);

			_current.position += length;
		}
		return true;
	}

	// use with mark() / preappend() / reset()
	// set the current position to the marked position(by calling mark() or preappend()).
	// there is a feature:
	//		while restoring the marked position, current position will be marked(same as calling mark()),
	//		so you can call reset() again to restore the position pointer.
	//		that is useful in some situation, such as length encoding.
	inline bool reset() {
		if(_mark.invalid) return false;
		pos tmp(_current);
		_current = _mark;
		_mark = tmp;
		return true;
	}

	// set this buffer to initial state
	inline void clear() {
		_capacity = 0;
		_usable = 0;

		_current = pos(NULL, 0, 0, false);
		_zero = _current;
		_mark.set_invalid();
		_limit.set_invalid();

		buffer_block* node = NULL;
		while((node = _buf_list.pop_front()) != NULL) {
			free_node(node);
		}
	}

	// set position to zero, call reset() to restore
	inline void rewind() {
		_mark = _current;
		_current = _zero;
	}

	// set current position as reading limit
	// in the meanwhile, set position to zero for reading
	inline bool flip() {
		if(!_limit.invalid) return false;
		_mark.set_invalid();
		_limit = _current;
		_current = _zero;
		return true;
	}

	// drop the read data, set position to the end of data
	inline bool compact() {
		if(_limit.invalid) return false;

		if(_limit.position == _current.position) {
			_zero = pos(_buf_list.get_head(), _buf_list.get_head()->buf, 0, _block_size);
			_current = _zero;
			_usable = _capacity;
		} else {
			_zero = pos(_current.block, _current.curr_buf, 0, _current.remaining);
			_usable = _capacity - (_block_size - _zero.remaining);

			// fix buffer_block, move previous buffer_block to the end of _buf_list
			_linked_list<buffer_block> tmp;
			_buf_list.split(_current.block, tmp);
			tmp.splice(_buf_list);
			_buf_list.swap(tmp);

			_current = pos(_limit.block, _limit.curr_buf,
					_limit.position - _current.position, _limit.remaining);
		}

		_mark.set_invalid();
		_limit.set_invalid();

		return true;
	}

	inline size_t capacity() {
		return _capacity;
	}

	inline size_t position() {
		return _current.position;
	}

	// get the size of remaining data for reading
	inline size_t remaining() {
		return _limit.invalid ? 0 : (_limit.position - _current.position);
	}

	// return remaining() > 0
	inline bool has_remaining() {
		return remaining() > 0;
	}

	// get the size of data, which has been wrote
	inline size_t data_length() {
		return _limit.invalid ? _current.position : 0;
	}

	// return data_length() > 0
	inline bool has_data() {
		return data_length() > 0;
	}

	inline bool get(uint8_t& num) {
		return get(&num, 1);
	}

	inline bool get(uint16_t& num, bool bigendian = true) {
		uint8_t* buf = reinterpret_cast<uint8_t*>(&num);
		if(!get(buf, 2)) return false;
		if(bigendian ^ IS_BIGENDIAN) {
			SWAP_BYTE(buf, 0, 1);
		}
		return true;
	}

	inline bool get(uint32_t& num, bool bigendian = true) {
		uint8_t* buf = reinterpret_cast<uint8_t*>(&num);
		if(!get(buf, 4)) return false;
		if(bigendian ^ IS_BIGENDIAN) {
			SWAP_BYTE(buf, 0, 3);
			SWAP_BYTE(buf, 1, 2);
		}
		return true;
	}

	inline bool get(uint64_t& num, bool bigendian = true) {
		uint8_t* buf = reinterpret_cast<uint8_t*>(&num);
		if(!get(buf, 8)) return false;
		if(bigendian ^ IS_BIGENDIAN) {
			SWAP_BYTE(buf, 0, 7);
			SWAP_BYTE(buf, 1, 6);
			SWAP_BYTE(buf, 2, 5);
			SWAP_BYTE(buf, 3, 4);
		}
		return true;
	}

	// return false when length > remaining(), and do nothing
	inline bool get(uint8_t buf[], size_t length) {
		if(_limit.invalid || (remaining() < length)) {
			return false;
		}

		if(UNLIKELY(length == 0)) return true;	// zero length may cause problem

		// _limit is made of _current, so
		// _current is always valid because _limit is valid
		assert(_current.remaining != 0);
//		if(_current.remaining == 0) {
//			_current.block = _current.block->next;	// block->next != NULL because parameter exam passed
//			_current.curr_buf = _current.block->buf;
//			_current.remaining = _block_size;
//		}

		if(LIKELY(length < _current.remaining)) {
			// speed up
			memcpy(buf, _current.curr_buf, length);
			_current.curr_buf += length;
			_current.remaining -= length;
			_current.position += length;
		} else {
			uint8_t* buf_ptr = buf;
			size_t remaining = length;
			do {
				size_t tmp = (remaining < _current.remaining) ? remaining : _current.remaining;
				memcpy(buf_ptr, _current.curr_buf, tmp);
				buf_ptr += tmp;
				remaining -= tmp;
				_current.curr_buf += tmp;
				_current.remaining -= tmp;
				if(_current.remaining == 0 && _current.block->next != NULL) {
					_current.block = _current.block->next;
					_current.curr_buf = _current.block->buf;
					_current.remaining = _block_size;
				}
			} while(remaining > 0);

			_current.position += length;
		}

		return true;
	}

	inline bool get(buffer& buf) {
		return get(buf, remaining());
	}

	// notice: buf's position will advance length bytes if succeeded
	inline bool get(buffer& buf, size_t length) {
		if(_limit.invalid || (remaining() < length)) {
			return false;
		}

		// check buf writable
		if(!buf._limit.invalid || !buf.reserve(buf._current.position + length)) {
			return false;
		}

		if(UNLIKELY(length == 0)) return true;	// zero length may cause problem

		//_current.fix();
		assert(_current.remaining != 0);

		if(LIKELY(length < _current.remaining)) {
			// speed up
			buf.put(_current.curr_buf, length);
			_current.curr_buf += length;
			_current.remaining -= length;
			_current.position += length;
		} else {
			size_t remaining = length;
			do {
				size_t tmp = std::min(remaining, _current.remaining);
				buf.put(_current.curr_buf, tmp);
				remaining -= tmp;
				_current.curr_buf += tmp;
				_current.remaining -= tmp;
				if(_current.remaining == 0 && _current.block->next != NULL) {
					_current.block = _current.block->next;
					_current.curr_buf = _current.block->buf;
					_current.remaining = _block_size;
				}
			} while(remaining > 0);

			_current.position += length;
		}

		return true;
	}

	inline bool put(uint8_t num) {
		return put(&num, 1);
	}

	inline bool put(uint16_t num, bool bigendian = true) {
		uint8_t* buf = reinterpret_cast<uint8_t*>(&num);
		if(bigendian ^ IS_BIGENDIAN) {
			SWAP_BYTE(buf, 0, 1);
		}
		return put(buf, 2);
	}

	inline bool put(uint32_t num, bool bigendian = true) {
		uint8_t* buf = reinterpret_cast<uint8_t*>(&num);
		if(bigendian ^ IS_BIGENDIAN) {
			SWAP_BYTE(buf, 0, 3);
			SWAP_BYTE(buf, 1, 2);
		}
		return put(buf, 4);
	}

	inline bool put(uint64_t num, bool bigendian = true) {
		uint8_t* buf = reinterpret_cast<uint8_t*>(&num);
		if(bigendian ^ IS_BIGENDIAN) {
			SWAP_BYTE(buf, 0, 7);
			SWAP_BYTE(buf, 1, 6);
			SWAP_BYTE(buf, 2, 5);
			SWAP_BYTE(buf, 3, 4);
		}
		return put(buf, 8);
	}

	inline bool put(uint8_t buf[], size_t length) {
		if(!_limit.invalid || !reserve(_current.position + length)) return false;

		if(UNLIKELY(length == 0)) return true;	// _current may invalid when length == 0

		if(_current.remaining == 0) {
			_current.block = _current.block->next;	// block->next != NULL because reserve() passed
			_current.curr_buf = _current.block->buf;
			_current.remaining = _block_size;
		}

		if(LIKELY(length < _current.remaining)) {
			// speed up
			memcpy(_current.curr_buf, buf, length);
			_current.curr_buf += length;
			_current.remaining -= length;
			_current.position += length;
		} else {
			size_t remaining = length;
			uint8_t* buf_ptr = buf;
			do {
				size_t tmp = std::min(remaining, _current.remaining);
				memcpy(_current.curr_buf, buf_ptr, tmp);
				buf_ptr += tmp;
				remaining -= tmp;
				_current.curr_buf += tmp;
				_current.remaining -= tmp;
				if(_current.remaining == 0 && _current.block->next != NULL) {
					_current.block = _current.block->next;
					_current.curr_buf = _current.block->buf;
					_current.remaining = _block_size;
				}
			} while(remaining > 0);

			_current.position += length;
		}

		return true;
	}

	inline bool put(buffer& buf) {
		return put(buf, buf.remaining());
	}

	// notice: buf's position will advance length bytes if succeeded
	inline bool put(buffer& buf, size_t length) {
		return buf.get(*this, length);
	}

	inline bool peek(uint8_t& num) {
		pos tmp(_current);
		if(!get(num)) return false;
		_current = tmp;
		return true;
	}

	inline bool peek(uint16_t& num, bool bigendian = true) {
		pos tmp(_current);
		if(!get(num, bigendian)) return false;
		_current = tmp;
		return true;
	}

	inline bool peek(uint32_t& num, bool bigendian = true) {
		pos tmp(_current);
		if(!get(num, bigendian)) return false;
		_current = tmp;
		return true;
	}

	inline bool peek(uint64_t& num, bool bigendian = true) {
		pos tmp(_current);
		if(!get(num, bigendian)) return false;
		_current = tmp;
		return true;
	}

	inline bool reserve(size_t size) {
		while(_usable < size) {
			buffer_block* node = alloc_node();
			if(UNLIKELY(node == NULL)) return false;
			_buf_list.push_back(node);
			_capacity += _block_size;
			_usable += _block_size;

			// this statement can be moved to outer 'while',
			// but in inner, there is a lower calling frequency than in outer in most cases
			if(UNLIKELY(_current.block == NULL)) {
				assert(_current.position == 0);
				_current = pos(_buf_list.get_head(), _buf_list.get_head()->buf, 0, _block_size);
				_zero = _current;
				// if _mark is valid, it must be at zero position
				if(!_mark.invalid) _mark = _current;
			}
		}

		return true;
	}

private:
	// no copy
	buffer(const buffer& a);
	buffer& operator= (const buffer& a);

	inline buffer_block* alloc_node() {
		return reinterpret_cast<buffer_block*>(_mem_pool->allocate());
	}

	inline void free_node(buffer_block* node) {
		_mem_pool->recycle(reinterpret_cast<void*>(node));
	}

private:
	block_mempool* _mem_pool;
	size_t _block_size;

	size_t _capacity;
	size_t _usable;

	pos _current;
	pos _zero;
	pos _mark;
	pos _limit;		// reading limit

	_linked_list<buffer_block> _buf_list;
};


#undef LIKELY
#undef UNLIKELY
#undef SWAP_BYTE


}

#endif /* _BUFFER_H_ */
