/*
 * transport.h
 *
 *  Created on: 2011-8-5
 *      Author: livingroom
 */

#ifndef TRANSPORT_H_
#define TRANSPORT_H_

#include "istreamer.h"
#include "tcp_acceptor.h"
#include "connection_mgr.h"

namespace sax {

class transport {
public:
    transport() {}
    ~transport(){}

public:
    bool init(istreamer *streamer, epoll_mgr *em, connection_mgr *cm);
    bool listen(const char *ip, const int &port);
    tcp_connection* connect(const char *ip, const int &port, const bool &is_tcp = 1);
    bool disconnect(tcp_connection* handler);

    int send(void* a_task, const char *ip, const int &port);
    void response(void* a_task, void *obj, uint32_t obj_type);

private:
    istreamer *_streamer;
    epoll_mgr *_epoll_mgr;
    connection_mgr *_conn_mgr;
};

}

#endif /* TRANSPORT_H_ */
