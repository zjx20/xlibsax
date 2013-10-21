/*
 * cothread.h
 *
 *  Created on: 2013-10-12
 *      Author: jianxiongzhou
 */

#ifndef COTHREAD_H_
#define COTHREAD_H_

#include <stdlib.h>
#include <assert.h>

#include "sax/sysutil.h"
#include "sax/c++/linkedlist.h"

// move "include pcl.h" to cothread.cpp
typedef void* coroutine_t;

namespace sax {

class cothread_mgr;

class cothread_base
{
public:
	cothread_base(cothread_mgr* mgr) :
			_next(NULL), _prev(NULL), _mgr(mgr), _co(NULL),
			_state(cothread_base::IDLE), _stop(false),
			_interruptable(true), _interrupted(false),
			_yield_count(0), _resume_count(0)
	{}

	virtual ~cothread_base();

	virtual void run() = 0;

	// can only be called by the thread which is same as the one calling run().
	//
	// NOTE:
	//   the typical usage pattern is leaving a task before yield(), and then
	//   calling resume() after the task completed.
	//   however, it may fail if the task is designed to be handled in another
	//   thread, then calling resume() directly. because there is NO guarantee
	//   that yield() can always be fired earlier than resume().
	//
	// when the cothread resumed, it should check is_interrupted(),
	// and then handle the situation of interrupted (typically going to stop).
	void yield();

	typedef void (*yield_func)(cothread_base* co, void* param);

	// this is a overload of yield(), almost same as yield() version.
	// the difference is the additional func parameter. it will call
	// the func before yield, and it can guarantee that, any resume()
	// calling for this cothread is safe whose invoke time older than
	// func's. in the other word, tasks can be delegated to other thread
	// in func, to avoid the issue described above.
	void yield(yield_func func, void* param);

	// call by other thread.
	// return false if resume()/interrupt() has already been called,
	// or the cothread isn't in suspended state at all
	bool resume();

	inline bool is_idle() { return _state == cothread_base::IDLE; }
	inline bool is_running() { return _state == cothread_base::RUNNING; }
	inline bool is_suspended() { return _state == cothread_base::SUSPENDED; }
	inline bool is_resumable() { return _state == cothread_base::RESUMABLE; }
	inline bool is_stopped() { return _state == cothread_base::STOPPED; }

	inline bool is_interrupted(bool clean = false)
	{
		if (_interrupted) {
			_interrupted = !clean;
			return true;
		}
		return false;
	}

	inline bool should_interrupt() { return _interruptable; }
	inline void set_interruptable(bool value) { _interruptable = value; }

	inline bool should_stop() { return _stop; }

	static cothread_base* get_current();

private:
	// call by cothread_mgr
	void interrupt();

	inline void set_coroutine(coroutine_t cr) { _co = cr; }

private:

	enum state
	{
		IDLE,
		RUNNING,
		SUSPENDED,
		RESUMABLE,
		STOPPED,
	};

private:
	friend class cothread_mgr;

	friend class sax::linkedlist<cothread_base>;
	cothread_base* _next;
	cothread_base* _prev;

	cothread_mgr*  _mgr;
	coroutine_t    _co;
	state          _state;
	volatile bool  _stop;
	bool           _interruptable;
	bool           _interrupted;

	uint32_t       _yield_count;
	uint32_t       _resume_count;
	sax::spin_type _lock; // lock for counters
};

///////////////////////////////////////////////////
///////////////////////////////////////////////////

class cothread_factory
{
public:
	cothread_factory() {}
	virtual ~cothread_factory() {}

	virtual cothread_base* create(cothread_mgr* mgr) = 0;
	virtual void destroy(cothread_base* co) = 0;
};

///////////////////////////////////////////////////
///////////////////////////////////////////////////

class cothread_mgr
{
public:
	cothread_mgr(uint32_t routines, uint32_t stack_size) :
			_routines(routines), _stack_size(stack_size),
			_cothreads(NULL), _factory(NULL)
	{
		assert(routines > 0);
		assert(stack_size > 4096);
	}

	virtual ~cothread_mgr();

	// NOTE: don't deallocate the factory object
	bool init(cothread_factory* factory);

	bool call_cothread();

	bool try_resume();

	// call by its' owner thread
	// it will interrupt all suspended cothreads
	void signal_stop(bool interrupt);

private:
	void yield_cothread(cothread_base* co, cothread_base::yield_func func, void* param);

	// the lock param is for signal_stop()
	bool resume_cothread(cothread_base* co, bool interrupt, bool lock = true);

	static void _co_main_func(void* data);

private:
	friend class cothread_base;

	uint32_t _routines;
	uint32_t _stack_size;

	sax::linkedlist<cothread_base> _suspended;
	sax::linkedlist<cothread_base> _resumable;
	sax::linkedlist<cothread_base> _running; // only be maintained in one thread
	sax::linkedlist<cothread_base> _idle; // only be maintained in one thread

	cothread_base**   _cothreads;
	cothread_factory* _factory;
	sax::spin_type    _lock; // lock for _suspended and _resumable
};

}

#endif /* COTHREAD_H_ */
