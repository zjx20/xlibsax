/*
 * netutil.cpp
 *
 *  Created on: 2012-8-24
 *      Author: x
 */

#include <errno.h>
#include <assert.h>
#include "netutil.h"
#include "compiler.h"

namespace sax {

transport::transport()
{
	_maxfds = 0;
	_handler = NULL;
	_seq = 0;
	_ctx = NULL;
	_eda = NULL;
	_inited = false;
	_cloned = false;
}

transport::~transport()
{
	// TODO close all fd?
	if (!_inited) return;

	g_eda_close(_eda);
	_eda = NULL;

	if (!_cloned) {
		delete _handler;
		_handler = NULL;
		delete[] _ctx;
		_ctx = NULL;
	}
}

bool transport::init(int32_t maxfds, transport_handler* handler)
{
	_maxfds = maxfds;
	_handler = handler;
	_eda = g_eda_open(maxfds, _eda_callback, (void*) this);
	if (!_eda) return false;
	_ctx = new (std::nothrow) transport_context[maxfds];
	if (!_ctx) return false;
	_inited = true;

	return true;
}

bool transport::clone(const transport& source)
{
	if (!source._inited) return false;
	_maxfds = source._maxfds;
	_handler = source._handler;
	_eda = g_eda_open(_maxfds, _eda_callback, (void*) this);
	if (!_eda) return false;
	_ctx = source._ctx;
	_inited = true;
	_cloned = true;

	return true;
}

bool transport::add_fd(int fd, int eda_mask, const char* addr, uint16_t port_h)
{
	if (fd >= _maxfds) {
		// TODO: add a error log or set errno to notify user
		assert(0);	// FIXME: temporary code
		return false;
	}

	if (g_eda_add(_eda, fd, eda_mask) != 0) return false;

	transport_context& context = _ctx[fd];
	g_inet_aton(addr, &context.ip_n);
	context.port_h = port_h;
	context.type = transport_context::TCP_LISTEN;

	context.tid.fd = fd;
	context.tid.seq = _seq++;
	context.tid.trans = this;

	return false;
}

bool transport::listen(const char* addr, uint16_t port_h, transport_id& tid
		, int32_t backlog/* = 511*/)
{
	int fd = g_tcp_listen(addr, port_h, backlog);
	if (fd == -1) return false;

	if (!add_fd(fd, EDA_READ, addr, port_h)) return false;

	tid = _ctx[fd].tid;

	return true;
}

bool transport::listen_clone(const transport_id& source)
{
	if (_cloned == true && source.trans->_ctx == _ctx &&
			_ctx[source.fd].type == transport_context::TCP_LISTEN) {
		return g_eda_add(_eda, source.fd, EDA_READ) == 0;
	}
	return false;
}

bool transport::bind(const char* addr, uint16_t port_h, transport_id& tid)
{
	int fd = g_udp_open(addr, port_h);
	if (fd == -1) return false;

	if (!add_fd(fd, EDA_READ, addr, port_h)) return false;

	tid = _ctx[fd].tid;

	return true;
}

bool transport::bind_clone(const transport_id& source)
{
	if (_cloned == true && source.trans->_ctx == _ctx &&
			_ctx[source.fd].type == transport_context::UDP_BIND) {
		return g_eda_add(_eda, source.fd, EDA_READ) == 0;
	}
	return false;
}

bool transport::connect(const char* addr, uint16_t port_h, transport_id& tid)
{
	int fd = g_tcp_connect(addr, port_h, 1);
	if (fd == -1) return false;

	if (!add_fd(fd, EDA_READ, addr, port_h)) return false;

	tid = _ctx[fd].tid;

	return true;
}

void transport::close(const transport_id& tid)
{
	if (UNLIKELY(_ctx[tid.fd].tid != tid)) return;
	g_eda_del(_eda, tid.fd);
	g_close_socket(tid.fd);
}

} // namespace sax
