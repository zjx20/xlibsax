#ifndef __STAGE_H_QS__
#define __STAGE_H_QS__

#include <stdio.h>

#include "event_type.h"
#include "dispatcher.h"
#include "event_queue.h"
#include "stage_mgr.h"
#include "sax/c++/fifo.h"
#include "sax/slabutil.h"

namespace sax {

struct handler_base {
	virtual bool init(void* param) = 0;
	virtual void on_start(int32_t thread_id) {}
	virtual void on_event(const sax::event_type *ev) = 0;
	virtual void on_finish(int32_t thread_id) {}
	virtual ~handler_base() {}
};

template <class HANDLER, class THREADOBJ>
class stage_creator;

class thread_obj
{
public:
	virtual void run()
	{
		_handler->on_start(_thread_id);

		uint32_t idle_count = 0;
		while (1) {
			event_type* ev = _ev_queue->pop_event();
			if (ev) {
				_handler->on_event(ev);
				_ev_queue->destroy_event(ev);
			}
			else {
				if (_stop) break;
				if (++idle_count >= 10) {
					g_thread_sleep(0.001);
					idle_count = 0;
				}
			}
		}

		_handler->on_finish(_thread_id);
	}

	void signal_stop() { _stop = true; }

	void join_thread()
	{
		long ret;
		g_thread_join(_thread, &ret);
		_thread = NULL;
	}

	template <class THREADOBJ, class HANDLER>
	static THREADOBJ* new_thread_obj(const char* name,
			int32_t thread_id, HANDLER* handler, event_queue* ev_queue) throw(std::bad_alloc)
	{
		THREADOBJ* tobj = new THREADOBJ(thread_id, handler, ev_queue);
		void** param = new void*[2];
		param[0] = (void*)static_cast<thread_obj*>(tobj);
		param[1] = (void*)name;
		g_thread_t thread = g_thread_start(thread_obj::_thread_proc, param);
		if (thread != NULL) {
			tobj->_thread = thread;
			return tobj;
		}

		delete[] param;
		delete tobj;
		return NULL;
	}

	virtual ~thread_obj()
	{
		assert(_thread == NULL);

		event_type* ev;
		while ((ev = _ev_queue->pop_event()) != NULL) {
			_ev_queue->destroy_event(ev);
		}
	}

protected:
	thread_obj(int32_t thread_id, handler_base* handler, event_queue* ev_queue) :
		_ev_queue(ev_queue), _handler(handler), _thread(NULL),
		_thread_id(thread_id), _stop(false), _create_status(CREATING) {}

	static void* _thread_proc(void* param)
	{
		thread_obj* obj = (thread_obj*)(((void**)param)[0]);
		const char* name = (const char*)(((void**)param)[1]);

		char new_name[50];
		g_snprintf(new_name, sizeof(new_name), "%.32s_%d", name, obj->_thread_id);
		g_thread_bind(-1, new_name);

		delete[] (void**) param;

		if (obj->wait_for_stage_creator()) {
			obj->run();
		}

		return 0;
	}

protected:
	event_queue* _ev_queue;
	handler_base* _handler;
	g_thread_t _thread;
	int32_t _thread_id;
	volatile bool _stop;

private:
	template <class HANDLER, class THREADOBJ>
	friend class stage_creator;

	bool wait_for_stage_creator()
	{
		while (_create_status == CREATING) {
			g_thread_sleep(0.001);
		}
		return _create_status == OK;
	}

	enum create_status {CREATING, OK, FAILED};

	volatile int32_t _create_status;
};

class stage
{
public:
	template <class HANDLER, class THREADOBJ>
	friend class stage_creator;

	template <class EVENT_TYPE>
	inline EVENT_TYPE* allocate_event(uint64_t shard_key = 0, bool wait = true)
	{
		return _queues[_dispatcher->dispatch(EVENT_TYPE::ID, shard_key)]->
				allocate_event<EVENT_TYPE>(wait);
	}

	void push_event(event_type* ev)
	{
		event_queue::event_header* block = (event_queue::event_header*)
				(((char*) ev) - sizeof(event_queue::event_header));
		// gcc 4.6.3 generate bad code when compile with -O3
		MEMORY_BARRIER(&block);
		block->block_state = event_queue::event_header::COMMITTED;
	}

	stage() :
		_handlers(NULL), _threads(NULL), _queues(NULL), _dispatcher(NULL),
		_thread_num(0), _stopped(true), _next(NULL), _prev(NULL) {}
	
	inline ~stage() {
		if (_threads) {
			assert(_stopped);
			for (uint32_t i=0; i<_thread_num; i++) {
				thread_obj *p = _threads[i];
				if (p) delete p;
			}
			delete[] _threads;
		}

		if (_handlers) {
			for (uint32_t i=0; i<_thread_num; i++) {
				handler_base *p = _handlers[i];
				if (p) delete p;
			}
			delete[] _handlers;
		}

		if (_queues) {
			for (uint32_t i=0; i<_thread_num; i++) {
				event_queue* p = _queues[i];
				if (p) delete p;
			}
			delete[] _queues;
		}

		if (_dispatcher) delete _dispatcher;
	}

	void start() { _stopped = false; }

	void signal_stop()
	{
		if (_threads) {
			for (uint32_t i=0; i<_thread_num; i++) {
				thread_obj *p = _threads[i];
				if (p) p->signal_stop();
			}
		}
	}

	void wait_for_stop()
	{
		if (_threads) {
			_stopped = true;
			for (uint32_t i=0; i<_thread_num; i++) {
				thread_obj *p = _threads[i];
				if (p) p->join_thread();
			}
		}
	}

protected:
	handler_base** _handlers;
	thread_obj** _threads;
	event_queue** _queues;
	dispatcher_base* _dispatcher;
	uint32_t _thread_num;
	char _name[33];
	bool _stopped;

private:
	// for linkedlist
	friend class stage_mgr;
	friend class linkedlist<stage>;
	stage* _next;
	stage* _prev;
};

template <class HANDLER, class THREADOBJ = thread_obj>
class stage_creator
{
public:
	static stage* create_stage(const char* name, uint32_t threads, void* handler_param,
			uint32_t queue_bytes, dispatcher_base* dispatcher) throw(std::bad_alloc)
	{
		stage *st = new stage();

		uint32_t i, n = threads;

		handler_base** hb = new handler_base* [n];
		thread_obj** to = new thread_obj* [n];
		event_queue** eq = new event_queue* [n];

		for (i=0; i<n; i++) {hb[i]=NULL; to[i]=NULL; eq[i]=NULL;}

		for (i=0; i<n; i++) {
			hb[i] = new HANDLER();
			if (!hb[i]->init(handler_param)) {
				fprintf(stderr, "%s:%d init handler failed.\n", __FILE__, __LINE__);
				goto create_stage_failed;
			}
		}

		for (i=0; i<n; i++) {
			eq[i] = new event_queue(queue_bytes);
		}

		for (i=0; i<n; i++) {
			to[i] = thread_obj::new_thread_obj<THREADOBJ, HANDLER>(
					name, i, static_cast<HANDLER*>(hb[i]), eq[i]);
			if (!to[i]) goto create_stage_failed;
		}

		g_snprintf(st->_name, sizeof(st->_name), "%.32s", name);
		st->_thread_num = n;
		st->_handlers = hb;
		st->_threads = to;
		st->_queues = eq;
		st->_dispatcher = dispatcher;
		st->_dispatcher->init(threads);

		for (i=0; i<n; i++) {
			to[i]->_create_status = thread_obj::OK;
		}

		st->start();

		stage_mgr::get_instance()->register_stage(st);

		return st;

	create_stage_failed:

		if (to) {
			for (i=0; i<n; i++) {
				if (to[i]) {
					to[i]->_create_status = thread_obj::FAILED;
					to[i]->join_thread();
					delete to[i];
				}
			}
			delete[] to;
		}

		if (hb) {
			for (i=0; i<n; i++) {
				if (hb[i]) delete hb[i];
			}
			delete[] hb;
		}

		if (eq) {
			for (i=0; i<n; i++) {
				if (eq[i]) delete eq[i];
			}
			delete[] eq;
		}

		delete st;

		return NULL;
	}
};

} //namespace

#endif//__STAGE_H_QS__

