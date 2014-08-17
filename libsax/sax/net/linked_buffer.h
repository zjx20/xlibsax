/*
 * linked_buffer.h
 *
 *  Created on: 2011-8-24
 *      Author: x_zhou
 */

#ifndef _SAX_LINKED_BUFFER_H_
#define _SAX_LINKED_BUFFER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "sax/os_types.h"
#include "sax/compiler.h"
#include "sax/os_api.h"

namespace sax {

template <typename T>
class _linked_list
{
public:
	_linked_list() :
		_head(NULL), _tail(NULL), _size(0) {}
	~_linked_list() {}

	inline void push_back(T* node)
	{
		if (UNLIKELY(_tail == NULL)) {
			_head = _tail = node;
			node->next = NULL;
		}
		else {
			node->next = NULL;
			_tail->next = node;
			_tail = node;
		}
		++_size;
	}

	inline T* pop_front()
	{
		if (UNLIKELY(_head == NULL)) {
			return NULL;
		}
		else {
			T* ret = _head;
			_head = _head->next;
			--_size;
			if (UNLIKELY(_head == NULL)) _tail = NULL;
			return ret;
		}
	}

	inline T* get_head()
	{
		return _head;
	}

	inline int32_t get_size()
	{
		return _size;
	}

private:
	T* _head;
	T* _tail;
	int32_t _size;
};


class linked_buffer
{

#pragma pack(push, 1)

	struct buffer_block {
		buffer_block* next;
		uint8_t buf[1];
	};

#pragma pack(pop)

	struct pos {
		inline pos() :
				position(0), remaining(0),
				block(NULL), curr_buf(NULL),
				invalid(true)
		{}

		inline pos(buffer_block* block_, uint8_t *curr_buf_, uint32_t position_,
				uint32_t remaining_, bool invalid_ = false) :
				position(position_), remaining(remaining_),
				block(block_), curr_buf(curr_buf_), invalid(invalid_)
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

		uint32_t position;
		uint32_t remaining;
		buffer_block* block;
		uint8_t* curr_buf;
		bool invalid;
	};

	typedef _linked_list<buffer_block> buf_list_type;

public:
	enum
	{
		INVALID_VALUE = (uint32_t) -1
	};

	linked_buffer() throw (std::bad_alloc)
	{
		assert((size_t) g_shm_unit() > offsetof(linked_buffer::buffer_block, buf));
		_block_size = g_shm_unit() - offsetof(linked_buffer::buffer_block, buf);

		buffer_block* first = alloc_node();
		if (UNLIKELY(first == NULL)) throw std::bad_alloc();
		_buf_list.push_back(first);

		_capacity = _block_size;
		_usable = _block_size;
		_current = pos(first, first->buf, 0, _block_size);
		_zero = _current;
	}

	~linked_buffer()
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
	inline bool skip(uint32_t length)
	{
		if (UNLIKELY(!_limit.invalid &&
				_limit.position - _current.position < length)) {
			// reading skip, no enough data to skip
			return false;
		}

		if (UNLIKELY(_limit.invalid &&
				(_usable < (_current.position + length + 1) &&
						!reserve(_current.position + length)))) {
			// writing skip, no enough space to skip
			return false;
		}

		_mark = _current;

		if(LIKELY(length < _current.remaining)) {
			// speed up
			_current.curr_buf += length;
			_current.remaining -= length;
			_current.position += length;
		}
		else {
			uint32_t remaining = length - _current.remaining;
			do {
				_current.block = _current.block->next;
				_current.curr_buf = _current.block->buf;
				_current.remaining = _block_size;

				uint32_t tmp = remaining;
				if (UNLIKELY(remaining > _current.remaining)) tmp = _current.remaining;
				remaining -= tmp;
				_current.curr_buf += tmp;
				_current.remaining -= tmp;
			} while(UNLIKELY(remaining > 0));

			_current.position += length;
		}
		return true;
	}

	// use with mark() / skip() / reset()
	// set the current position to the marked position(by calling mark() or skip()).
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
		if (_buf_list.get_size() > 1) {
			buffer_block* first_block = _buf_list.pop_front();
			buffer_block* block = NULL;
			while ((block = _buf_list.pop_front()) != NULL) {
				free_node(block);
			}
			_buf_list.push_back(first_block);
		}

		_capacity = _block_size;
		_usable = _capacity;

		_zero = pos(_buf_list.get_head(),
				_buf_list.get_head()->buf, 0, _block_size);
		_current = _zero;

		_mark.set_invalid();
		_limit.set_invalid();
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
			_zero = pos(_current.block, _current.block->buf, 0, _block_size);
			_current = _zero;
		}
		else {
			_zero = pos(_current.block, _current.curr_buf, 0, _current.remaining);
			_current = pos(_limit.block, _limit.curr_buf,
					_limit.position - _current.position, _limit.remaining);
		}

		while (UNLIKELY(_buf_list.get_head() != _zero.block)) {
			free_node(_buf_list.pop_front());
			_capacity -= _block_size;
		}

		_usable = _capacity - (_block_size - _zero.remaining);

		_mark.set_invalid();
		_limit.set_invalid();

		return true;
	}

	inline uint32_t capacity()
	{
		return _capacity;
	}

	inline uint32_t position()
	{
		return _current.position;
	}

	// get the size of remaining data for reading
	// the returning value is invalid when writing
	inline uint32_t remaining()
	{
		if (UNLIKELY(_limit.invalid)) return INVALID_VALUE;
		return _limit.position - _current.position;
	}

	// get the size of data, which has been wrote
	// the returning value is invalid when reading
	inline uint32_t data_length()
	{
		if (UNLIKELY(!_limit.invalid)) return INVALID_VALUE;
		return _current.position;
	}

	inline bool get(uint8_t& num)
	{
		return get(&num, 1);
	}

	inline bool get(uint16_t& num, bool bigendian = false)
	{
		union bytes
		{
			uint16_t num_type;
			uint8_t buf[sizeof(uint16_t)];
		} b;

		if (UNLIKELY(!get(b.buf, sizeof(b.buf)))) return false;
		if (bigendian ^ !IS_LITTLE_ENDIAN) {
			num = (uint16_t)bswap_16(b.num_type);
		}
		else {
			num = b.num_type;
		}

		return true;
	}

	inline bool get(uint32_t& num, bool bigendian = false)
	{
		union bytes
		{
			uint32_t num_type;
			uint8_t buf[sizeof(uint32_t)];
		} b;

		if (UNLIKELY(!get(b.buf, sizeof(b.buf)))) return false;
		if (bigendian ^ !IS_LITTLE_ENDIAN) {
			num = (uint32_t)bswap_32(b.num_type);
		}
		else {
			num = b.num_type;
		}

		return true;
	}

	inline bool get(uint64_t& num, bool bigendian = false)
	{
		union bytes
		{
			uint64_t num_type;
			uint8_t buf[sizeof(uint64_t)];
		} b;

		if (UNLIKELY(!get(b.buf, sizeof(b.buf)))) return false;
		if (bigendian ^ !IS_LITTLE_ENDIAN) {
			num = (uint64_t)bswap_64(b.num_type);
		}
		else {
			num = b.num_type;
		}

		return true;
	}

	// return false when length > remaining(), and do nothing
	inline bool get(uint8_t buf[], uint32_t length)
	{
		if (UNLIKELY(_limit.invalid ||
				(_limit.position - _current.position < length))) {
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
			uint32_t remaining = length - _current.remaining;

			memcpy(buf, _current.curr_buf, _current.remaining);

			do {
				_current.block = _current.block->next;
				_current.curr_buf = _current.block->buf;
				_current.remaining = _block_size;

				uint32_t tmp = remaining;
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

	inline bool get(linked_buffer& buf)
	{
		return get(buf, remaining());
	}

	// notice: buf's position will advance length bytes if succeeded
	inline bool get(linked_buffer& buf, uint32_t length)
	{
		if (UNLIKELY(_limit.invalid ||
				(_limit.position - _current.position < length))) {
			return false;
		}

		// check buf is writable
		if (UNLIKELY(!buf._limit.invalid ||
				(buf._usable < (buf._current.position + length + 1) &&
						!buf.reserve(buf._current.position + length)))) {
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
			uint32_t remaining = length - _current.remaining;

			buf.put(_current.curr_buf, _current.remaining);

			do {
				_current.block = _current.block->next;
				_current.curr_buf = _current.block->buf;
				_current.remaining = _block_size;

				uint32_t tmp = remaining;
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
		if (bigendian ^ !IS_LITTLE_ENDIAN) {
			num = (uint16_t)bswap_16(num);
		}

		return put((uint8_t*)&num, 2);
	}

	inline bool put(uint32_t num, bool bigendian = false)
	{
		if (bigendian ^ !IS_LITTLE_ENDIAN) {
			num = (uint32_t)bswap_32(num);
		}

		return put((uint8_t*)&num, 4);
	}

	inline bool put(uint64_t num, bool bigendian = false)
	{
		if (bigendian ^ !IS_LITTLE_ENDIAN) {
			num = (uint64_t)bswap_64(num);
		}

		return put((uint8_t*)&num, 8);
	}

	inline bool put(uint8_t buf[], uint32_t length)
	{
		if (UNLIKELY(!_limit.invalid ||
				(_usable < (_current.position + length + 1) &&
						!reserve(_current.position + length)))) {
			return false;
		}

		if (LIKELY(length < _current.remaining)) {
			// speed up
			memcpy(_current.curr_buf, buf, length);
			_current.curr_buf += length;
			_current.remaining -= length;
			_current.position += length;
		}
		else {
			uint32_t remaining = length - _current.remaining;
			uint8_t* buf_ptr = buf + _current.remaining;

			memcpy(_current.curr_buf, buf, _current.remaining);

			do {
				_current.block = _current.block->next;
				_current.curr_buf = _current.block->buf;
				_current.remaining = _block_size;

				uint32_t tmp = remaining;
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

	inline bool put(linked_buffer& buf)
	{
		return put(buf, buf.remaining());
	}

	// notice: buf's position will advance length bytes if succeeded
	inline bool put(linked_buffer& buf, uint32_t length)
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

	inline char* direct_get(uint32_t& limit_length)
	{
		if (UNLIKELY(_limit.invalid)) {
			return NULL;
		}

		if (_current.block == _limit.block) {
			limit_length = _limit.position - _current.position;
		}
		else {
			limit_length = _current.remaining;
		}

		return (char*) _current.curr_buf;
	}

	inline bool commit_get(char* ptr, uint32_t length)
	{
		if (UNLIKELY(_limit.invalid || _current.curr_buf != (uint8_t*) ptr)) {
			return false;
		}

		if (_current.block == _limit.block) {
			if (UNLIKELY(_current.position + length > _limit.position)) {
				return false;
			}

			_current.position += length;
			_current.curr_buf += length;
			_current.remaining -= length;
		}
		else {
			if (UNLIKELY(_current.remaining < length)) {
				return false;
			}
			else if (_current.remaining == length) {
				_current.block = _current.block->next;
				_current.curr_buf = _current.block->buf;
				_current.remaining = _block_size;
				_current.position += length;
			}
			else {
				_current.position += length;
				_current.curr_buf += length;
				_current.remaining -= length;
			}
		}

		return true;
	}

	inline bool reserve(uint32_t size)
	{
		/*
		 * there is a thick in "++size",
		 * adding a extra byte, then _current.remaining will never be zero
		 * either writing or reading.
		 */
		++size;
		while (UNLIKELY(_usable < size)) {
			buffer_block* node = alloc_node();
			if (UNLIKELY(node == NULL)) return false;
			_buf_list.push_back(node);
			_capacity += _block_size;
			_usable += _block_size;
		}

		return true;
	}

private:
	// no copy
	linked_buffer(const linked_buffer&);
	linked_buffer& operator= (const linked_buffer&);

	inline buffer_block* alloc_node()
	{
		return reinterpret_cast<buffer_block*>(g_shm_alloc_pages(1));
	}

	inline void free_node(buffer_block* node)
	{
		g_shm_free_pages(node);
	}

private:
	pos _current;
	pos _zero;
	pos _mark;
	pos _limit;		// reading limit

	uint32_t _block_size;

	uint32_t _capacity;
	uint32_t _usable;

	_linked_list<buffer_block> _buf_list;
};

}

#endif /* _SAX_LINKED_BUFFER_H_ */
