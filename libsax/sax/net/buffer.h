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

#include "sax/os_types.h"
#include "sax/compiler.h"
#include "sax/os_api.h"

namespace sax {

class buffer
{
public:
	enum
	{
		INVALID_VALUE = (uint32_t) -1
	};

	buffer() throw (std::bad_alloc)
	{
		_block_size = g_shm_unit();
		_capacity   = 0;
		_current    = 0;
		_limit      = INVALID_VALUE;
		_mark       = INVALID_VALUE;
		_buf        = NULL;

		// _block_size = 2^x
		assert(_block_size > 0 && (_block_size & (_block_size - 1)) == 0);

		_block_size_mask = _block_size - 1;

		if (!reserve(_block_size)) throw std::bad_alloc();
	}

	~buffer()
	{
		if (_buf) {
			free_buffer(_buf);
			_buf = NULL;
		}
	}

	// use with reset()
	// mark the current position
	inline void mark()
	{
		_mark = _current;
	}

	// set reading limit and return the old one
	// useful when decoding, to protect the next frame
	inline uint32_t limit(uint32_t new_limit)
	{
		if (UNLIKELY(_limit == INVALID_VALUE)) {
			return INVALID_VALUE;
		}
		assert(new_limit <= _capacity);
		uint32_t pre_limit = _limit;
		_limit = new_limit;
		return pre_limit;
	}

	// use with reset()
	// mark the current position and skip "length" bytes
	// NOTICE: do NOT assume the content of those skipped bytes
	inline bool skip(uint32_t length)
	{
		if (_limit == INVALID_VALUE) {
			// writing
			if (UNLIKELY(!reserve(_current + length))) {
				return false;
			}
		}
		else {
			// reading
			if (UNLIKELY(_limit - _current < length)) {
				return false;
			}
		}

		_mark = _current;
		_current += length;

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
		if (UNLIKELY(_mark == INVALID_VALUE)) {
			return false;
		}
		uint32_t tmp = _current;
		_current = _mark;
		_mark = tmp;
		return true;
	}

	// set this buffer to initial state
	inline void clear()
	{
		_current = 0;

		_mark = INVALID_VALUE;
		_limit = INVALID_VALUE;
	}

	// set position to zero, call reset() to restore
	inline void rewind()
	{
		_mark = _current;
		_current = 0;
	}

	// set current position as reading limit
	// in the meanwhile, set position to zero for reading
	inline bool flip()
	{
		if (UNLIKELY(_limit != INVALID_VALUE)) {
			return false;
		}
		_mark = INVALID_VALUE;
		_limit = _current;
		_current = 0;
		return true;
	}

	// drop the read data, set position to the end of data
	inline bool compact()
	{
		if (UNLIKELY(_limit == INVALID_VALUE)) {
			return false;
		}

		if (_limit != _current) {
			memmove(_buf, _buf + _current, _limit - _current);
		}

		_current = _limit - _current;
		_mark = INVALID_VALUE;
		_limit = INVALID_VALUE;

		return true;
	}

	inline uint32_t capacity()
	{
		return _capacity;
	}

	inline uint32_t position()
	{
		return _current;
	}

	// get the size of remaining data for reading
	// return INVALID_VALUE when writing
	inline uint32_t remaining()
	{
		if (UNLIKELY(_limit == INVALID_VALUE)) {
			return INVALID_VALUE;
		}
		return _limit - _current;
	}

	// get the size of data, which has been wrote
	// return INVALID_VALUE when reading
	inline uint32_t data_length()
	{
		if (_limit != INVALID_VALUE) {
			return INVALID_VALUE;
		}
		return _current;
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
			uint8_t  buf[sizeof(uint16_t)];
		} b;

		if (UNLIKELY(!get(b.buf, sizeof(b.buf)))) return false;
		if (bigendian ^ !IS_LITTLE_ENDIAN) {
			num = (uint16_t) bswap_16(b.num_type);
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
			uint8_t  buf[sizeof(uint32_t)];
		} b;

		if (UNLIKELY(!get(b.buf, sizeof(b.buf)))) return false;
		if (bigendian ^ !IS_LITTLE_ENDIAN) {
			num = (uint32_t) bswap_32(b.num_type);
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
			uint8_t  buf[sizeof(uint64_t)];
		} b;

		if (UNLIKELY(!get(b.buf, sizeof(b.buf)))) return false;
		if (bigendian ^ !IS_LITTLE_ENDIAN) {
			num = (uint64_t) bswap_64(b.num_type);
		}
		else {
			num = b.num_type;
		}

		return true;
	}

	// return false when length > remaining(), and do nothing
	inline bool get(uint8_t buf[], uint32_t length)
	{
		if (UNLIKELY(_limit == INVALID_VALUE ||
				_limit - _current < length)) {
			return false;
		}

		memcpy(buf, _buf + _current, length);
		_current += length;

		return true;
	}

	inline bool put(uint8_t num)
	{
		return put(&num, 1);
	}

	inline bool put(uint16_t num, bool bigendian = false)
	{
		if (bigendian ^ !IS_LITTLE_ENDIAN) {
			num = (uint16_t) bswap_16(num);
		}

		return put((uint8_t*) &num, 2);
	}

	inline bool put(uint32_t num, bool bigendian = false)
	{
		if (bigendian ^ !IS_LITTLE_ENDIAN) {
			num = (uint32_t) bswap_32(num);
		}

		return put((uint8_t*) &num, 4);
	}

	inline bool put(uint64_t num, bool bigendian = false)
	{
		if (bigendian ^ !IS_LITTLE_ENDIAN) {
			num = (uint64_t) bswap_64(num);
		}

		return put((uint8_t*) &num, 8);
	}

	inline bool put(uint8_t buf[], uint32_t length)
	{
		if (UNLIKELY(_limit != INVALID_VALUE ||
				!reserve(_current + length))) {
			return false;
		}

		memcpy(_buf + _current, buf, length);
		_current += length;

		return true;
	}

	inline bool peek(uint8_t& num)
	{
		uint32_t tmp = _current;
		if (UNLIKELY(!get(num))) return false;
		_current = tmp;
		return true;
	}

	inline bool peek(uint16_t& num, bool bigendian = false)
	{
		uint32_t tmp = _current;
		if (UNLIKELY(!get(num, bigendian))) return false;
		_current = tmp;
		return true;
	}

	inline bool peek(uint32_t& num, bool bigendian = false)
	{
		uint32_t tmp = _current;
		if (UNLIKELY(!get(num, bigendian))) return false;
		_current = tmp;
		return true;
	}

	inline bool peek(uint64_t& num, bool bigendian = false)
	{
		uint32_t tmp = _current;
		if (UNLIKELY(!get(num, bigendian))) return false;
		_current = tmp;
		return true;
	}

	// return the buffer pointer for direct put,
	// call commit_put() to confirm
	inline char* direct_put(uint32_t prepare_length)
	{
		if (UNLIKELY(_limit != INVALID_VALUE ||
				!reserve(_current + prepare_length))) {
			return NULL;
		}
		return _buf + _current;
	}

	// param: direct  pointer returned by direct_put()
	//        length  length of direct put bytes
	inline bool commit_put(char* direct, uint32_t length)
	{
		if (UNLIKELY(_limit != INVALID_VALUE ||
				_buf + _current != direct ||
				_current + length > _capacity)) {
			return false;
		}
		_current += length;
		return true;
	}

	// return the buffer pointer for direct get,
	// call commit_get() to confirm
	inline char* direct_get()
	{
		if (UNLIKELY(_limit == INVALID_VALUE)) {
			return NULL;
		}
		return _buf + _current;
	}

	// param: direct  pointer returned by direct_get()
	//        length  length of direct put bytes
	inline bool commit_get(char* direct, uint32_t length)
	{
		if (UNLIKELY(_limit == INVALID_VALUE ||
				_buf + _current != direct ||
				_current + length > _limit)) {
			return false;
		}
		_current += length;
		return true;
	}

private:
	// no copy
	buffer(const buffer&);
	buffer& operator= (const buffer&);

	// this is a horrible function, may cause data losing in some
	// improper situation. so user cannot call it directly.
	// use skip() and reset() instead of reserve() if you really have to.
	inline bool reserve(uint32_t size)
	{
		if (UNLIKELY(_capacity < size)) {
			uint32_t pages = size / _block_size;
			pages += (size & _block_size_mask) > 0;
			char* tmp = alloc_buffer(pages);
			if (UNLIKELY(tmp == NULL)) return false;

			// buggy:
			// 1. _buf could be NULL, but _current must be 0 at that time,
			//    so nothing will happen.
			// 2. free_buffer(NULL) is also ok because it is a wrapper of ::free().
			memcpy(tmp, _buf, _current);
			free_buffer(_buf);

			_buf = tmp;
			_capacity = pages * _block_size;
		}

		return true;
	}

	inline char* alloc_buffer(uint32_t pages)
	{
		return reinterpret_cast<char*>(g_shm_alloc_pages(pages));
	}

	inline void free_buffer(char* ptr)
	{
		g_shm_free_pages(ptr);
	}

private:
	uint32_t _current;
	uint32_t _mark;
	uint32_t _limit;		// reading limit
	uint32_t _capacity;

	uint32_t _block_size;	// system page size
	uint32_t _block_size_mask;

	char* _buf;
};

}

#endif /* _SAX_BUFFER_H_ */
