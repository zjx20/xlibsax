/*
 * task_streamer.cpp
 *
 *  Created on: 2011-8-11
 *      Author: livingroom
 */
#include "task_streamer.h"
#include "log.h"
#include "tcp_connection.h"

namespace sax {

task_queue task_streamer::_task_queue(10240);

void task_streamer::on_data_read(connection_base *conn, buffer &read) {
    read.flip();
    //read 4 bytes,then read a full task

    while(read.remaining() >= (size_t)sizeof(uint32_t)) {
        LOG(INFO,"read.remaining() :  %d\n",read.remaining());

        read.mark();
        uint32_t task_len = 0;
        read.get(task_len);

        LOG(INFO,"task lenth : %u\n",task_len);

        if(read.remaining() >= (size_t)task_len) {
            task *a_task = new task();
            read.get(a_task->trans_key);
            read.get(a_task->type);
            uint8_t flag = 0;
            //get one byte for parameter set
            read.get(flag);
            a_task->is_call = (flag & 1) != 0;
            a_task->is_need_response = (flag & (1 << 1)) != 0;
            a_task->is_finish = (flag & (1 << 2)) != 0;
            a_task->is_thrift_data = (flag & (1 << 3)) != 0;
            if(!read.get(a_task->data, task_len - 13)) {
                LOG(ERROR,"cannot get buffer ,lenth : %d,read.remaining() :  %d\n",task_len - 14,read.remaining());
            }

            if(a_task->is_thrift_data) {
            	// auto translate
            	a_task->data.flip();
            	a_task->obj = thrift_utility::to_object(&a_task->data, a_task->type);
            	a_task->data.compact();
            }
            a_task->conn = conn;

//            //deal with a task
//            handle_task(conn, a_task);

            //push the task into task_queue
            if( !_task_queue.push_back(a_task, 500) ) {
                LOG_ERROR("a_task cannot push into the queue.transkey : %llu",a_task->trans_key);
                delete a_task;
            }
            else {
                LOG_INFO("Push a task...current size : %u",_task_queue.size());
            }
        }
        else {
            read.reset();
            break;
        }
    }

    read.compact();
}

int task_streamer::on_data_write(connection_base *conn, void *data, buffer &write) {
    task *a_task = (task *)data;

    if(a_task->is_thrift_data) {
    	//auto translate
    	thrift_utility::to_buffer(&a_task->data, a_task->obj, a_task->type);
    }
    if(a_task->is_call && a_task->is_need_response) {
        add_need_response_task(conn->_conn_id, a_task);
    }

    a_task->data.flip();

    uint32_t totlen = a_task->data.remaining() + sizeof(a_task->trans_key) + sizeof(a_task->type) + sizeof(uint8_t);
    LOG_INFO("totlen [%d] has put into write buffer.",totlen);
    if(conn && connection_base::TCP == conn->_conn_protocol) {
        ((tcp_connection *)conn)->lock_write();
        write.put(totlen);
        write.put(a_task->trans_key);
        write.put(a_task->type);
        uint8_t flag = 0;
        flag = (1 & a_task->is_call) + ((a_task->is_need_response)?((1 << 1)):0)
                + ((a_task->is_finish)?((1 << 2)):0) + ((a_task->is_thrift_data)?((1 << 3)):0);
        write.put(flag);
        write.put(a_task->data, a_task->data.remaining());
        ((tcp_connection *)conn)->unlock_write();
    }
    return totlen;
}

void task_streamer::handle_task(task *a_task) {
    if(a_task->is_call) {
        LOG(INFO,"on_user_request\n");
        _handler->on_user_request(a_task);
    }
    else {
        LOG(INFO,"on_task_finish\n");
        on_response_task_back(a_task->conn->_conn_id, a_task);
        _handler->on_task_finish(a_task);
    }
}

void task_streamer::add_need_response_task(uint64_t conn_id, task *a_task) {
    task_state state;
    state.start_time = 0;
    state.type = a_task->type;

    conn_task_map::iterator it = _conn_task_map.find(conn_id);
    if(it == _conn_task_map.end()) {
        task_state_map ts;
        ts.insert(task_state_map::value_type(a_task->trans_key, state));
        _conn_task_map.insert(conn_task_map::value_type(conn_id, ts));
    }
    else {
        task_state_map::iterator it2 = it->second.find(a_task->trans_key);
        if(it2 != it->second.end()) {
            it->second.insert(task_state_map::value_type(a_task->trans_key, state));
        }
    }
}

void task_streamer::on_response_task_back(uint64_t conn_id, task *a_task) {
    conn_task_map::iterator it = _conn_task_map.find(conn_id);
    if(it == _conn_task_map.end()) {
        a_task->is_sended = false;
        return;
    }
    task_state_map::iterator it2 = it->second.find(a_task->trans_key);
    if(it2 == it->second.end()) {
        a_task->is_sended = false;
        return;
    }
    task_state &state = _conn_task_map[conn_id][a_task->trans_key];
    a_task->is_sended = true;
    a_task->is_recieved = true;

    ///TODO check the time
    a_task->is_timeout = state.start_time == 0;

    _conn_task_map[conn_id].erase(it2);
}

void task_streamer::on_connection_closed(connection_base *conn) {
	conn_task_map::iterator it = _conn_task_map.find(conn->_conn_id);
	if(it == _conn_task_map.end()) return;
	task *a_task = new task();
	a_task->is_recieved = false;
	a_task->is_finish = false;
	a_task->conn = conn;
	for(task_state_map::iterator it2 = it->second.begin();
			it2 != it->second.end();it2++) {
	    a_task->trans_key = it2->first;
	    a_task->type = it2->second.type;
		_handler->on_task_finish(a_task);
	}
}

void task_streamer::response(void *a_task, void *obj, uint32_t obj_type) {
	task *tk_task = (task *)a_task;
    if(tk_task->conn && connection_base::TCP == tk_task->conn->_conn_protocol) {
        tcp_connection *tcp_conn = (tcp_connection*)tk_task->conn;
        tk_task->obj = obj;
        tk_task->type = obj_type;
        tk_task->is_thrift_data = true;
        tk_task->is_call = false;
        on_data_write(tcp_conn, (void *)tk_task, tcp_conn->get_write_buffer());
//        tcp_conn->set_event(true , true);
    }
    else {
        LOG_ERROR("can not response task.");
    }
}

void task_streamer::start_handle() {
    LOG_INFO("task_handler is starting to handle task...");
    _handler_run_flag = true;
    _handler_t = g_thread_start(this->handler_do, this);
}

void task_streamer::stop_handle() {
    LOG_INFO("task_handler is stopping...");
    _handler_run_flag = false;
    g_thread_join(_handler_t, NULL);
}

long task_streamer::handler_do(void *data) {
	task_streamer *streamer = (task_streamer *)data;
    LOG_INFO("thread running...%d" ,streamer->_handler_run_flag);
    while(streamer->_handler_run_flag) {
        task *a_task = NULL;
        if(_task_queue.pop_front(a_task, 500) && a_task) {
            LOG_INFO("Get a task...");
            streamer->handle_task(a_task);
        }

        LOG_INFO("current size : %u ", _task_queue.size());
        sleep(1);
    }
    return 0;
}

}


