/*
 * t_buffer_transport.h
 *
 *  Created on: 2011-8-12
 *      Author: x_zhou
 */

#include <transport/TTransport.h>

#include "buffer.h"

namespace sax {
namespace newnew {

class TBufferTransport:
	public apache::thrift::transport::TTransport
{
public:
	TBufferTransport() : _buf(NULL), _len(0) {}
	virtual ~TBufferTransport() {}

	void set_buffer(buffer* buf, bool forread) {
		_buf = buf;
		if(!forread) {
			_need_preappend = true;
			_start_pos = buf->data_length();
		} else {
			_len = 0;	// for read
		}
	}

	buffer* get_buffer() {
		return _buf;
	}

	virtual bool isOpen() {
		return _buf != NULL;
	}

	virtual void open() {
		if(_buf != NULL) {
			_buf->clear();
			_len = 0;
		}
	}

	virtual void close() {
		if(_buf != NULL) {
			_buf->clear();
			_len = 0;
		}
	}

	virtual void flush() {
		uint32_t len = (uint32_t)(_buf->data_length() - _start_pos) - 4;
		_buf->reset();
		_buf->put(len, true);
		_buf->reset();

		_need_preappend = true;
		_start_pos = _buf->data_length();
	}

	uint32_t readAll(uint8_t* buf, uint32_t len) {
		return read_virt(buf, len);
	}

	void write(const uint8_t* buf, uint32_t len) {
		write_virt(buf, len);
	}

	uint32_t readAll_virt(uint8_t* buf, uint32_t len) {
		if(_len == 0) {
			size_t remain = _buf->remaining();
			if(remain < sizeof(_len)) return 0;
			_buf->get((uint32_t&)_len, true);
		}

		uint32_t got = (len < _len ? len : _len);
		_buf->get(buf, got);
		_len -= got;
		return got;
	}

	void write_virt(const uint8_t* buf, uint32_t len) {
		if(_need_preappend) {
			_buf->preappend(4);
			_need_preappend = false;
		}
		_buf->put(const_cast<uint8_t*>(buf), len);
	}

private:
	buffer* _buf;
	uint32_t _len;
	bool _need_preappend;
	size_t _start_pos;
};

}
} // namespace
