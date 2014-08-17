#ifndef __NETUTIL_H__
#define __NETUTIL_H__

/**
 * @file netutil.h
 * @brief c++ wrapper for some objects defined in os_net.h
 *
 * @author x_zhou , livingroom
 * @date 2011-8-4
 */
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <sax/os_net.h>

#if defined(__cplusplus) || defined(c_plusplus)

namespace sax {

class socket {
public:
	inline static int net_start() {return g_net_start();}
	inline static void net_clean() {g_net_clean();}
	inline static int set_non_block(int fd) {return g_set_non_block(fd);}
	inline static int set_keepalive(int fd, int keepidle_sec, int keepintvl_sec, int keepcnt)
	{return g_set_keepalive(fd, keepidle_sec, keepintvl_sec, keepcnt);}
	// TODO: add other functions about socket...
};

class tcp_socket {
public:
	tcp_socket() : _fd(-1) {}
	tcp_socket(int fd) : _fd(fd) {}
	bool connect(const char *addr, int port, bool non_block = true);
	int read(void *buf, size_t count);
	int write(const void *buf, size_t count);
	inline void close() {if(_fd > 0) g_tcp_close(_fd);}
	inline int get_fd() {return _fd;}
	inline void set_fd(const int &fd) {_fd = fd;}

    bool listen(const char *addr, int port, bool non_block = false);
    int accept(char *cli_ip, int *port);
private:
	int _fd;
};

class udp_socket {
public:
    udp_socket() : _fd(-1) {}
    bool connect(const char *addr,const int &port);
    inline void close() {if(_fd > 0) g_udp_close(_fd);}
    int read(void *buf, size_t count, char *ip, int *port);
    int write(const void *buf,const int &buf_len, const char *ip,const int &port);
private:
    int _fd;
};

class epoll_mgr {
public:
    epoll_mgr() : _mgr(NULL) {}
    ~epoll_mgr() {if(_mgr) g_eda_close(_mgr);}
    inline bool init() {return (_mgr = g_eda_open()) != NULL;}
    bool add(const int &fd,const bool &read,const bool &write,const bool &error,void *data = NULL, g_eda_func *proc = epoll_mgr::call_back);
    void del(const int &fd,const bool &read,const bool &write,const bool &error);
    void set(const int &fd,const bool &read,const bool &write,const bool &error);
    int loop(const int &msec);
    inline void start_service(const int &msec) {while(true) { loop(msec);}}
private:
    static void call_back(g_eda_t *mgr, int fd, void *data, int mask);
private:
    g_eda_t *_mgr;
};

class epoll_ctx {
    friend class epoll_mgr;
public:
    epoll_ctx(int fd, epoll_mgr *m) : _fd(fd), _mgr(m) {}
    virtual ~epoll_ctx() {}
public:
    inline void set_fd(const int &fd) {_fd = fd;}
    inline int get_fd() {return _fd;}
    inline epoll_mgr* get_epoll_mgr() {return _mgr;}
    inline bool add_event(const bool &read,const bool &write,const bool &error) {return _mgr->add(_fd, read, write, error, this);}
    inline void del_event(const bool &read,const bool &write,const bool &error) {_mgr->del(_fd, read, write, error);}
    void set_event(const bool &read,const bool &write,const bool &error) {_mgr->set(_fd, read, write, error);}
    virtual void handle_read_event() = 0;
    virtual void handle_write_event() = 0;
    virtual void handle_error_event() = 0;
private:
    int _fd;
    epoll_mgr *_mgr;
};

} // namespace

#endif//__cplusplus

#endif /* __NETUTIL_H__ */
