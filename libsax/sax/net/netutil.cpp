/*
 * netutil.cpp
 *
 *  Created on: 2012-8-24
 *      Author: x
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "netutil.h"
#include "sax/compiler.h"
#include "sax/logger/logger.h"
#include "sax/os_api.h"

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
	if (!_inited) return;

	for (int32_t i = 0; i < _maxfds; i++) {
		if (_ctx[i].tid.seq != -1 && _ctx[i].tid.trans == this) {
			this->close(_ctx[i].tid);
		}
	}

	g_eda_close(_eda);
	_eda = NULL;

	delete _handler;
	_handler = NULL;

	if (!_cloned) {
		delete[] _ctx;
		_ctx = NULL;
	}
}

bool transport::init(int32_t maxfds, transport_handler* handler)
{
	if (_inited) return false;

	_eda = g_eda_open(maxfds, eda_callback, (void*) this);
	if (!_eda) goto init_error;

	_ctx = new (std::nothrow) context[maxfds];
	if (!_ctx) goto init_error;

	for (int32_t i = 0; i < maxfds; i++) {
		_ctx[i].tid.seq = -1;
		_ctx[i].write_buf = NULL;
		_ctx[i].read_buf = NULL;
	}

	_maxfds = maxfds;
	_handler = handler;

	_inited = true;

	return true;

init_error:
	if (_eda) g_eda_close(_eda);
	if (_ctx) delete[] _ctx;

	_eda = NULL;
	_ctx = NULL;

	return false;
}

bool transport::clone(const transport& source, transport_handler* handler)
{
	if (!source._inited || _inited) return false;

	_eda = g_eda_open(source._maxfds, eda_callback, (void*) this);
	if (!_eda) goto clone_error;

	_maxfds = source._maxfds;
	_handler = handler;
	_ctx = source._ctx;
	_inited = true;
	_cloned = true;

	return true;

clone_error:
	if (_eda) g_eda_close(_eda);

	_eda = NULL;

	return false;
}

bool transport::add_fd(int fd, int eda_mask, const char* addr, uint16_t port_h,
		context::TYPE type)
{
	uint32_t ip_n = 0;
	if (addr) g_inet_aton(addr, &ip_n);
	return add_fd(fd, eda_mask, ip_n, port_h, type);
}

bool transport::add_fd(int fd, int eda_mask, uint32_t ip_n, uint16_t port_h,
		context::TYPE type)
{
	if (UNLIKELY(fd >= _maxfds ||
			g_eda_add(_eda, fd, eda_mask) != 0)) {
		return false;
	}

	context& ctx = _ctx[fd];
	ctx.ip_n = ip_n;
	ctx.port_h = port_h;
	ctx.type = type;

	// for TCP_LISTEN or UDP_BIND, it is no necessary to use buffers.
	// but just let it waste some memory, to eliminate a branch.
	// TODO: may throw std::bad_alloc
	//ctx.write_buf = new linked_buffer();
	ctx.read_buf = new linked_buffer();

	ctx.tid.fd = fd;
	ctx.tid.seq = _seq++;
	ctx.tid.trans = this;

	// overflow
	if (UNLIKELY(_seq < 0)) _seq = 0;

	return true;
}

bool transport::listen(const char* addr, uint16_t port_h,
		int32_t backlog/* = 511*/, id& tid)
{
	int fd = g_tcp_listen(addr, port_h, backlog);
	if (fd == -1) return false;

	if (g_set_non_block(fd) == -1 ||
			!add_fd(fd, EDA_READ, addr, port_h, context::TCP_LISTEN)) {
		g_close_socket(fd);
		return false;
	}

	tid = _ctx[fd].tid;

	return true;
}

bool transport::listen_clone(const id& source)
{
	if (_cloned == true && source.trans->_ctx == _ctx &&
			_ctx[source.fd].type == context::TCP_LISTEN) {
		return g_eda_add(_eda, source.fd, EDA_READ) == 0;
	}
	return false;
}

bool transport::bind(const char* addr, uint16_t port_h, id& tid)
{
	int fd = g_udp_open(addr, port_h);
	if (fd == -1) return false;

	if (g_set_non_block(fd) == -1 ||
			!add_fd(fd, EDA_READ, addr, port_h, context::UDP_BIND)) {
		g_close_socket(fd);
		return false;
	}

	tid = _ctx[fd].tid;

	return true;
}

bool transport::bind_clone(const id& source)
{
	if (_cloned == true && source.trans->_ctx == _ctx &&
			_ctx[source.fd].type == context::UDP_BIND) {
		return g_eda_add(_eda, source.fd, EDA_READ) == 0;
	}
	return false;
}

bool transport::connect(const char* addr, uint16_t port_h, id& tid)
{
	int fd = g_tcp_connect(addr, port_h, 1 /*non block*/);
	if (fd == -1) return false;

	if (g_set_keepalive(fd, 60, 10, 3) == -1 ||	// detect errors in 90 seconds
			!add_fd(fd, EDA_READ, addr, port_h, context::TCP_CONNECTION)) {
		g_close_socket(fd);
		return false;
	}

	tid = _ctx[fd].tid;

	return true;
}

void transport::close(const id& tid)
{
	if (UNLIKELY(!(_ctx[tid.fd].tid == tid))) return;
	g_eda_del(_eda, tid.fd);

	_ctx[tid.fd].tid.seq = -1;

	delete _ctx[tid.fd].write_buf;	// delete NULL is ok
	delete _ctx[tid.fd].read_buf;
	_ctx[tid.fd].write_buf = NULL;
	_ctx[tid.fd].read_buf = NULL;

	// should close socket at last, to avoid concurrent problem
	// eg: the fd would be accepted again by other thread
	g_close_socket(tid.fd);
}

void transport::poll(uint32_t millseconds)
{
	g_eda_poll(_eda, (int) millseconds);
	// TODO: add timer
}

void transport::toggle_write(int fd, bool on/* = true*/)
{
	// -1 == 11111111111111111111111111111111
	g_eda_mod(_eda, fd, EDA_READ | (-((int) on) & EDA_WRITE));
}

void transport::eda_callback(g_eda_t* mgr, int fd, void* user_data, int mask)
{
	transport* trans = (transport*) user_data;
	context& ctx = trans->_ctx[fd];
	if (UNLIKELY(mask & EDA_ERROR)) {
		LOG_TRACE("in eda_callback() EDA_ERROR, fd: " << fd);
		id tid = ctx.tid;
		trans->close(tid);
		trans->_handler->on_closed(tid, 0);
		return;
	}

	if (mask & EDA_WRITE) {
		assert(ctx.type == context::TCP_CONNECTION);
		handle_tcp_write(trans, fd, ctx);
	}

	if (mask & EDA_READ) {
		if (ctx.type == context::TCP_LISTEN) {
			handle_tcp_accept(trans, fd);
		}
		else if (ctx.type == context::TCP_CONNECTION) {
			handle_tcp_read(trans, fd, ctx);
		}
		else if (ctx.type == context::UDP_BIND) {
			handle_udp_read(trans, fd, ctx);
		}
		else {
			assert(0);
		}
	}
}

bool transport::handle_tcp_write(transport* trans, int fd, context& ctx)
{
	/*
	 * behavior of tcp write,
	 * tested in ubuntu 12.04 Linux 3.2.0-25-generic x86_64:
	 * 1. send to a closed or broken socket, will ret > 0,
	 *    and will generate EDA_ERROR for that fd.
	 * 2. never generate EDA_WRITE if the io buffer
	 *    have more than 256000 bytes data.
	 * 3. return EAGAIN if cannot send anymore.
	 * 4. return other error if socket is broken when sending.
	 */

	linked_buffer* buf = ctx.write_buf;

	buf->flip();

	LOG_TRACE("in handle_tcp_write()," <<
			" trans: " << trans <<
			" fd: " << fd <<
			" buf->remaining(): " << buf->remaining());

	uint32_t total_send = 0;

	while(buf->remaining()) {
		uint32_t len;
		char* buf_ptr = buf->direct_get(len);

		int ret = g_tcp_write(fd, buf_ptr, len);

		LOG_TRACE("in handle_tcp_write()," <<
				" trans: " << trans <<
				" fd: " << fd <<
				" g_tcp_write(): " << ret <<
				" errno: " << errno);

		if (LIKELY(ret > 0)) {
			total_send += ret;
			buf->commit_get(buf_ptr, ret);
			if ((uint32_t) ret != len) {
				// io buffer is full, wait for next time
				break;
			}
		}
		else {
			if (UNLIKELY(errno != EAGAIN && errno != EWOULDBLOCK &&
					errno != EINTR)) {
				// error occurred when writing
				id tid = ctx.tid;
				trans->close(tid);
				trans->_handler->on_closed(tid, errno);
				return false;
			}
			else {
				// io buffer is full, wait for next time
				break;
			}
		}
		// else: io buffer is full, wait for next time
	}

	trans->_handler->on_tcp_send(ctx.tid, total_send);

	if (buf->remaining() == 0) {
		// nothing to send, turn off EDA_WRITE
		trans->toggle_write(fd, false);
	}

	buf->compact();

	return true;
}

static bool blocking_send(int fd, const char* buf, int32_t length)
{
	int32_t remaining = length;
	char* now = const_cast<char*>(buf);
	do {
		int ret = g_tcp_write(fd, now, remaining);
		if (ret >= 0) {
			remaining -= ret;
			now += ret;
		}
		else {
			if (UNLIKELY(errno != EAGAIN && errno != EWOULDBLOCK &&
					errno != EINTR)) {
				LOG_WARN("in blocking_send(), fd: " << fd <<
						" errno: " << errno << " " << strerror(errno));
				return false;
			}
			else {
				// try again
				g_thread_pause();
			}
		}
	} while (remaining > 0);

	return false;
}

bool transport::send(const id& tid, const char* buf, int32_t length)
{
	if (UNLIKELY(_ctx[tid.fd].type != context::TCP_CONNECTION ||
			!(_ctx[tid.fd].tid == tid) || length < 0)) {
		return false;
	}

	linked_buffer*& write_buf = _ctx[tid.fd].write_buf;

	if (write_buf == NULL || write_buf->position() == 0) {
		// send it directly, do not wait for eda_poll()
		int ret = g_tcp_write(tid.fd, buf, length);
		if (LIKELY(ret >= 0)) {
			if (UNLIKELY(ret < length)) {
				write_buf = new linked_buffer();	// TODO: may throw std::bad_alloc

				// put the rest of data to write buffer, send it next time
				if (UNLIKELY(write_buf->put((uint8_t*) buf + ret,
						length - ret) == false)) {
					// no memory for writing
					LOG_WARN("PERFORMANCE WARNING: cannot expand write "
							"buffer for sending data, doing a blocking send."
							" fd: " << tid.fd <<
							" write buffer capacity: " << write_buf->capacity());
					if (blocking_send(tid.fd, buf + ret, length - ret)) {
						_handler->on_tcp_send(tid, length);
						return true;
					}
					return false;
				}
				else {
					toggle_write(tid.fd, true);
				}
			}
			_handler->on_tcp_send(tid, ret);
		}
		else {
			if (UNLIKELY(errno != EAGAIN && errno != EWOULDBLOCK &&
					errno != EINTR)) {
				LOG_WARN("in send(), fd: " << tid.fd <<
						" errno: " << errno << " " << strerror(errno));
				return false;
			}
			else {
				// io buffer is full
				if (UNLIKELY(write_buf->put((uint8_t*) buf, length) == false)) {
					// no memory for writing
					LOG_WARN("PERFORMANCE WARNING: cannot expand write "
							"buffer for sending data, doing a blocking send."
							" fd: " << tid.fd <<
							" write buffer capacity: " << write_buf->capacity());
					if (blocking_send(tid.fd, buf + ret, length - ret)) {
						_handler->on_tcp_send(tid, length);
						return true;
					}
					return false;
				}
				toggle_write(tid.fd, true);
			}
		}
	}
	else {
		write_buf->put((uint8_t*) buf, length);
	}

	return true;
}

bool transport::handle_tcp_read(transport* trans, int fd, context& ctx)
{
	/*
	 * behavior of tcp read,
	 * tested in ubuntu 12.04 Linux 3.2.0-25-generic x86_64:
	 * 1. read from a empty socket, will return EAGAIN.
	 * 2. read a closed or broken socket, will ret == 0.
	 */

	const int max_recv_once = 8 * 1024;	// TODO: magic number
	linked_buffer* buf = ctx.read_buf;

	// do not read too much for one fd
	char temp[max_recv_once];

	int ret = g_tcp_read(fd, temp, sizeof(temp));

	LOG_TRACE("in handle_tcp_read()," <<
			" trans: " << trans <<
			" fd: " << fd <<
			" ret: " << ret);

	if (LIKELY(ret > 0)) {
		if (UNLIKELY(buf->put((uint8_t*) temp, ret) == false)) {
			LOG_WARN("cannot expand read buffer for receiving data." <<
					" fd: " << fd <<
					" trans: " << trans <<
					" read buffer capacity: " << buf->capacity());

			if (buf->data_length() > 0) {
				buf->flip();
				trans->_handler->on_tcp_recieved(ctx.tid, buf);
			}

			id tid = ctx.tid;
			trans->close(tid);
			trans->_handler->on_closed(tid, ENOMEM);
			return false;
		}
	}
	else if (ret == 0) {
		// connection close normally
		LOG_TRACE("connection closed normally. fd: " << fd);

		if (buf->data_length() > 0) {
			buf->flip();
			trans->_handler->on_tcp_recieved(ctx.tid, buf);
		}

		id tid = ctx.tid;
		trans->close(tid);
		trans->_handler->on_closed(tid, 0);
		return true;
	}
	else {
		if (UNLIKELY(errno != EAGAIN && errno != EWOULDBLOCK &&
				errno != EINTR)) {
			// error occurred when reading
			LOG_DEBUG("error occurred. fd: " << fd <<
					" errno: " << errno);

			if (buf->data_length() > 0) {
				buf->flip();
				trans->_handler->on_tcp_recieved(ctx.tid, buf);
			}

			id tid = ctx.tid;
			trans->close(tid);
			trans->_handler->on_closed(tid, errno);
			return false;
		}
		// else: io buffer is empty, wait for next time
	}

	buf->flip();

	trans->_handler->on_tcp_recieved(ctx.tid, buf);

	// connection may be closed by on_tcp_recieved(), so check ctx.tid.seq != -1
	if (UNLIKELY(ctx.tid.seq != -1 && buf->data_length() == linked_buffer::INVALID_VALUE)) {
		LOG_ERROR("buf->compact() must be called in on_tcp_recieved().\n");
		assert(0);
	}

	return true;
}

bool transport::handle_tcp_accept(transport* trans, int fd)
{
	const int max_accept_once = 64;	// TODO: magic number
	int remaining = max_accept_once;
	while (remaining > 0) {
		uint32_t ip_n;
		uint16_t port_h;
		int accepted_fd = g_tcp_accept(fd, &ip_n, &port_h);
		if (accepted_fd == -1) {
			if (UNLIKELY(errno != EAGAIN && errno != EWOULDBLOCK)) {
				// g_tcp_accept() never return EINTR
				LOG_ERROR("accept error in transport::handle_tcp_accept()." <<
						" errno: " << errno << " " << strerror(errno));
				return false;
			}

			// no more connection for accept
			break;
		}
		else {
			if (LIKELY(g_set_non_block(accepted_fd) != -1 &&
					g_set_keepalive(accepted_fd, 60, 10, 3) != -1 &&	// detect errors in 90 seconds
					trans->add_fd(accepted_fd, EDA_READ, ip_n, port_h,
							context::TCP_CONNECTION))) {
				trans->_handler->on_accepted(trans->_ctx[accepted_fd].tid,
						trans->_ctx[fd].tid);
			}
			else {
				// accpeted_fd exceed maxfds
				LOG_ERROR("cannot accept new connection, exceed maxfds!");
				g_close_socket(accepted_fd);
				return false;
			}
		}
		--remaining;
	}

	return true;
}

bool transport::handle_udp_read(transport* trans, int fd, context& ctx)
{
	/*
	 * behavior of udp read,
	 * tested in ubuntu 12.04 Linux 3.2.0-25-generic x86_64:
	 * 1. return EAGAIN even if io buffer is empty.
	 * 2. message is truncated if user buffer is
	 *    small than the real size.
	 */

	const int max_recv_once = 16 * 1024;	// TODO: magic number
	int remaining = max_recv_once;
	uint32_t ip_n;
	uint16_t port_h;
	char temp[MAX_UDP_PACKAGE_SIZE];

	do {
		int ret = g_udp_read(fd, temp, sizeof(temp), &ip_n, &port_h);

		if (ret > 0) {
			remaining -= ret;
			if (UNLIKELY(ret > (int) sizeof(temp))) {
				LOG_WARN("recv a large udp packet, dropped. len: " << ret);
				continue;
			}
			trans->_handler->on_udp_recieved(ctx.tid, temp, ret, ip_n, port_h);
		}
		else {
			if (errno == EAGAIN || errno == EWOULDBLOCK ||
					errno == EINTR) {
				// no more data to read
				break;
			}
			else {
				// unusual error occurred
				LOG_WARN("error occurred in transport::handle_udp_read()." <<
						" errno: " << errno << " " << strerror(errno));
				return false;
			}
		}
	} while (remaining > 0);

	return true;
}

bool transport::send_udp(const id& tid, uint32_t dest_ip_n,
		uint16_t dest_port_h, const char* buf, int32_t length)
{
	/*
	 * behavior of udp write,
	 * tested in ubuntu 12.04 Linux 3.2.0-25-generic x86_64:
	 * 1. never return EAGAIN even if io buffer is full.
	 * 2. never return any error even if the host is unreachable or
	 *    the destination port is not exists.
	 * 3. return EMSGSIZE if data length greater than 65507.
	 */

	if (UNLIKELY(_ctx[tid.fd].type != context::UDP_BIND ||
			!(_ctx[tid.fd].tid == tid) || length < 0)) {
		return false;
	}

	// send it directly
	return g_udp_write2(tid.fd, buf, length, dest_ip_n, dest_port_h) != -1;
}

bool transport::send_udp(uint32_t dest_ip_n, uint16_t dest_port_h,
		const char* buf, int32_t length)
{
	if (UNLIKELY(length < 0)) return false;
	// send it directly
	return g_udp_write2(-1, buf, length, dest_ip_n, dest_port_h) != -1;
}

} // namespace sax
