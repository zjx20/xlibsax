/*
 * connection_mgr.h
 *
 *  Created on: 2011-8-8
 *      Author: livingroom
 */

#ifndef CONNECTION_MGR_H_
#define CONNECTION_MGR_H_

#include "netutil.h"
#include <time.h>
#include <stdint.h>
#include <map>
#include <sax/sysutil.h>
#include <sax/c++/linklist.h>

namespace sax {

class connection_base : public epoll_ctx {
public:
    enum connection_type{
        none,
        is_listen,
        is_accept,
        is_connect
    };
    enum connection_protocol{
        TCP,UDP
    };
public:
    connection_base(int fd, epoll_mgr *em) : epoll_ctx(fd, em),
                   _conn_type(none), _last_use_time(-1), _conn_id(0),
                   _is_connected(true), _task_count(0) {}
    virtual ~connection_base() {}

    bool is_connected() {
        return true;
    }

    long add_task_count(long inc_val) {
        return g_lock_add(&_task_count, inc_val);
    }

    long get_task_count() {
        return _task_count;
    }

protected:
    virtual void handle_read_event() {};
    virtual void handle_write_event() {};

public:
    connection_type     _conn_type;
    connection_protocol _conn_protocol;
    int64_t    _last_use_time;
    uint64_t   _conn_id;
    bool       _is_connected;
    connection_base *_prev, *_next;   //just for sax/c++/linklisk

private:
    long   _task_count;
};


typedef linklist<connection_base> connection_list;
typedef std::map<uint64_t, connection_base *> connection_map;

class connection_mgr {
public:
    connection_mgr() : _gen_id(0), _use_len(0), _handler_t(NULL),
                    _handler_run_flag(false) {}
    ~connection_mgr() {}

public:
    bool init();
    bool add_connection(connection_base *conn);
    bool del_connection(connection_base *conn);

    //-- monitor thread --//
    void monitor_connections();
    void disable_monitor();
    static long monitor_connections_do(void *data);
    void check_connections();
    //-- monitor thread --//

    connection_base* get_connection(const char *ip, const int &port);
    bool add_connection_map(connection_base *conn, const char *ip, const int &port);
    int64_t generate_id(const char *ip, const int &port);
    uint32_t generate_id();

    size_t get_use_size() {return _use_len;}
private:
    void print_list(connection_list &x); //for debug

private:
    uint32_t        _gen_id;
    connection_list _use_list;
    size_t          _use_len;
    mutex_type      _conn_mtx;
    connection_map  _conn_map;
    g_thread_t    _handler_t;
    bool          _handler_run_flag;
};

} //namespace


#endif /* CONNECTION_MGR_H_ */
