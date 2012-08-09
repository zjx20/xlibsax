#ifndef __STAGE_H_QS__
#define __STAGE_H_QS__

#include "event_type.h"
#include "dispatcher.h"
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

class thread_obj
{
public:
	virtual void run()
	{
		_handler->on_start(_thread_id);

		double sec = 0.100;	// 100 milliseconds
		while (1) {
			event_type* event;
			if (_ev_queue.pop_front(event, sec)) {
				_handler->on_event(event);
				event->destroy();
			}
			else {
				if (_stop) break;
			}
		}

		_handler->on_finish(_thread_id);
	}

	bool push_event(event_type* ev, bool wait = true)
	{
		if (wait) {
			return _ev_queue.push_back(ev, -1);
		}
		return _ev_queue.push_back(ev);
	}

	void signal_stop() { _stop = true; }

	void wait_for_stop()
	{
		long ret;
		g_thread_join(_thread, &ret);
		_thread = NULL;
	}

	template <class THREADOBJ, class HANDLER>
	static THREADOBJ* new_thread_obj(const char* name,
			int32_t thread_id, HANDLER* handler, uint32_t queue_capacity)
	{
		THREADOBJ* tobj = new THREADOBJ(thread_id, handler, queue_capacity);
		if (tobj != NULL) {
			void** param = (void**) malloc(sizeof(void*) * 2);
			param[0] = (void*)static_cast<thread_obj*>(tobj);
			param[1] = (void*)name;
			g_thread_t thread = g_thread_start(thread_obj::_thread_proc, param);
			if (thread != NULL) {
				tobj->_thread = thread;
				return tobj;
			}

			delete tobj;
		}
		return NULL;
	}

	virtual ~thread_obj()
	{
		assert(_thread == NULL);

		event_type* ev;
		while (_ev_queue.pop_front(ev, 0)) {
			ev->destroy();
		}
	}

protected:
	thread_obj(int32_t thread_id, handler_base* handler, uint32_t queue_capacity) :
		_ev_queue(queue_capacity), _thread(NULL), _handler(handler),
		_thread_id(thread_id), _stop(false) {}

	static void* _thread_proc(void* param)
	{
		thread_obj* obj = (thread_obj*)(((void**)param)[0]);
		const char* name = (const char*)(((void**)param)[1]);

		char new_name[50];
		g_snprintf(new_name, sizeof(new_name), "%.32s_%d", name, obj->_thread_id);
		g_thread_bind(-1, new_name);

		// TODO: wait for all thread started, or just shutdown if create_stage() failed
		obj->run();

		free(param);

		return 0;
	}

protected:
	fifo<event_type*> _ev_queue;
	g_thread_t _thread;
	handler_base* _handler;
	int32_t _thread_id;
	volatile bool _stop;
};

struct stage
{
public:

	template<typename HANDLER, class THREADOBJ>
	friend
	stage *create_stage(const char *name, int32_t threads, void *handler_param,
			uint32_t queue_capacity, dispatcher_base* dispatcher);

	inline bool push_event(event_type *ev, bool wait = true)
	{
		return _threads[_dispatcher->dispatch(ev)]->push_event(ev, wait);
	}
	
	inline stage() : 
		_handlers(NULL), _threads(NULL), _dispatcher(NULL),
		_counter(0), _stopped(true), _next(NULL), _prev(NULL) {}
	
	inline ~stage() {
		if (_threads) {
			assert(_stopped);
			for (int32_t i=0; i<_counter; i++) {
				thread_obj *p = _threads[i];
				if (p) delete p;
			}
			delete[] _threads;
		}

		if (_handlers) {
			for (int32_t i=0; i<_counter; i++) {
				handler_base *p = _handlers[i];
				if (p) delete p;
			}
			delete[] _handlers;
		}

		if (_dispatcher) delete _dispatcher;
	}

	void start() { _stopped = false; }

	void signal_stop()
	{
		if (_threads) {
			for (int32_t i=0; i<_counter; i++) {
				thread_obj *p = _threads[i];
				if (p) p->signal_stop();
			}
		}
	}

	void wait_for_stop()
	{
		if (_threads) {
			_stopped = true;
			for (int32_t i=0; i<_counter; i++) {
				thread_obj *p = _threads[i];
				if (p) p->wait_for_stop();
			}
		}
	}

protected:
	handler_base** _handlers;
	thread_obj** _threads;
	dispatcher_base* _dispatcher;
	int32_t _counter;
	char _name[33];
	bool _stopped;

private:
	// for linkedlist
	friend class stage_mgr;
	friend class linkedlist<stage>;
	stage* _next;
	stage* _prev;
};

template<class HANDLER, class THREADOBJ>
stage *create_stage(const char *name, int32_t threads, void *handler_param,
		uint32_t queue_capacity, dispatcher_base* dispatcher)
{
	if (threads <= 0) return NULL;
	
	stage *st = new stage();
	if (st == NULL) return NULL;

	int32_t i, n = threads;

	handler_base **hb = new handler_base* [n];
	thread_obj **to = new thread_obj* [n];

	if (NULL == hb || NULL == to) goto create_stage_failed;

	for (i=0; i<n; i++) {hb[i]=NULL; to[i]=NULL;}

	for (i=0; i<n; i++) {
		if (!(hb[i] = new HANDLER)) goto create_stage_failed;
		if (!hb[i]->init(handler_param)) goto create_stage_failed;
	}

	for (i=0; i<n; i++) {
		to[i] = thread_obj::new_thread_obj<THREADOBJ, HANDLER>(
				name, i, static_cast<HANDLER*>(hb[i]), queue_capacity);
		if (!to[i]) goto create_stage_failed;
	}

	g_snprintf(st->_name, sizeof(st->_name), "%.32s", name);
	st->_counter = n;
	st->_handlers = hb;
	st->_threads = to;
	st->_dispatcher = dispatcher;
	st->_dispatcher->init(threads);
	st->start();

	stage_mgr::get_instance()->register_stage(st);

	return st;

create_stage_failed:

	if (to) {
		for (i=0; i<n; i++) {
			if (to[i]) {
				to[i]->signal_stop();
				to[i]->wait_for_stop();
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

	delete st;

	return NULL;
}

} //namespace

#endif//__STAGE_H_QS__

