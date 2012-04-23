/*
 * istreamer.h
 *
 *  Created on: 2011-8-5
 *      Author: livingroom
 */

#ifndef ISTREAMER_H_
#define ISTREAMER_H_

#include "buffer.h"
#include "connection_mgr.h"

namespace sax {

class istreamer {
public:
    istreamer() {}
    virtual ~istreamer() {}
    virtual void on_data_read(connection_base *conn, buffer &read) = 0;
    virtual int on_data_write(connection_base *conn, void *data, buffer &write) = 0;
    virtual void on_connection_closed(connection_base *conn) = 0;

    virtual void response(void* a_task, void *obj, uint32_t obj_type) = 0;
};

class memory_pool_factory {
public:
	static memory_pool* get_instance() {
		static memory_pool instance;
		return &instance;
	}
};

}


#endif /* ISTREAMER_H_ */
