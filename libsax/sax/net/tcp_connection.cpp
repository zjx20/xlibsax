/*
 * tcp_connection.cpp
 *
 *  Created on: 2011-8-5
 *      Author: livingroom
 */

#include "tcp_connection.h"
#include <iostream>
#include "log.h"

namespace sax {

int tcp_connection::max_buffer_size = 1024;

bool tcp_connection::init(const connection_type &type) {
    this->_conn_type = type;
    this->_conn_protocol = connection_base::TCP;
    this->_last_use_time = g_now_hr();
    _conn_mgr->add_connection(this);
    return true;
}

void tcp_connection::handle_read_event() {
    LOG(INFO,"handle_read_event()\n");

    bool is_client_close = false;
    char tmp_buf[max_buffer_size];
    while(1) {
        int read_len = _tcp.read(tmp_buf, max_buffer_size);
        LOG(INFO, "connection read data len : %d", read_len);
        if (read_len < 0) {  //finish reading
            LOG(INFO, "read ends.");
            break;
        }
        else if (read_len == 0) {
            LOG(INFO, "client fd %d close.",_tcp.get_fd());
            is_client_close = true;
            break;
        }
        tmp_buf[read_len] = '\0';
        LOG(INFO, "client has send : %s", tmp_buf);
        //push tmp_buf into read buffer...
        _read.put((uint8_t*)tmp_buf, read_len);
    }

    if(!is_client_close) {
		_streamer->on_data_read(this, _read);
		set_event(true, true, true);
		this->_last_use_time = g_now_hr();
    }
    else {
        if(this->_conn_type == connection_base::is_connect) {
        	_streamer->on_connection_closed(this);
        }
        del_event(true, true, true);
        disconnect();
        _conn_mgr->del_connection(this);
    }

}

void tcp_connection::handle_write_event() {
    LOG(INFO,"handle_write_event()\n");
    _write.flip();
    size_t totlen = _write.remaining();
    LOG(INFO,"_write.remaining() : %d\n",totlen);

    sax::buffer::pos p = _write.position();
    int has_send = try_send(totlen);
    LOG(INFO,"has_send : %d\n",has_send);
    p += has_send;
    _write.position(p);

    if((size_t)has_send == totlen) {
        //nothing to send
        set_event(true, false, true);
    }

    _write.compact();

    this->_last_use_time = g_now_hr();
}

void tcp_connection::handle_error_event() {
    LOG_INFO("tcp_connection::handle_error_event() ...... \n");
    if(this->_conn_type == connection_base::is_connect) {
        _streamer->on_connection_closed(this);
    }
    del_event(true, true, true);
    disconnect();
    _conn_mgr->del_connection(this);
//    printf("error headle end......\n");
//    fflush(stdout);
}

int tcp_connection::try_send(uint32_t lenth) {
    int tot_send = 0;
    char tmp_buf[max_buffer_size];
    while(lenth > 0) {
        memset(tmp_buf, 0, max_buffer_size);
        if(lenth < (size_t)max_buffer_size) {
            _write.get((uint8_t*)tmp_buf, lenth);
            int has_send = _tcp.write(tmp_buf, lenth);
            if(has_send < 0) {
                // TODO: handle write error
                //printf("error1!!!!!!!!!!! has_send=%d\n",has_send);
                break;
            }
            tot_send += has_send;
            break;
        }
        else {
            _write.get((uint8_t*)tmp_buf, max_buffer_size);
            int has_send = _tcp.write(tmp_buf, max_buffer_size);
            if(has_send < 0) {
                // TODO: handle write error
                //printf("error2!!!!!!!!!!! has_send=%d\n",has_send);
                break;
            }
            tot_send += has_send;
            lenth -= has_send;
            if(has_send != max_buffer_size) { //full
                break;
            }
        }
    }
    return tot_send;
}

int tcp_connection::send(buffer &data) {
    //push data into write buffer
    data.flip();
    size_t data_len = data.remaining();
    bool ret = _write.put(data);
    set_event(true, true, true);
    this->_last_use_time = g_now_hr();
    return ret == true ? data_len : 0;
}

bool tcp_connection::disconnect() {
    if(_tcp.get_fd() < 0) return false;
    _tcp.close();
    LOG(INFO,"fd %d disconnected.",_tcp.get_fd());
    this->_last_use_time = g_now_hr();
    return true;
}

}

