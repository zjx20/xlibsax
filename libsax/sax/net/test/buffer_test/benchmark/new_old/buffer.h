/*
 * buffer.h
 *
 *  Created on: 2011-8-24
 *      Author: x_zhou
 */

#ifndef _BUFFER_NEW_OLD_H_
#define _BUFFER_NEW_OLD_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <algorithm>

#include "sax/os_types.h"
//#include "sax/sysutil.h"

namespace sax {
namespace newold {

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
	virtual void recycle(void*& ptr) = 0;

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

	inline void recycle(void*& ptr) {
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


#undef SWAP_BYTE
#define SWAP_BYTE(a, p1, p2) a[p1] ^= a[p2]; a[p2] ^= a[p1]; a[p1] ^= a[p2]


class buffer {

#pragma pack(push, 1)

	struct buffer_block {
		buffer_block* next;
		buffer_block* prev;
		size_t buf_size;
		uint8_t buf[1];
	};

#pragma pack(pop)

	class pos {
	public:
		inline pos() : _block(NULL), _block_curr_pos(0), _position(0), _invalid(true) {}
		inline ~pos() {}

		inline pos(buffer_block* block, size_t block_curr_pos = 0, size_t position = 0, bool invalid = false) {
			_block = block;
			_block_curr_pos = block_curr_pos;
			_position = position;
			_invalid = invalid;
		}

		inline pos(const pos& p) {
			_block = p._block;
			_block_curr_pos = p._block_curr_pos;
			_position = p._position;
			_invalid = p._invalid;
		}

		inline pos& operator= (const pos& p) {
			if(&p == this) return *this;
			_block = p._block;
			_block_curr_pos = p._block_curr_pos;
			_position = p._position;
			_invalid = p._invalid;
			return *this;
		}

		inline void advance(size_t size) {
			if(_block == NULL) return;
			while(size > 0) {
				size_t tmp = _block->buf_size - _block_curr_pos;
				if(tmp > size) tmp = size;
				size -= tmp;
				_block_curr_pos += tmp;
				_position += tmp;
				if(_block_curr_pos == _block->buf_size) {
					if(_block->next == NULL) break;
					_block = _block->next;
					_block_curr_pos = 0;
				}
			}
		}

		inline void fast_advance(size_t size) {
			_block_curr_pos += size;
			_position += size;
			if(_block_curr_pos == _block->buf_size && _block->next != NULL) {
				_block = _block->next;
				_block_curr_pos = 0;
			}
		}

		inline size_t position() {
			return _position;
		}

		inline buffer_block* block() {
			return _block;
		}

		inline size_t block_curr_pos() {
			return _block_curr_pos;
		}

		inline size_t free_size() {
			return _block->buf_size - _block_curr_pos;
		}

		inline uint8_t* curr_buf() {
			return _block->buf + _block_curr_pos;
		}

		inline bool is_invalid() {
			return _invalid;
		}

		// advance to next block, if necessary
		inline void fix() {
			if((_block_curr_pos == _block->buf_size) &&
					(_block->next != NULL)) {
				_block = _block->next;
				_block_curr_pos = 0;
			}
		}

	private:
		buffer_block* _block;
		size_t _block_curr_pos;
		size_t _position;
		bool _invalid;
	};

	typedef _linked_list<buffer_block> buf_list_type;

	static uint32_t const __TEST_ENDIAN_UINT32_T__;
	static bool const IS_BIGENDIAN;

public:
	buffer(block_mempool* mem_pool)	:
		_mem_pool(mem_pool),
		_block_size(mem_pool->get_block_size() - offsetof(buffer::buffer_block, buf))
	{
		// block size must large than the size of buffer_block in this implement
		assert(mem_pool->get_block_size() >= sizeof(buffer_block));

		_capacity = 0;
		_usable = 0;

		_current = pos(NULL, 0, 0, false);
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
		if(!_limit.is_invalid() || !reserve(_current.position() + length)) return false;
		_mark = _current;
		if(length != 0) _current.advance(length);
		return true;
	}

	// use with mark() / preappend() / reset()
	// set the current position to the marked position(by calling mark() or preappend()).
	// there is a feature:
	//		while restoring the marked position, current position will be marked(same as calling mark()),
	//		so you can call reset() again to restore the position pointer.
	//		that is useful in some situation, such as length encoding.
	inline bool reset() {
		if(_mark.is_invalid()) return false;
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
		_mark = pos();
		_limit = pos();

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
		if(!_limit.is_invalid()) return false;
		if(!_mark.is_invalid()) _mark = pos();	// set _mark invalid
		_limit = _current;
		_current = _zero;
		return true;
	}

	// drop the read data, set position to the end of data
	inline bool compact() {
		if(_limit.is_invalid()) return false;

		if(_limit.position() == _current.position()) {
			_zero = pos(_buf_list.get_head(), 0, 0);
			_current = _zero;
			_usable = _capacity;
		} else {
			_zero = pos(_current.block(), _current.block_curr_pos(), 0);
			_usable = _capacity - _zero.block_curr_pos();

			// fix buffer_block, move previous buffer_block to the end of _buf_list
			_linked_list<buffer_block> tmp;
			_buf_list.split(_current.block(), tmp);
			tmp.splice(_buf_list);
			_buf_list.swap(tmp);

			_current = pos(_limit.block(), _limit.block_curr_pos(),
					_limit.position() - _current.position());
		}

		if(!_mark.is_invalid()) _mark = pos();
		_limit = pos();

		return true;
	}

	inline size_t capacity() {
		return _capacity;
	}

	inline size_t position() {
		return _current.position();
	}

	// get the size of remaining data for reading
	inline size_t remaining() {
		return _limit.is_invalid() ? 0 : (_limit.position() - _current.position());
	}

	// return remaining() > 0
	inline bool has_remaining() {
		return remaining() > 0;
	}

	// get the size of data, which has been wrote
	inline size_t data_length() {
		return _limit.is_invalid() ? _current.position() : 0;
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
		if(_limit.is_invalid() || (remaining() < length)) {
			return false;
		}

		if(length == 0) return true;	// zero length may cause problem

		// _limit is made of _current, so
		// _current is always valid because _limit is valid
		_current.fix();

		uint8_t* buf_ptr = buf;
		size_t remaining = length;
		while(remaining > 0) {
			size_t tmp = std::min(remaining, _current.free_size());
			memcpy(buf_ptr, _current.curr_buf(), tmp);
			buf_ptr += tmp;
			remaining -= tmp;
			_current.fast_advance(tmp);
		}

		return true;
	}

	inline bool get(buffer& buf) {
		return get(buf, remaining());
	}

	// notice: buf's position will advance length bytes if succeeded
	inline bool get(buffer& buf, size_t length) {
		if(_limit.is_invalid() || (remaining() < length)) {
			return false;
		}

		// check buf writable
		if(!buf._limit.is_invalid() || !buf.reserve(buf._current.position() + length)) {
			return false;
		}

		if(length == 0) return true;	// zero length may cause problem

		_current.fix();

		size_t remaining = length;
		while(remaining > 0) {
			size_t tmp = std::min(remaining, _current.free_size());
			buf.put(_current.curr_buf(), tmp);
			_current.fast_advance(tmp);
			remaining -= tmp;
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
		if(!_limit.is_invalid() || !reserve(_current.position() + length)) return false;

		if(length == 0) return true;	// _current may invalid when length == 0

		_current.fix();

		size_t remaining = length;
		uint8_t* buf_ptr = buf;
		while(remaining > 0) {
			size_t tmp = std::min(remaining, _current.free_size());
			memcpy(_current.curr_buf(), buf_ptr, tmp);
			buf_ptr += tmp;
			remaining -= tmp;
			_current.fast_advance(tmp);
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
			if(node == NULL) return false;
			_buf_list.push_back(node);
			_capacity += _block_size;
			_usable += _block_size;

			// this statement can be moved to outer 'while',
			// but in inner, there is a lower calling frequency than in outer in most cases
			if(_current.block() == NULL) {
				assert(_current.position() == 0);
				_current = pos(_buf_list.get_head(), 0, 0);
				_zero = _current;
				// if _mark is valid, it must be at zero position
				if(!_mark.is_invalid()) _mark = _current;
			}
		}

		return true;
	}

private:
	// no copy
	buffer(const buffer& a);
	buffer& operator= (const buffer& a);

	inline buffer_block* alloc_node() {
		buffer::buffer_block* node = reinterpret_cast<buffer::buffer_block*>(_mem_pool->allocate());
		if(node == NULL) return NULL;

		node->buf_size = _block_size;
		return node;
	}

	inline void free_node(buffer_block* node) {
		_mem_pool->recycle(reinterpret_cast<void*&>(node));
	}

private:
	block_mempool* _mem_pool;
	const size_t _block_size;

	size_t _capacity;
	size_t _usable;

	pos _current;
	pos _zero;
	pos _mark;
	pos _limit;		// reading limit

	_linked_list<buffer_block> _buf_list;
};


#undef SWAP_BYTE

}
}

#endif /* _BUFFER_H_ */
