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
                   _conn_type(none), _last_use_time(-1), _conn_id(0) {}
    virtual ~connection_base() {}

    bool is_connected() {
        return true;
    }

protected:
    virtual void handle_read_event() {};
    virtual void handle_write_event() {};

public:
    connection_type     _conn_type;
    connection_protocol _conn_protocol;
    int64_t    _last_use_time;
    uint64_t   _conn_id;
    connection_base *_prev, *_next;   //just for sax/c++/linklisk
};


typedef linklist<connection_base> connection_list;
typedef std::map<int64_t, connection_base *> connection_map;

class connection_mgr {
public:
    connection_mgr() : _use_len(0), _mutex(NULL) {}
    ~connection_mgr() {}

public:
    bool init();
    bool add_connection(connection_base *conn);
    bool del_connection(connection_base *conn);
    void monitor_connections();
    static void monitor_connections_do();

    connection_base* get_connection(const char *ip, const int &port);
    bool add_connection_map(connection_base *conn, const char *ip, const int &port);
    int64_t generate_id(const char *ip, const int &port);

    size_t get_use_size() {return _use_len;}
private:
    //for test
    void print_list(connection_list &x);

private:
//    int64_t _gen_id;
    connection_list _use_list;
    connection_map  _conn_map;
    size_t          _use_len;
    g_mutex_t      *_mutex;
};

} //namespace


#endif /* CONNECTION_MGR_H_ */
