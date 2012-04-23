/*
 * thrift_utility.cpp
 *
 *  Created on: 2011-8-15
 *      Author: x_zhou
 */

#include "thrift_utility.h"

#include <iostream>

namespace sax {

void* thrift_utility::to_object(buffer* buf, uint32_t type) {
	thrift_utility& tu = thrift_utility::get_instance();
	to_object_map::const_iterator it = tu._to_object_funs.find(type);
	if(it != tu._to_object_funs.end()) {
		return it->second(buf);
	}
	return NULL;
}

void thrift_utility::to_buffer(buffer* dest, void* obj, uint32_t type) {
	thrift_utility& tu = thrift_utility::get_instance();
	to_buffer_map::const_iterator it = tu._to_buffer_funs.find(type);
	if(it != tu._to_buffer_funs.end()) {
		it->second(dest, obj);
	}
}

thrift_task* thrift_utility::to_thrift_task(buffer* buf) {
	thrift_utility& tu = thrift_utility::get_instance();
	if(tu._to_thrift_task_fun == NULL) return NULL;
	return tu._to_thrift_task_fun(buf);
}

void thrift_utility::to_result_buffer(buffer* dest, thrift_task *ttask) {
	thrift_utility& tu = thrift_utility::get_instance();
	if(tu._to_result_buffer_fun != NULL) {
		tu._to_result_buffer_fun(dest, ttask);
	}
}

void thrift_utility::thrift_register(uint32_t type, to_object_fun fun1, to_buffer_fun fun2) {
	thrift_utility& tu = thrift_utility::get_instance();
	if(tu._to_object_funs[type] != NULL) {
		std::cerr << "ERROR: same typeid of thrift-struct registered, " <<
				"please check your program building process " <<
				"to avoid linking more than one auto_serial module." <<
				std::endl;
	}
	tu._to_object_funs[type] = fun1;
	tu._to_buffer_funs[type] = fun2;
}

void thrift_utility::thrift_rpc_register(to_thrift_task_fun fun1, to_result_buffer_fun fun2) {
	thrift_utility& tu = thrift_utility::get_instance();
	if(tu._to_thrift_task_fun != NULL) {
		std::cerr << "WARNING: we only support one thrift-service at one time, " <<
				"please check your program building process " <<
				"to avoid linking more than one auto_rpc_serial module." <<
				std::endl;
	}
	tu._to_thrift_task_fun = fun1;
	tu._to_result_buffer_fun = fun2;
}

}
