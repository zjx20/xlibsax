/*
 * cothread.cpp
 *
 *  Created on: 2013-10-12
 *      Author: jianxiongzhou
 */

#include "cothread.h"

#include <stdio.h>

#include <pcl.h>

namespace sax {

cothread_base::~cothread_base()
{
	if (_co) {
		co_delete(_co);
	}
}

cothread_base* cothread_base::get_current()
{
	return (cothread_base*)co_get_data(co_current());
}

void cothread_base::yield()
{
	yield(NULL, NULL);
}

void cothread_base::yield(yield_func func, void* param)
{
	_lock.enter();
	_yield_count += 1;
	_resume_count = _yield_count - 1;
	_lock.leave();
	_mgr->yield_cothread(this, func, param);
}

bool cothread_base::resume()
{
	_lock.enter();
	bool result = ((_resume_count + 1) == _yield_count);
	_resume_count += 1;
	_lock.leave();

	return result && _mgr->resume_cothread(this, false);
}

void cothread_base::interrupt()
{
	if (should_interrupt()) {
		_mgr->resume_cothread(this, true);
	}
}

///////////////////////////////////////////////////
///////////////////////////////////////////////////

bool cothread_mgr::init(cothread_factory* factory)
{
	if (co_thread_init() != 0) {
		return false;
	}

	_cothreads = (cothread_base**) malloc(sizeof(void*) * _routines);
	if (_cothreads == NULL) return false;

	memset(_cothreads, 0, sizeof(void*) * _routines);

	for (uint32_t i = 0; i < _routines; i++) {
		cothread_base* base = factory->create(this);
		if (base == NULL) {
			fprintf(stderr, "cannot new cothread_base instance form factory");
			goto cleanup;
		}

		// TODO: to use self allocate memory for stack?
		// TODO: check stack overflow
		coroutine_t cr = co_create(cothread_mgr::_co_main_func, base, NULL, _stack_size);
		if (cr == NULL) goto cleanup;

		base->set_coroutine(cr);

		_cothreads[i] = base;
	}

	for (uint32_t i = 0; i < _routines; i++) {
		_idle.push_back(_cothreads[i]);
	}

	_factory = factory;

	return true;

cleanup:
	if (_cothreads) {
		for (uint32_t i = 0; i < _routines; i++) {
			if (_cothreads[i]) {
				if (_cothreads[i]->_co != NULL) {
					co_delete(_cothreads[i]->_co);
				}
				factory->destroy(_cothreads[i]);
			}
		}
		free(_cothreads);
		_cothreads = NULL;
	}

	return false;
}

cothread_mgr::~cothread_mgr()
{
	if (_cothreads) {
		for (uint32_t i = 0; i < _routines; i++) {
			if (_cothreads[i]) {
				_factory->destroy(_cothreads[i]);
			}
		}
		free(_cothreads);
		_cothreads = NULL;
	}
}

void cothread_mgr::_co_main_func(void* data) {
	cothread_base* co = (cothread_base*) data;
	while (!co->_stop) {
		co->_state = cothread_base::RUNNING;
		co->_mgr->_running.push_back(co);

		co->run();

		co->_state = cothread_base::IDLE;
		co->_mgr->_running.erase(co);
		co->_mgr->_idle.push_front(co); // prefer to use the last activated cothread

		if (!co->_stop) {
			co_resume(); // passes execution to upper level
		}
	}

	co->_state = cothread_base::STOPPED;
	co->_mgr->_idle.erase(co);
}

bool cothread_mgr::call_cothread()
{
	cothread_base* co = _idle.pop_front();
	if (co == NULL) return false;

	assert(co->is_idle());

	co_call(co->_co);
	return true;
}

bool cothread_mgr::try_resume()
{
	_lock.enter();
	cothread_base* co = _resumable.pop_front();
	_lock.leave();

	if (co) {
		co->_state = cothread_base::RUNNING;
		_running.push_back(co);

		co_call(co->_co);
		return true;
	}

	return false;
}

void cothread_mgr::signal_stop(bool interrupt)
{
	_lock.enter();

	if (interrupt) {
		cothread_base* co = NULL;
		while ((co = _suspended.head()) != NULL) {
			// spin_type does not support recursive locking
			if (co->should_interrupt()) {
				this->resume_cothread(co, true, false);
			}
		}
	}

	for (uint32_t i=0; i<_routines; i++) {
		_cothreads[i]->_stop;
	}

	_lock.leave();
}

void cothread_mgr::yield_cothread(cothread_base* co, cothread_base::yield_func func, void* param)
{
	_lock.enter();
	assert(co->is_running());
	co->_state = cothread_base::SUSPENDED;
	co->_interrupted = false;
	_running.erase(co);
	_suspended.push_back(co);
	_lock.leave();

	if (func) {
		func(co, param);
	}

	co_resume(); // passes execution to upper level
}

bool cothread_mgr::resume_cothread(cothread_base* co, bool interrupt, bool lock/* = true*/)
{
	if (lock) _lock.enter();

	bool result = false;
	if (co->is_suspended()) {
		co->_state = cothread_base::RESUMABLE;
		co->_interrupted = interrupt;
		_suspended.erase(co);
		_resumable.push_back(co);
		result = true;
	}

	if (lock) _lock.leave();

	return result;
}

}
