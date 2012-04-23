/*
 * task_streamer.h
 *
 *  Created on: 2011-8-11
 *      Author: livingroom
 */

#ifndef TASK_STREAMER_H_
#define TASK_STREAMER_H_

#include "istreamer.h"
#include "thrift_utility.h"
#include <sax/c++/fifo.h>
#include <sax/os_api.h>

namespace sax {

struct task_state {
    task_state() : start_time(0) {}
    uint32_t type;
    uint64_t start_time;
};

struct task {
    task() : trans_key(0), type(0),
            is_call(false), is_need_response(false), is_finish(false),
            is_timeout(false), is_sended(false), is_recieved(false),
            data(dynamic_cast<sax::memory_pool*>(memory_pool_factory::get_instance())),
            obj(NULL), conn(NULL)
            {}

    uint64_t trans_key;
    uint32_t type;

    //one byte for parameter set
    bool is_call;
    bool is_need_response;
    bool is_finish;
    bool is_thrift_data;

    //one byte for result set
    bool is_timeout;
    bool is_sended;
    bool is_recieved;

    //data
    buffer data;
    void *obj;   //temp

    //connection
    connection_base *conn;
};

class task_handler {
public:
    task_handler () {}
    virtual ~task_handler () {}
public:
    virtual void on_user_request(task *a_task) = 0;
    virtual void on_task_finish(task *a_task) = 0;
};

class task_streamer;
typedef std::map<uint64_t,task_state> task_state_map;
typedef std::map<uint64_t, task_state_map > conn_task_map;
typedef sax::fifo< task* > task_queue;


class task_streamer : public istreamer {
public:
    task_streamer() : _handler(NULL),
                    _handler_t(NULL), _handler_run_flag(false)
                    {}
    virtual ~task_streamer() {}

public:
    void set_handler(task_handler *handler) {_handler = handler;}
    virtual void on_data_read(connection_base *conn, buffer &read);
    virtual int on_data_write(connection_base *conn, void *data, buffer &write);
    virtual void on_connection_closed(connection_base *conn);
    virtual void response(void* a_task, void *obj, uint32_t obj_type);

private:
    void handle_task(task *a_task);

    void add_need_response_task(uint64_t conn_id, task *a_task);
    void on_response_task_back(uint64_t conn_id, task *a_task);

//new thread for handler
public:
    void start_handle();
    void stop_handle();
private:
    static long handler_do(void *streamer);
//----

private:
    task_handler *_handler;
    conn_task_map _conn_task_map;
    static task_queue    _task_queue;
    g_thread_t    _handler_t;
    bool          _handler_run_flag;
};

}    // namespace sax

#endif /* TASK_STREAMER_H_ */
