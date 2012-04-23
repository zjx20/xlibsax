/*
 * thrift_task_streamer.h
 *
 *  Created on: 2011-8-17
 *      Author: livingroom
 */

#ifndef THRIFT_TASK_STREAMER_H_
#define THRIFT_TASK_STREAMER_H_

#include "istreamer.h"
#include "thrift_task.h"
#include <sax/c++/fifo.h>
#include <sax/os_api.h>

namespace sax {

class thrift_task_handler {
public:
    thrift_task_handler(){}
    virtual ~thrift_task_handler() {}
public:
    virtual void on_thrift_client_request(thrift_task &a_task) = 0;
};

struct thrift_task_queue : public sax::fifo< thrift_task* >
{
public:
	uint32_t size() {
	    return (_wid - _rid + _cap) % _cap;
	}
};

class thrift_task_streamer : public istreamer {
public:
    thrift_task_streamer() : _handler(NULL) {}
    thrift_task_streamer(thrift_task_handler *handler) : _handler(handler) {}
    virtual ~thrift_task_streamer() {}

public:
    virtual void on_data_read(connection_base *conn, buffer &read);
    virtual int on_data_write(connection_base *conn, void *data, buffer &write);
    virtual void on_connection_closed(connection_base *conn) {}
    void set_handler(thrift_task_handler *handler) {_handler = handler;}

    virtual void response(void* a_task, void *obj, uint32_t obj_type);

private:
    void handle_task(thrift_task *a_task);

//new thread for handler
public:
    void start_handle();
    void stop_handle();
private:
    static long handler_do(void *streamer);
//----

private:
    thrift_task_handler *_handler;
    static thrift_task_queue _task_queue;
    g_thread_t    _handler_t;
    bool          _handler_run_flag;
};

}   //namespace sax

#endif /* THRIFT_TASK_STREAMER_H_ */
