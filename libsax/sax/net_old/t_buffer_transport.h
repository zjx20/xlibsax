/*
 * t_buffer_transport.h
 *
 *  Created on: 2011-8-12
 *      Author: x_zhou
 */

#include <transport/TTransport.h>

#include "buffer.h"

namespace sax {

class TBufferTransport:
	public apache::thrift::transport::TTransport
{
public:
	TBufferTransport() : _buf(NULL), _len(0) {}
	virtual ~TBufferTransport() {}

	void set_buffer(buffer* buf, bool forread) {
		_buf = buf;
		if(!forread) {
			_pos = _buf->position();	// for write
		} else {
			_len = 0;	// for read
		}
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
		sax::buffer::pos end = _buf->position();
		uint32_t len = (uint32_t)(end - _pos) - 4;
		_buf->reset();
		_buf->put(len, true);
		_buf->position(end);

		_pos = _buf->position();
	}

	uint32_t read(uint8_t* buf, uint32_t len) {
		return read_virt(buf, len);
	}

	void write(const uint8_t* buf, uint32_t len) {
		write_virt(buf, len);
	}

	uint32_t read_virt(uint8_t* buf, uint32_t len) {
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
		if(_pos == _buf->position()) {
			_buf->preappend(4);
		}
		_buf->put(const_cast<uint8_t*>(buf), len);
	}

private:
	buffer* _buf;
	uint32_t _len;
	sax::buffer::pos _pos;
};

} // namespace
