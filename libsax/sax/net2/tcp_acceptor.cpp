/*
 * tcp_acceptor.cpp
 *
 *  Created on: 2011-8-5
 *      Author: livingroom
 */

#include "tcp_acceptor.h"
#include <iostream>
#include "log.h"

namespace sax {

bool tcp_acceptor::init(const char *ip, const int &port) {
     bool ret = _tcp.listen(ip, port, true);
     this->_conn_type = connection_base::is_listen;
     this->_conn_protocol = connection_base::TCP;
     this->_last_use_time = g_now_hr();
     set_fd(_tcp.get_fd());
     socket::set_non_block(get_fd());
     add_event(true, false);
     _conn_mgr->add_connection(this);
     return ret;
}

void tcp_acceptor::handle_read_event() {
    char cli_ip[20];
    int cli_port;
    while(true) {
        int fd = _tcp.accept(cli_ip, &cli_port);
        if(fd < 0) break;

        LOG(INFO, "Accept new fd : %d,ip : %s,port %d",fd, cli_ip, cli_port);

        socket::set_non_block(fd);
        tcp_connection *conn = new tcp_connection(fd, get_epoll_mgr(), _conn_mgr, _streamer);
        conn->_conn_id = _conn_mgr->generate_id();
        conn->init(connection_base::is_accept);
        conn->add_event(true, false);
    }
}

}
