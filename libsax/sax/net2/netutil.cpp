/**
 * @file sysutil.cpp
 * @brief implement for c++ wrapper defined in os_net.h
 *
 * @author x_zhou , livingroom
 * @date 2011-8-4
 */

#include "netutil.h"

namespace sax {

bool tcp_socket::listen(const char *addr, int port, bool non_block /*= false*/) {
	if(_fd >= 0) return false;	// just listen once
	_fd = g_tcp_listen(addr, port);
	if(_fd >= 0 && non_block) {
		if(socket::set_non_block(_fd) < 0) {
			g_tcp_close(_fd);
			return false;
		}
	}
	return _fd >= 0;
}

int tcp_socket::accept(char *cli_ip, int *port) {
	if(_fd < 0) return -1;	// should call listen() first
	return g_tcp_accept(_fd, cli_ip, port);
}

bool tcp_socket::connect(const char *addr, int port, bool non_block /*= true*/) {
    if(NULL == addr) return false;
	return (_fd = g_tcp_connect(addr, port, non_block)) >= 0;
}

int tcp_socket::read(void *buf, size_t count) {
    if(_fd < 0 || NULL == buf) return -1;
    return g_tcp_read(_fd, buf, count);
}

int tcp_socket::write(const void *buf, size_t count) {
    if(_fd < 0 || NULL == buf) return -1;
    return g_tcp_write(_fd, buf, count);
}

bool udp_socket::connect(const char *addr,const int &port) {
    if(NULL == addr) return -1;
    return (_fd = g_udp_open(addr, port)) >= 0;
}

int udp_socket::read(void *buf, size_t count, char *ip, int *port) {
    if(_fd < 0 || NULL == buf || NULL == ip || NULL == port) return -1;
    return g_udp_read(_fd, buf, count, ip, port);
}

int udp_socket::write(const void *buf,const int &buf_len, const char *ip,const int &port) {
    if(_fd < 0 || NULL == buf || NULL == ip) return -1;
    return g_udp_write(_fd, buf, buf_len, ip, port);
}

bool epoll_mgr::add(const int &fd,const bool &read,const bool &write, void *data, g_eda_func *proc) {
    if(NULL == _mgr || NULL == proc) return false;
    int mask = 0;
    if(read) mask |= EDA_READ;
    if(write) mask |= EDA_WRITE;
    return g_eda_add(_mgr, fd, mask, proc, data) == 0;
}

void epoll_mgr::del(const int &fd,const bool &read,const bool &write) {
    if(NULL == _mgr) return;
    int mask = 0;
    if(read) mask |= EDA_READ;
    if(write) mask |= EDA_WRITE;
    g_eda_sub(_mgr, fd, mask);
}

void epoll_mgr::set(const int &fd,const bool &read,const bool &write) {
    if(NULL == _mgr) return;
    int mask = 0;
    if(read) mask |= EDA_READ;
    if(write) mask |= EDA_WRITE;
    g_eda_set(_mgr, fd, mask);
}

int epoll_mgr::loop(const int &msec) {
    if(NULL == _mgr) return 0;
    return g_eda_loop(_mgr, msec);
}

void epoll_mgr::call_back(g_eda_t *mgr, int fd, void *data, int mask) {
    epoll_ctx *ctx = (epoll_ctx *)data;
    if(mask & EDA_READ) {
        ctx->handle_read_event();
    }
    else if(mask & EDA_WRITE) {
        ctx->handle_write_event();
    }
}

} // namespace
