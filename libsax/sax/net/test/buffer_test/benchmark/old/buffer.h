/*
 * buffer.h
 *
 *  Created on: 2011-8-24
 *      Author: x_zhou
 */

#ifndef _BUFFER_OLD_H_
#define _BUFFER_OLD_H_

#include <stdlib.h>
#include <string.h>

//#include <sax/sysutil.h>

namespace sax {
namespace old {

template <typename T>
class concurrent_queue {

	struct node_type {
		T value;
		node_type* next;
	};

public:
	concurrent_queue() :
		_head(NULL), _tail(NULL), _size(0)//, _lock()
	{}

	virtual ~concurrent_queue() {
		//_lock.enter();
		node_type* t;
		while(_head != NULL) {
			t = _head;
			_head = t->next;
			delete t;
		}
		_tail = NULL;
		//_lock.leave();
	}

	bool push(const T& value) {
		node_type* node = new node_type();
		if(node == NULL) return false;
		node->next = NULL;
		node->value = value;

		//_lock.enter();

		if(_tail == NULL) {
			_head = _tail = node;
		} else {
			_tail->next = node;
			_tail = node;
		}
		_size++;

		//_lock.leave();

		return true;
	}

	bool pop(T& value) {
		//_lock.enter();
		if(_head != NULL) {
			node_type* node = _head;
			_head = node->next;
			if(_head == NULL) _tail = NULL;
			_size--;

			//_lock.leave();

			value = node->value;
			delete node;

			return true;
		} else {
			//_lock.leave();
		}

		return false;
	}

	size_t size() {
		//auto_mutex auto_lock(&_lock);
		return _size;
	}

private:
	node_type* _head;
	node_type* _tail;
	volatile size_t _size;

	//mutex_type _lock;
};

class memory_pool {
public:
	static const int16_t MAX_SIZE_LEVEL = sizeof(size_t) * 8;
	static const size_t MAX_ALLOCATE_SIZE = (size_t)1 << (MAX_SIZE_LEVEL - 1);
public:
	memory_pool(size_t max_holding_size = (size_t)-1) :
		_max_holding_size(max_holding_size)
	{}

	virtual ~memory_pool() {
		int32_t i;
		void* ptr = NULL;
		for(i = 0; i < MAX_SIZE_LEVEL; i++) {
			while(_free[i].pop(ptr)) {
				free(ptr);
			}
		}
	}

	// size will be modified to feedback the real allocated size
	void* allocate(size_t& size) {
		if(size > MAX_ALLOCATE_SIZE) return NULL;
		int16_t size_level = calc_size_level(size);
		size = (size_t)1 << size_level;
		void* ptr = NULL;
		if(!_free[size_level].pop(ptr)) {
			ptr = malloc(size);
		}
		return ptr;
	}

	bool recycle(size_t size, void* ptr) {
		int16_t size_level = calc_size_level(size);
		size_t temp = (_free[size_level].size() + 1) * ((size_t)1 << size_level);
		if(temp > _max_holding_size) {
			free(ptr);
			return false;
		} else {
			if(_free[size_level].push(ptr) == false) {
				free(ptr);
			}
		}
		return true;
	}

private:
	inline int16_t calc_size_level(size_t size) {
		int16_t size_level = 0;
		size_t temp = 1;
		while(size_level < memory_pool::MAX_SIZE_LEVEL) {
			if(temp >= size) break;
			temp <<= 1;
			size_level++;
		}
		return size_level;
	}

private:
	size_t _max_holding_size;
	concurrent_queue<void*> _free[MAX_SIZE_LEVEL];	// never reach MAX_SIZE_LEVEL for array index
};

class buffer {
public:
	typedef size_t pos;
public:
	buffer(memory_pool* mem_pool, size_t init_size = 512) {
		_buf = NULL;
		_mark = INVALID_POS;
		_position = 0;
		_limit = 0;
		_capacity = 0;
		_reading = false;
		_limited_write = false;
		_mem_pool = mem_pool;

		reserve(init_size);
	}

	virtual ~buffer() {
		if(_buf != NULL) {
			_mem_pool->recycle(_capacity, _buf);
		}
	}

private:
	// nocopyable
	buffer(const buffer& b);
	buffer& operator = (const buffer& b);

public:

	static const pos INVALID_POS = (pos)-1;

	size_t capacity() {
		return _capacity;
	}

	pos position() {
		return _position;
	}

	pos limit() {
		return _limit;
	}

	bool position(const pos& new_pos) {
		if(new_pos > _limit) return false;

		_position = new_pos;
		if (_mark != INVALID_POS && _mark > _position) {
			_mark = INVALID_POS;
		}
		return true;
	}

	bool limit(const pos& new_limit) {
		if(new_limit > _capacity) return false;
		if(!_reading) _limited_write = true;
		_limit = new_limit;
		if (_position > _limit) _position = _limit;
		if (_mark != INVALID_POS && _mark > _limit) {
			_mark = INVALID_POS;
		}
		return true;
	}

	// use with reset()
	bool mark() {
		_mark = _position;
		return true;
	}

	// use with reset()
	bool preappend(size_t length) {
		if(_reading) return false;
		if(!reserve(_position + length)) return false;
		mark();
		_position += length;
		return true;
	}

	// use with mark() / preappend()
	bool reset() {
		if (_mark != INVALID_POS) {
			_position = _mark;
			return true;
		}
		return false;
	}

	bool clear() {
		_position = 0;
		_limit = _capacity;
		_limited_write = false;
		if(_mark > _position) _mark = INVALID_POS;
		return true;
	}

	bool rewind() {
		_position = 0;
		if(_mark > _position) _mark = INVALID_POS;
		return true;
	}

	bool flip() {
		if(_reading) return false;
		_reading = true;
		_limit = _position;
		_limited_write = false;
		_position = 0;
		if(_mark > _position) _mark = INVALID_POS;
		return true;
	}

	bool compact() {
		if(!_reading) return false;
		_reading = false;
		_mark = INVALID_POS;
		size_t length = remaining();
		if(_position > 0) {
			memmove(_buf, _buf + _position, length);
		}

		_position = length;
		_limit = _capacity;
		_limited_write = false;
		return true;
	}

	// for read
	size_t remaining() {
		//if(!_reading) return 0;
		return _limit - _position;
	}

	// for read
	bool has_remaining() {
		return remaining() > 0;
	}

	bool get(uint8_t& num) {
		return get(&num, 1);
	}

	bool get(uint16_t& num, bool is_bigendian = true) {
		uint8_t buf[2];
		if(!get(buf, 2)) return false;
		if(is_bigendian) {
			num = buf[0]; num <<= 8;
			num |= buf[1];
		} else {
			num = buf[1]; num <<= 8;
			num |= buf[0];
		}
		return true;
	}

	bool get(uint32_t& num, bool is_bigendian = true) {
		uint8_t buf[4];
		if(!get(buf, 4)) return false;
		if(is_bigendian) {
			num = buf[0]; num <<= 8;
			num |= buf[1]; num <<= 8;
			num |= buf[2]; num <<= 8;
			num |= buf[3];
		} else {
			num = buf[3]; num <<= 8;
			num |= buf[2]; num <<= 8;
			num |= buf[1]; num <<= 8;
			num |= buf[0];
		}

		return true;
	}

	bool get(uint64_t& num, bool is_bigendian = true) {
		uint8_t buf[8];
		if(!get(buf, 8)) return false;
		if(is_bigendian) {
			num = buf[0]; num <<= 8;
			num |= buf[1]; num <<= 8;
			num |= buf[2]; num <<= 8;
			num |= buf[3]; num <<= 8;
			num |= buf[4]; num <<= 8;
			num |= buf[5]; num <<= 8;
			num |= buf[6]; num <<= 8;
			num |= buf[7];
		} else {
			num = buf[7]; num <<= 8;
			num |= buf[6]; num <<= 8;
			num |= buf[5]; num <<= 8;
			num |= buf[4]; num <<= 8;
			num |= buf[3]; num <<= 8;
			num |= buf[2]; num <<= 8;
			num |= buf[1]; num <<= 8;
			num |= buf[0];
		}
		return true;
	}

	// false when length > remaining(), and nothing change with _position
	bool get(uint8_t buf[], size_t length) {
		if(_buf == NULL || !_reading) return false;
		if(remaining() < length) return false;

		memmove(buf, _buf + _position, length);
		_position += length;

		return true;
	}

	bool get(buffer& buf) {
		return get(buf, remaining());
	}

	bool get(buffer& buf, size_t length) {
		return buf.put(*this, length);
	}

	bool put(uint8_t num) {
		return put(&num, 1);
	}

	bool put(uint16_t num, bool is_bigendian = true) {
		uint8_t buf[2];
		if(is_bigendian) {
			buf[0] = (num >> 8) & 0x0ff;
			buf[1] = num & 0x0ff;
		} else {
			buf[0] = num & 0x0ff;
			buf[1] = (num >> 8) & 0x0ff;
		}
		return put(buf, 2);
	}

	bool put(uint32_t num, bool is_bigendian = true) {
		uint8_t buf[4];
		if(is_bigendian) {
			buf[0] = (num >> 24) & 0x0ff;
			buf[1] = (num >> 16) & 0x0ff;
			buf[2] = (num >> 8) & 0x0ff;
			buf[3] = num & 0x0ff;
		} else {
			buf[0] = num & 0x0ff;
			buf[1] = (num >> 8) & 0x0ff;
			buf[2] = (num >> 16) & 0x0ff;
			buf[3] = (num >> 24) & 0x0ff;
		}
		return put(buf, 4);
	}

	bool put(uint64_t num, bool is_bigendian = true) {
		uint8_t buf[8];
		if(is_bigendian) {
			buf[0] = (num >> 56) & 0x0ff;
			buf[1] = (num >> 48) & 0x0ff;
			buf[2] = (num >> 40) & 0x0ff;
			buf[3] = (num >> 32) & 0x0ff;
			buf[4] = (num >> 24) & 0x0ff;
			buf[5] = (num >> 16) & 0x0ff;
			buf[6] = (num >> 8) & 0x0ff;
			buf[7] = num & 0x0ff;
		} else {
			buf[0] = num & 0x0ff;
			buf[1] = (num >> 8) & 0x0ff;
			buf[2] = (num >> 16) & 0x0ff;
			buf[3] = (num >> 24) & 0x0ff;
			buf[4] = (num >> 32) & 0x0ff;
			buf[5] = (num >> 40) & 0x0ff;
			buf[6] = (num >> 48) & 0x0ff;
			buf[7] = (num >> 56) & 0x0ff;
		}
		return put(buf, 8);
	}

	bool put(uint8_t buf[], size_t length) {
		if(_buf == NULL || _reading) return false;
		if(remaining() < length) {
			if(!_limited_write) {	// infinite capacity
				if(!reserve(_position + length)) return false;
			} else {
				return false;
			}
		}

		memmove(_buf + _position, buf, length);
		_position += length;
		return true;
	}

	bool put(buffer& buf) {
		return put(buf, buf.remaining());
	}

	bool put(buffer& buf, size_t length) {
		if(buf.remaining() < length) return false;
		if(!put(buf._buf + buf._position, length)) return false;
		buf._position += length;
		return true;
	}

	bool peek(uint8_t& num) {
		if(!get(num)) return false;
		_position--;
		return true;
	}

	bool peek(uint16_t& num, bool is_bigendian = true) {
		if(!get(num, is_bigendian)) return false;
		_position -= 2;
		return true;
	}

	bool peek(uint32_t& num, bool is_bigendian = true) {
		if(!get(num, is_bigendian)) return false;
		_position -= 4;
		return true;
	}

	bool peek(uint64_t& num, bool is_bigendian = true) {
		if(!get(num, is_bigendian)) return false;
		_position -= 8;
		return true;
	}

	bool reserve(size_t size) {
		if(_capacity >= size) return true;

		// new_size will be changed to real allocated size by allocate()
		void* ptr = _mem_pool->allocate(size);
		if(ptr == NULL) return false;

		if(_buf != NULL) {
			memmove(ptr, _buf, _position);	// XXX: maybe we should copy the whole buffer
			_mem_pool->recycle(_capacity, _buf);
		}

		_buf = (uint8_t*)ptr;
		if(_reading || !_limited_write) _limit = size;
		_capacity = size;

		return true;
	}


protected:
	uint8_t* _buf;

	pos _mark;
	pos _position;
	pos _limit;
	size_t _capacity;

	bool _reading;
	bool _limited_write;

	memory_pool* _mem_pool;
};

}
}

#endif /* _BUFFER_H_ */
