#ifndef __STAGE_H_QS__
#define __STAGE_H_QS__

#include <sax/sysutil.h>

#include "fifo.h"
#include "pools.h"

namespace sax {

struct event_type {
public:
	inline int get_type() const {return type_id;}
	virtual ~event_type() {}
protected:
	inline event_type(int id=-1) : type_id(id) {}
	int type_id;
};

template<int tid>
class event_base : public event_type
{
public:
	enum {ID = tid};
	virtual ~event_base() {}
protected:
	inline event_base(int id=tid) : event_type(id) {}
};

struct handler_base {
	virtual bool init(sax::timer_base* timer, void* param) = 0;
	virtual void on_start(int thread_id) {}
	virtual bool on_pump(double sec) {return false;}
	virtual void on_event(const sax::event_type *ev, int thread_id) = 0;
	virtual void on_timeout(uint32_t id, void *param) {}
	virtual void on_finish(int thread_id) {}
	virtual ~handler_base() {}
};

class staged_timer : public timer_base
{
public:
	staged_timer(handler_base *h) : _handler(h) {}
private:
	void on_timeout(uint32_t id, void *param) {
		_handler->on_timeout(id, param);
	}
	handler_base *_handler;
};

struct stage : public nocopy, protected thread_mgr
{
public:
	template<typename HANDLER>
	friend stage *create_stage(const char *name, int queue_cap, 
		int threads, bool singleton, int mem_size, void *param);
	
	template<typename EVENT>
	inline EVENT *new_event() {
		if (is_stopped()) return NULL;
		return _mem.alloc_obj<EVENT>();
	}
	
	inline void del_event(event_type *ev) {_mem.free_obj(ev);}
	
	inline bool push_event(event_type *ev, double sec=-1) 
	{
		if (is_stopped()) return false;
		return _mem.check(ev) && _queue->push_back(ev, sec);
	}
	
	inline void halt() { this->stop(); }

	inline stage() : 
		_handlers(0), _timers(0), _counter(0), _queue(0) {}
	
	inline ~stage() {
		this->stop();
		
		if (_timers) {
			for (int i=0; i<_counter; i++) {
				timer_base *t = _timers[i];
				if (t) delete t;
			}
			delete[] _timers;
		}
		
		if (_handlers) {
			for (int i=0; i<_counter; i++) {
				handler_base *p = _handlers[i];
				if (p) delete p;
			}
			delete[] _handlers;
		}
		
		if (_queue) delete _queue;
	}

protected:
	char _name[256];
	handler_base **_handlers;
	timer_base **_timers;
	int _counter;
	mpool _mem;
	fifo<event_type *> *_queue;
	
private:
	inline void on_routine(int i)
	{
		do {
			char mark[260];
			g_snprintf(mark, sizeof(mark), _name, i);
			g_thread_bind(-1, mark);
		} while(0);
		
		// a thread should drive a timer if existing
		timer_base *tb = (i < _counter ? _timers[i] : NULL);
		
		handler_base *p = _handlers[(i < _counter ? i : 0)];
		p->on_start(i);
		
		const double sec = 0.001; //wait: 1ms
		event_type *ev = NULL;
		while (!is_stopped()) {
			double tw = (p->on_pump(sec) ? 0 : sec);
			if (_queue->pop_front(ev, tw)) {
				p->on_event(ev, i);
				this->del_event(ev);
			}
			if (tb) tb->poll(); // poll timeout events
		}
		
		// handling events that are pending in queue
		while (_queue->pop_front(ev, 0)) {
			p->on_event(ev, i);
			this->del_event(ev);
		}
		p->on_finish(i);
	}
};

template<typename HANDLER>
stage *create_stage(const char *name, int queue_cap, 
	int threads, bool singleton, int mem_size, void *param)
{
	if (queue_cap <= 0 || threads <= 0) return NULL;
	
	stage *st = new stage();
	do {
		int i, n = (singleton ? 1 : threads);
		st->_counter = n;
		g_strlcpy(st->_name, name, sizeof(st->_name));
		
		if (!st->_mem.init(NULL, mem_size)) break;
		
		st->_queue = new fifo<event_type *>(queue_cap);
		if (!st->_queue) break;
		
		handler_base **hb = new handler_base* [n];
		if (NULL == (st->_handlers = hb)) break;
		
		timer_base **tb = new timer_base* [n];
		if (NULL == (st->_timers = tb)) break;
		
		for (i=0; i<n; i++) {hb[i]=NULL; tb[i]=NULL;}
		
		for (i=0; i<n; i++) {
			if (!(hb[i] = new HANDLER)) break;
			if (!(tb[i] = new staged_timer(hb[i]))) break;
			if (!hb[i]->init(tb[i], param)) break;
		}
		
		if (i == n) {
			st->run(threads); return st;
		}
	} while(0);
	
	delete st; return NULL;
}

} //namespace

#endif//__STAGE_H_QS__

