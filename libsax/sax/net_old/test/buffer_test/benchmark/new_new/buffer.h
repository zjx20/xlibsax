/*
 * buffer.h
 *
 *  Created on: 2011-8-24
 *      Author: x_zhou
 */

#ifndef _SAX_BUFFER_H_
#define _SAX_BUFFER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <algorithm>

#include "sax/mempool.h"
#include "sax/os_types.h"
#include "sax/compiler.h"

/*
 * TODO:
 * 1. replace memory pool with slab.	DONE
 * 2. replace bidirectional linked list with unidirectional linked list.	FAILED but optimized
 * 3. strict aliasing error in get(int) functions.	DONE
 * 4. call reserve() before calling other functions, do not call reserve() every time
 * 5. coding style.		DONE
 * 6. check all branch, add LIKELY or UNLIKELY if necessary.	DONE
 */

#undef SWAP_BYTE
#define SWAP_BYTE(a, p1, p2) a[p1] ^= a[p2]; a[p2] ^= a[p1]; a[p1] ^= a[p2]

namespace sax {
namespace newnew {


template <typename T>
class _linked_list
{
public:
	_linked_list() :
		_head(NULL), _tail(NULL) {}
	virtual ~_linked_list() {}

	inline void push_back(T* node)
	{
		if (UNLIKELY(_tail == NULL)) {
			_head = _tail = node;
			node->next = NULL;
			node->prev = NULL;
		}
		else {
			node->next = NULL;
			node->prev = _tail;
			_tail->next = node;
			_tail = node;
		}
	}

	inline T* pop_front()
	{
		if (UNLIKELY(_head == NULL)) {
			return NULL;
		}
		else {
			T* ret = _head;
			_head = _head->next;
			if (UNLIKELY(_head != NULL)) _head->prev = NULL;
			else _tail = NULL;
			return ret;
		}
	}

	inline T* get_head()
	{
		return _head;
	}

	inline void roll(T* pos)
	{
		T* new_tail = pos->prev;
		if (new_tail == NULL) return;

		_tail->next = _head;
		_head->prev = _tail;
		new_tail->next = NULL;
		pos->prev = NULL;
		_head = pos;
		_tail = new_tail;
	}

private:
	T* _head;
	T* _tail;
};


class buffer
{

#pragma pack(push, 1)

	struct buffer_block {
		buffer_block* next;
		buffer_block* prev;
		uint8_t buf[1];
	};

#pragma pack(pop)

	struct pos {
		inline pos() :
				block(NULL), curr_buf(NULL), remaining(0),
				position(0), invalid(true)
		{}

		inline pos(buffer_block* block_, uint8_t *curr_buf_, size_t position_,
				size_t remaining_, bool invalid_ = false) :
			block(block_), curr_buf(curr_buf_), remaining(remaining_),
			position(position_), invalid(invalid_)
		{}

		inline pos(const pos& p)
		{
			block = p.block;
			curr_buf = p.curr_buf;
			position = p.position;
			remaining = p.remaining;
			invalid = p.invalid;
		}

		inline pos& operator= (const pos& p)
		{
			block = p.block;
			curr_buf = p.curr_buf;
			position = p.position;
			remaining = p.remaining;
			invalid = p.invalid;
			return *this;
		}

		inline void set_invalid()
		{
			invalid = true;
		}

		buffer_block* block;
		uint8_t *curr_buf;
		size_t remaining;
		size_t position;
		bool invalid;
	};

	typedef _linked_list<buffer_block> buf_list_type;

public:
	buffer(g_xslab_t* slab) throw (std::bad_alloc)
	{
		// block size must large than the size of buffer_block in this implement
		assert((size_t) g_xslab_alloc_size(slab) >= sizeof(buffer_block));

		_block_size = g_xslab_alloc_size(slab) - offsetof(buffer::buffer_block, buf);

		_slab = slab;

		buffer_block* first = alloc_node();
		if (first == NULL) throw std::bad_alloc();
		_buf_list.push_back(first);

		_capacity = _block_size;
		_usable = _block_size;
		_current = pos(first, first->buf, 0, _block_size);
		_zero = _current;
	}

	virtual ~buffer()
	{
		buffer_block* node = NULL;
		while((node = _buf_list.pop_front()) != NULL) {
			free_node(node);
		}
	}

	// use with reset()
	// mark the current position
	inline void mark()
	{
		_mark = _current;
	}

	// use with reset()
	// mark the current position and skip "length" bytes
	inline bool preappend(size_t length)
	{
		if (UNLIKELY(!_limit.invalid ||
				!reserve(_current.position + length))) return false;
		_mark = _current;
		if (UNLIKELY(length == 0)) {
			return true;
		}
		else if(LIKELY(length < _current.remaining)) {
			// speed up
			_current.curr_buf += length;
			_current.remaining -= length;
			_current.position += length;
		}
		else {
			size_t remaining = length;
			do {
				size_t tmp = remaining < _current.remaining ? remaining : _current.remaining;
				remaining -= tmp;
				_current.curr_buf += tmp;
				_current.remaining -= tmp;
				if (_current.remaining == 0 && _current.block->next != NULL) {
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
	inline bool reset()
	{
		if (UNLIKELY(_mark.invalid)) return false;
		pos tmp(_current);
		_current = _mark;
		_mark = tmp;
		return true;
	}

	// set this buffer to initial state
	inline void clear()
	{
		_capacity = 0;
		_usable = 0;

		_current = pos(NULL, 0, 0, false);
		_zero = _current;
		_mark.set_invalid();
		_limit.set_invalid();

		buffer_block* node = NULL;
		while ((node = _buf_list.pop_front()) != NULL) {
			free_node(node);
		}
	}

	// set position to zero, call reset() to restore
	inline void rewind()
	{
		_mark = _current;
		_current = _zero;
	}

	// set current position as reading limit
	// in the meanwhile, set position to zero for reading
	inline bool flip()
	{
		if (UNLIKELY(!_limit.invalid)) return false;
		_mark.set_invalid();
		_limit = _current;
		_current = _zero;
		return true;
	}

	// drop the read data, set position to the end of data
	inline bool compact()
	{
		if (UNLIKELY(_limit.invalid)) return false;

		if (_limit.position == _current.position) {
			_zero = pos(_buf_list.get_head(), _buf_list.get_head()->buf, 0, _block_size);
			_current = _zero;
			_usable = _capacity;
		}
		else {
			_zero = pos(_current.block, _current.curr_buf, 0, _current.remaining);
			_usable = _capacity - (_block_size - _zero.remaining);

			// fix buffer_block, move previous buffer_block to the end of _buf_list
			_buf_list.roll(_current.block);

			_current = pos(_limit.block, _limit.curr_buf,
					_limit.position - _current.position, _limit.remaining);
		}

		_mark.set_invalid();
		_limit.set_invalid();

		return true;
	}

	inline size_t capacity()
	{
		return _capacity;
	}

	inline size_t position()
	{
		return _current.position;
	}

	// get the size of remaining data for reading
	// the returning value is invalid when writing
	inline size_t remaining()
	{
		return _limit.position - _current.position;
	}

	// return remaining() > 0
	inline bool has_remaining()
	{
		return remaining() > 0;
	}

	// get the size of data, which has been wrote
	// the returning value is invalid when reading
	inline size_t data_length()
	{
		return _current.position;
	}

	// return data_length() > 0
	inline bool has_data()
	{
		return data_length() > 0;
	}

	inline bool get(uint8_t& num)
	{
		return get(&num, 1);
	}

	inline bool get(uint16_t& num, bool bigendian = false)
	{
		uint8_t buf[sizeof(num)];
		if (UNLIKELY(!get(buf, sizeof(buf)))) return false;
		if (bigendian ^ !IS_LITTLE_ENDIAN) {
			SWAP_BYTE(buf, 0, 1);
		}
		memcpy(&num, buf, sizeof(buf));	// avoid strict aliasing bug
		return true;
	}

	inline bool get(uint32_t& num, bool bigendian = false)
	{
		uint8_t buf[sizeof(num)];
		if (UNLIKELY(!get(buf, sizeof(buf)))) return false;
		if (bigendian ^ !IS_LITTLE_ENDIAN) {
			SWAP_BYTE(buf, 0, 3);
			SWAP_BYTE(buf, 1, 2);
		}
		memcpy(&num, buf, sizeof(buf));	// avoid strict aliasing bug
		return true;
	}

	inline bool get(uint64_t& num, bool bigendian = false)
	{
		uint8_t buf[sizeof(num)];
		if (UNLIKELY(!get(buf, sizeof(buf)))) return false;
		if (bigendian ^ !IS_LITTLE_ENDIAN) {
			SWAP_BYTE(buf, 0, 7);
			SWAP_BYTE(buf, 1, 6);
			SWAP_BYTE(buf, 2, 5);
			SWAP_BYTE(buf, 3, 4);
		}
		memcpy(&num, buf, sizeof(buf));	// avoid strict aliasing bug
		return true;
	}

	// return false when length > remaining(), and do nothing
	inline bool get(uint8_t buf[], size_t length)
	{
		if (UNLIKELY(_limit.invalid || (remaining() < length))) {
			return false;
		}

		if (LIKELY(length < _current.remaining)) {
			// speed up
			memcpy(buf, _current.curr_buf, length);
			_current.curr_buf += length;
			_current.remaining -= length;
			_current.position += length;
		}
		else {
			uint8_t* buf_ptr = buf + _current.remaining;
			size_t remaining = length - _current.remaining;

			memcpy(buf, _current.curr_buf, _current.remaining);

			do {
				_current.block = _current.block->next;
				_current.curr_buf = _current.block->buf;
				_current.remaining = _block_size;

				size_t tmp = remaining;
				if (UNLIKELY(remaining > _current.remaining)) tmp = _current.remaining;
				memcpy(buf_ptr, _current.curr_buf, tmp);
				buf_ptr += tmp;
				remaining -= tmp;
				_current.curr_buf += tmp;
				_current.remaining -= tmp;
			} while(remaining > 0);

			_current.position += length;
		}

		return true;
	}

	inline bool get(buffer& buf)
	{
		return get(buf, remaining());
	}

	// notice: buf's position will advance length bytes if succeeded
	inline bool get(buffer& buf, size_t length)
	{
		if (UNLIKELY(_limit.invalid || (remaining() < length))) {
			return false;
		}

		// check buf is writable
		if (UNLIKELY(!buf._limit.invalid ||
				!buf.reserve(buf._current.position + length + 1))) {
			return false;
		}

		if (LIKELY(length < _current.remaining)) {
			// speed up
			buf.put(_current.curr_buf, length);
			_current.curr_buf += length;
			_current.remaining -= length;
			_current.position += length;
		}
		else {
			size_t remaining = length - _current.remaining;

			buf.put(_current.curr_buf, _current.remaining);

			do {
				_current.block = _current.block->next;
				_current.curr_buf = _current.block->buf;
				_current.remaining = _block_size;

				size_t tmp = remaining;
				if (UNLIKELY(remaining > _current.remaining)) {
					tmp = _current.remaining;
				}
				buf.put(_current.curr_buf, tmp);
				remaining -= tmp;
				_current.curr_buf += tmp;
				_current.remaining -= tmp;
			} while(UNLIKELY(remaining > 0));

			_current.position += length;
		}

		return true;
	}

	inline bool put(uint8_t num)
	{
		return put(&num, 1);
	}

	inline bool put(uint16_t num, bool bigendian = false)
	{
		uint8_t* buf = reinterpret_cast<uint8_t*>(&num);
		if (bigendian ^ !IS_LITTLE_ENDIAN) {
			SWAP_BYTE(buf, 0, 1);
		}
		return put(buf, 2);
	}

	inline bool put(uint32_t num, bool bigendian = false)
	{
		uint8_t* buf = reinterpret_cast<uint8_t*>(&num);
		if (bigendian ^ !IS_LITTLE_ENDIAN) {
			SWAP_BYTE(buf, 0, 3);
			SWAP_BYTE(buf, 1, 2);
		}
		return put(buf, 4);
	}

	inline bool put(uint64_t num, bool bigendian = false)
	{
		uint8_t* buf = reinterpret_cast<uint8_t*>(&num);
		if (bigendian ^ !IS_LITTLE_ENDIAN) {
			SWAP_BYTE(buf, 0, 7);
			SWAP_BYTE(buf, 1, 6);
			SWAP_BYTE(buf, 2, 5);
			SWAP_BYTE(buf, 3, 4);
		}
		return put(buf, 8);
	}

	inline bool put(uint8_t buf[], size_t length)
	{
		if (UNLIKELY(!_limit.invalid ||
				!reserve(_current.position + length + 1))) return false;

		/*
		 * there is a thick in "reserve(_current.position + length + 1)",
		 * adding a extra byte, then _current.remaining will never be zero
		 * either writing or reading.
		 */

		if (LIKELY(length < _current.remaining)) {
			// speed up
			memcpy(_current.curr_buf, buf, length);
			_current.curr_buf += length;
			_current.remaining -= length;
			_current.position += length;
		}
		else {
			size_t remaining = length - _current.remaining;
			uint8_t* buf_ptr = buf + _current.remaining;

			memcpy(_current.curr_buf, buf, _current.remaining);

			do {
				_current.block = _current.block->next;
				_current.curr_buf = _current.block->buf;
				_current.remaining = _block_size;

				size_t tmp = remaining;
				if (UNLIKELY(remaining > _current.remaining)) {
					tmp = _current.remaining;
				}
				memcpy(_current.curr_buf, buf_ptr, tmp);
				buf_ptr += tmp;
				remaining -= tmp;
				_current.curr_buf += tmp;
				_current.remaining -= tmp;
			} while(UNLIKELY(remaining > 0));

			_current.position += length;
		}

		return true;
	}

	inline bool put(buffer& buf)
	{
		return put(buf, buf.remaining());
	}

	// notice: buf's position will advance length bytes if succeeded
	inline bool put(buffer& buf, size_t length)
	{
		return buf.get(*this, length);
	}

	inline bool peek(uint8_t& num)
	{
		pos tmp(_current);
		if (UNLIKELY(!get(num))) return false;
		_current = tmp;
		return true;
	}

	inline bool peek(uint16_t& num, bool bigendian = false)
	{
		pos tmp(_current);
		if (UNLIKELY(!get(num, bigendian))) return false;
		_current = tmp;
		return true;
	}

	inline bool peek(uint32_t& num, bool bigendian = false)
	{
		pos tmp(_current);
		if (UNLIKELY(!get(num, bigendian))) return false;
		_current = tmp;
		return true;
	}

	inline bool peek(uint64_t& num, bool bigendian = false)
	{
		pos tmp(_current);
		if (UNLIKELY(!get(num, bigendian))) return false;
		_current = tmp;
		return true;
	}

	inline bool reserve(size_t size)
	{
		while (UNLIKELY(_usable < size)) {
			buffer_block* node = alloc_node();
			if (UNLIKELY(node == NULL)) return false;
			_buf_list.push_back(node);
			_capacity += _block_size;
			_usable += _block_size;

//			// _current.block == NULL in the initial state of buffer
//			if (UNLIKELY(_current.block == NULL)) {
//				assert(_current.position == 0);
//				_current = pos(_buf_list.get_head(), _buf_list.get_head()->buf, 0, _block_size);
//				_zero = _current;
//				// if _mark is valid, it must be at zero position
//				if (UNLIKELY(!_mark.invalid)) _mark = _current;
//			}
		}

		return true;
	}

private:
	// no copy
	buffer(const buffer&);
	buffer& operator= (const buffer&);

	inline buffer_block* alloc_node()
	{
		return reinterpret_cast<buffer_block*>(g_xslab_alloc(_slab));
	}

	inline void free_node(buffer_block* node)
	{
		g_xslab_free(_slab, node);
	}

private:
	g_xslab_t* _slab;
	size_t _block_size;

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

#endif /* _SAX_BUFFER_H_ */
