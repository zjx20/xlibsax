/*
 * stage_mgr.cpp
 *
 *  Created on: 2012-8-1
 *      Author: x
 */

#include "stage_mgr.h"
#include "stage.h"
#include "sax/os_api.h"
#include "sax/sysutil.h"

using namespace sax;

stage_mgr* stage_mgr::get_instance()
{
	static stage_mgr instance;
	return &instance;
}

void stage_mgr::register_stage(stage* st)
{
	auto_lock<spin_type> scoped_lock(_lock);
	_stages.push_back(st);
}

void stage_mgr::unregister_stage(stage* st)
{
	auto_lock<spin_type> scoped_lock(_lock);
	_stages.erase(st);
}

stage* stage_mgr::get_stage_by_name(const char* name)
{
	auto_lock<spin_type> scoped_lock(_lock);
	stage* node = _stages.head();
	while (node != NULL) {
		if (g_strncmp(name, node->_name, 32) == 0) break;
		node = node->_next;
	}
	return node;
}

void stage_mgr::stop_all()
{
	auto_lock<spin_type> scoped_lock(_lock);
	stage* node = _stages.head();
	while (node != NULL) {
		node->signal_stop();
		node = node->_next;
	}

	node = _stages.head();
	while (node != NULL) {
		node->wait_for_stop();
		node = node->_next;
	}

	while (1) {
		node = _stages.head();
		if (node == NULL) break;
		_stages.erase(node);
		delete node;
	}
}
