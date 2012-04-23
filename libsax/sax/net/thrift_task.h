/*
 * thrift_task.h
 *
 *  Created on: 2011-8-18
 *      Author: x_zhou
 */

#ifndef _THRIFT_TASK_H_
#define _THRIFT_TASK_H_

#include <string>

namespace sax {

struct thrift_task {
    thrift_task() : ttype(UNKNOWN),fname(""),obj_type(0),obj(NULL),conn(NULL) {}

    enum task_type {
      UNKNOWN      = 0,
      T_CALL       = 1,
      T_REPLY      = 2,
      T_ONEWAY     = 4
    };
    task_type ttype;	// set it to T_REPLY when response
    int32_t seqid;		// don't change it
    std::string fname;	// don't change it
    uint32_t obj_type;
    void* obj;

    //for connection
    void *conn;
};

}

#endif /* _THRIFT_TASK_H_ */
