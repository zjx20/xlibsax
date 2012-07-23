#ifndef __STAGE_H_QS__
#define __STAGE_H_QS__

#include "sax/c++/fifo.h"
#include "sax/slab.h"

namespace sax {

struct event_type {
public:
	inline int get_type() const {return type_id;}
	virtual ~event_type() {}
protected:
	inline event_type(int id=-1) : type_id(id) {}
	int type_id;
};

template<int TID, typename REAL_TYPE>
class event_base : public event_type
{
public:
	enum {ID = TID};
	virtual ~event_base() {}

	static REAL_TYPE* new_event()
	{
		return SLAB_NEW(REAL_TYPE);
	}

	void destroy()
	{
		SLAB_DELETE(REAL_TYPE, this);
	}

protected:
	inline event_base() : event_type(TID) {}
};

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
	void run()
	{
		_handler->on_start(_thread_id);

		double sec = 0.100;	// 100 milliseconds
		while (1) {
			event_type* event;
			if (_ev_queue.pop_front(event, sec)) {
				_handler->on_event(event);
			}
			else {
				// idle, do we need an idle event?
				if (_stop) break;
			}
		}

		// TODO: gracefully shutdown

		_handler->on_finish(_thread_id);
	}

	bool push_event(event_type* ev)
	{
		return _ev_queue.push_back(ev);
	}

	void signal_stop() { _stop = true; }

	void wait_for_stop()
	{
		long ret;
		g_thread_join(_thread, &ret);
	}

	static thread_obj* new_thread_obj(const char* name, int32_t thread_id, handler_base* handler)
	{
		thread_obj* tobj = new thread_obj(thread_id, handler);
		if (tobj != NULL) {
			void* param[] = {tobj, name};
			g_thread_t thread = g_thread_start(thread_obj::_thread_proc, param);
			if (thread != NULL) {
				tobj->_thread = thread;
				return tobj;
			}

			delete tobj;
		}
		return NULL;
	}

private:
	thread_obj(int32_t thread_id, handler_base* handler) :
		_thread(NULL), _handler(handler), _thread_id(thread_id), _stop(false) {}

	static void* _thread_proc(void* param)
	{
		const char* name = (const char*)((void**)(param)[0]);
		thread_obj* obj = (thread_obj*)((void**)(param)[1]);

		char new_name[50];
		g_snprintf(new_name, sizeof(new_name), "%32s_%d", name, _thread_id);
		g_thread_bind(-1, new_name);

		((thread_obj*)param)->run();
		return 0;
	}

private:
	fifo<event_type*, slab_stl_allocator<event_type*> > _ev_queue;
	g_thread_t _thread;
	handler_base* _handler;
	int32_t _thread_id;
	volatile bool _stop;
};

class dispatcher_base
{
public:
	virtual ~dispatcher_base() {}
	virtual void init(int32_t number_of_queues) = 0;
	virtual int32_t dispatch(const event_type* ev) = 0;
};

class default_dispatcher : public dispatcher_base
{
public:
	default_dispatcher() : _curr(0), _queues(0) {}
	~default_dispatcher() {}

	void init(int32_t queues) { _queues = queues; }

	int32_t dispatch(const event_type* ev)
	{
		++_curr;
		if (_curr >= _queues) _curr = 0;
		return _curr;
	}

private:
	int32_t _curr;
	int32_t _queues;
};

struct stage
{
public:

	template<typename HANDLER>
	friend
	stage *create_stage(const char *name,
		int threads, void *param);

	inline bool push_event(event_type *ev)
	{
		return _threads[_dispatcher->dispatch(ev)]->push_event(ev);
	}
	
	inline stage() : 
		_handlers(NULL), _threads(NULL), _dispatcher(NULL), _counter(0) {}
	
	inline ~stage() {
		this->stop();
		
		if (_threads) {
			for (int i=0; i<_counter; i++) {
				thread_obj *p = _threads[i];
				if (p) {
					p->wait_for_stop();
					delete p;
				}
			}
			delete[] _threads;
		}

		if (_handlers) {
			for (int i=0; i<_counter; i++) {
				handler_base *p = _handlers[i];
				if (p) delete p;
			}
			delete[] _handlers;
		}

		delete _dispatcher;
	}

	void stop()
	{
		if (_threads) {
			for (int i=0; i<_counter; i++) {
				thread_obj *p = _threads[i];
				if (p) p->signal_stop();
			}
		}
	}

protected:
	handler_base** _handlers;
	thread_obj** _threads;
	dispatcher_base* _dispatcher;
	int _counter;
};

template<typename HANDLER>
stage *create_stage(const char *name,
	int threads, void *param)
{
	if (threads <= 0) return NULL;
	
	stage *st = new stage();
	do {
		int i, n = threads;
		st->_counter = n;
		
		handler_base **hb = new handler_base* [n];
		if (NULL == (st->_handlers = hb)) break;
		
		thread_obj **to = new thread_obj* [n];
		if (NULL == (st->_threads = to)) break;

		for (i=0; i<n; i++) {hb[i]=NULL; to[i]=NULL;}
		
		for (i=0; i<n; i++) {
			if (!(hb[i] = new HANDLER)) break;
			if (!hb[i]->init(param)) break;
		}

		if (i == n) break;

		for (i=0; i<n; i++) {
			if (!(to[i] = thread_obj::new_thread_obj(name, i, hb[i]))) break;
		}

		if (i == 0) return st;
	} while(0);
	
	delete st; return NULL;
}

} //namespace

#endif//__STAGE_H_QS__

