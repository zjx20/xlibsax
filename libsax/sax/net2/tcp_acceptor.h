/*
 * tcp_acceptor.h
 *
 *  Created on: 2011-8-5
 *      Author: livingroom
 */

#include "tcp_connection.h"
#include "connection_mgr.h"

namespace sax {

class tcp_acceptor : public connection_base {
public:
    tcp_acceptor(int fd, epoll_mgr *em, connection_mgr *cm, istreamer *streamer)
                                : connection_base(fd, em), _conn_mgr(cm), _streamer(streamer) {};
    virtual ~tcp_acceptor(){};

    bool init(const char *ip, const int &port);
protected:
    virtual void handle_read_event();
    virtual void handle_write_event() {}

private:
    tcp_socket _tcp;
    connection_mgr *_conn_mgr;
    istreamer *_streamer;
};

}
