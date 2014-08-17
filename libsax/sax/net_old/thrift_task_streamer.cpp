/*
 * thrift_task_streamer.cpp
 *
 *  Created on: 2011-8-17
 *      Author: livingroom
 */

#include "thrift_task_streamer.h"
#include "log.h"
#include "thrift_utility.h"
#include "tcp_connection.h"

namespace sax {

void thrift_task_streamer::on_data_read(connection_base *conn, buffer &read) {
    read.flip();

    while(read.remaining() >= (size_t)sizeof(uint32_t)) {
        LOG(INFO,"read.remaining() :  %d\n",read.remaining());
        sax::buffer::pos p = read.position();
        sax::buffer::pos lim = read.limit();
        uint32_t task_len = 0;
        uint8_t  tmp_len[4];
        read.get(tmp_len,4);
        task_len |= tmp_len[0];
        task_len <<= 8;
        task_len |= tmp_len[1];
        task_len <<= 8;
        task_len |= tmp_len[2];
        task_len <<= 8;
        task_len |= tmp_len[3];
        LOG(INFO,"task lenth : %u\n",task_len);

	if(task_len != 33){
		printf("error!!!!!!!!!!! %u\n",task_len);
	}
        if(read.remaining() >= (size_t)task_len) {
            read.position(p);
            p += sizeof(uint32_t) + task_len;
            read.limit(p);
            thrift_task *a_task = thrift_utility::to_thrift_task(&read);
            read.limit(lim);
            read.position(p);
            if(NULL == a_task) {   //abandon this packet
                continue;
            }
            //deal with a task
            handle_task(conn, a_task);
        }
        else {  // not a full packet
            read.position(p);
            break;
        }
    }

    read.compact();
}


int thrift_task_streamer::on_data_write(connection_base *conn, void *data, buffer &write) {
    if(NULL == data) {
        LOG_ERROR("Cannot send NULL data ... !");
        return -1;
    }
    thrift_task *a_task = (thrift_task *)data;
    write.flip();
    size_t old_len = write.remaining();
    write.compact();
    if(NULL == a_task->obj) {
        LOG_ERROR("a_task->obj is NULL ... !");
        return -1;
    }
    thrift_utility::to_result_buffer(&write, a_task);
    if(NULL != a_task->obj) {
        /// TODO print the a_task->obj_type 's enum_name
        LOG_WARN("a_task->obj is not NULL ... !thrift_task's obj_type is wrong,obj_type : %u",a_task->obj_type);
    }
    write.flip();
    size_t new_len = write.remaining();
    write.compact();
    LOG_INFO("thrift_task data has been write in buffer [%d] bytes. type : %u", new_len - old_len, a_task->obj_type);
    return new_len - old_len;
}

void thrift_task_streamer::handle_task(connection_base *conn, thrift_task *a_task) {
    if(thrift_task::T_CALL == a_task->ttype ||
            thrift_task::T_ONEWAY == a_task->ttype) {
        a_task->conn = conn;
        _handler->on_thrift_client_request(*a_task);
        delete a_task;
    }
    else {
        LOG_ERROR("UNKNOWN handle thrift_task type : %d",a_task->ttype);
    }
}

void thrift_task_streamer::response(void* a_task, void *obj, uint32_t obj_type) {
    thrift_task *tt_task = (thrift_task *)a_task;
    connection_base *conn = (connection_base *)tt_task->conn;
    if(conn && connection_base::TCP == conn->_conn_protocol) {
        tcp_connection *tcp_conn = (tcp_connection*)conn;
        tt_task->obj = obj;
        tt_task->ttype = thrift_task::T_REPLY;
        tt_task->obj_type = obj_type;
        on_data_write(tcp_conn, (void *)tt_task, tcp_conn->get_write_buffer());
    }
    else {
        LOG_ERROR("can not response thrift_task .");
    }
}

}

