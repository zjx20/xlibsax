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

namespace sax {

class thrift_task_handler {
public:
    thrift_task_handler(){}
    virtual ~thrift_task_handler() {}
public:
    virtual void on_thrift_client_request(thrift_task &a_task) = 0;
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
    void handle_task(connection_base *conn, thrift_task *a_task);

private:
    thrift_task_handler *_handler;
};

}   //namespace sax

#endif /* THRIFT_TASK_STREAMER_H_ */
