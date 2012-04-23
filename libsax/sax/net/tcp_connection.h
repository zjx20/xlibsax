/*
 * tcp_connection.h
 *
 *  Created on: 2011-8-5
 *      Author: livingroom
 */

#ifndef TCP_CONNECTION_H_
#define TCP_CONNECTION_H_

#include "connection_mgr.h"
#include "task_streamer.h"

namespace sax {

class tcp_connection : public connection_base {
    friend class tcp_acceptor;
    friend class transport;
public:
    tcp_connection(int fd, epoll_mgr *em, connection_mgr *cm, istreamer *streamer)
                : connection_base(fd, em) , _tcp(fd),
    _read(dynamic_cast<sax::memory_pool*>(memory_pool_factory::get_instance())),
    _write(dynamic_cast<sax::memory_pool*>(memory_pool_factory::get_instance())),
    _conn_mgr(cm), _streamer(streamer)
    {};
    virtual ~tcp_connection(){};

protected:
    bool init(const connection_type &type);
    virtual void handle_read_event();
    virtual void handle_write_event();
    virtual void handle_error_event();

public:
    //api
    int send(buffer &data);
    bool disconnect();

    buffer& get_write_buffer() {return _write;}

private:
    int try_send(uint32_t lenth);

private:
    static int max_buffer_size;

private:
    tcp_socket _tcp;
    buffer _read, _write;
    connection_mgr *_conn_mgr;
    istreamer *_streamer;

};

}

#endif /* TCP_CONNECTION_H_ */
