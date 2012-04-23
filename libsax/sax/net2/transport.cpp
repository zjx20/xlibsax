/*
 * transport.cpp
 *
 *  Created on: 2011-8-5
 *      Author: livingroom
 */
#include "transport.h"
#include <iostream>
#include "log.h"

namespace sax {

bool transport::init(istreamer *streamer, epoll_mgr *em, connection_mgr *cm) {
    _epoll_mgr = em;
    _conn_mgr = cm;
    _streamer = streamer;
    return true;
}

bool transport::listen(const char *ip, const int &port) {

    tcp_acceptor *acc = new tcp_acceptor(-1, _epoll_mgr, _conn_mgr, _streamer);
    if(!acc->init(ip,port))
    {
        delete acc;
        return false;
    }
    LOG(INFO, "listening to ip : %s, port : %d",ip, port);

    return true;
}

tcp_connection* transport::connect(const char *ip, const int &port, const bool &is_tcp/* = 1*/) {
    if(is_tcp) {
        tcp_socket tcp;
        if(!tcp.connect(ip, port, true)) {
            LOG(ERROR,"cannot connect to ip : %s,port : %d", ip , port);
            return NULL;
        }
        tcp_connection *conn = new tcp_connection(tcp.get_fd(), _epoll_mgr, _conn_mgr, _streamer);
        conn->_conn_id = _conn_mgr->generate_id(ip, port);
        if(!conn->init(connection_base::is_connect)) {
            delete conn;
            LOG(ERROR,"connection init fail...");
            return NULL;
        }
        conn->add_event(true, true);
        return conn;
    }
    return NULL;
}

bool transport::disconnect(tcp_connection* handler) {
    if(NULL == handler || !handler->disconnect()) {
        return false;
    }
    return true;
}

int transport::send(void* a_task, const char *ip, const int &port) {
    tcp_connection *conn = (tcp_connection*)_conn_mgr->get_connection(ip, port);

    if(NULL == conn) { // new conn
        LOG(INFO,"creating new connection for ip : %s, port : %d", ip, port);
        conn = connect(ip, port);
        if(!_conn_mgr->add_connection_map(conn, ip, port)) {
            LOG(ERROR, "cannot add_connection_map...");
            conn->disconnect();
            //_conn_mgr->del_connection(conn);
            conn->_is_connected = false;
            return -1;
        }
        if(NULL == conn) {
            LOG(ERROR,"creating new connection fail...");
            return -1;    //error
        }
    }
    else {
        LOG_INFO("reusing an old connection. id : %lld", conn->_conn_id);
    }

    if(!conn->is_connected()) {
        //_conn_mgr->del_connection(conn);
        conn->_is_connected = false;
        conn->disconnect();
        conn = NULL;
    }

    return _streamer->on_data_write(conn, a_task, conn->_write);
}

void transport::response(void* a_task, void *obj, uint32_t obj_type) {
	_streamer->response(a_task, obj, obj_type);
}

}

