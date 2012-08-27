/*
 * thrift_utility.h
 *
 *  Created on: 2011-8-15
 *      Author: x_zhou
 */

#ifndef _THRIFT_UTILITY_H_
#define _THRIFT_UTILITY_H_

#include <map>

#include "buffer.h"
#include "sax/c++/nocopy.h"
#include "thrift_task.h"

namespace sax {

typedef void* (*to_object_fun)(buffer*);
typedef void (*to_buffer_fun)(buffer*, void*);

typedef thrift_task* (*to_thrift_task_fun)(buffer*);
typedef void (*to_result_buffer_fun)(buffer*, thrift_task*);

class thrift_utility : public nocopy {
public:

	static void* to_object(buffer* buf, uint32_t type);
	static void to_buffer(buffer* dest, void* obj, uint32_t type);

	static thrift_task* to_thrift_task(buffer* buf);
	static void to_result_buffer(buffer* dest, thrift_task* ttask);

	static void thrift_register(uint32_t type, to_object_fun fun1, to_buffer_fun fun2);

	static void thrift_rpc_register(to_thrift_task_fun fun, to_result_buffer_fun fun);

	static thrift_utility& get_instance() {
		static thrift_utility instance;
		return instance;
	}

private:
	thrift_utility() : _to_thrift_task_fun(NULL), _to_result_buffer_fun(NULL) {}

	typedef std::map<uint32_t, to_object_fun> to_object_map;
	to_object_map _to_object_funs;

	typedef std::map<uint32_t, to_buffer_fun> to_buffer_map;
	to_buffer_map _to_buffer_funs;

	to_thrift_task_fun _to_thrift_task_fun;
	to_result_buffer_fun _to_result_buffer_fun;
};

}

#endif /* _THRIFT_UTILITY_H_ */
