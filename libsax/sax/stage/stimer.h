/*
 * stimer.h
 *
 *  Created on: 2012-7-30
 *      Author: x
 */

#ifndef STIMER_H_
#define STIMER_H_

#include "stage.h"
#include "sax_events.h"
#include "sax/timer.h"

namespace sax {

class stimer_handler : public handler_base
{
	struct timer_param
	{
		uint64_t trans_id;
		stage* biz_stage;
		void* param;
		stimer_handler* self;
	};

public:

	stimer_handler() : _timer(NULL), _last_ms(g_now_ms()) {}

	virtual ~stimer_handler()
	{
		if (_timer) g_timer_destroy(_timer, _free_func);
	}

	virtual bool init(void* param)
	{
		_timer = g_timer_create();
		return _timer != NULL;
	}

	virtual void on_event(const sax::event_type* ev)
	{
		switch(ev->get_type())
		{
		case add_timer_event::ID:
		{
			add_timer_event* event = (add_timer_event*) ev;

			timer_param* p = new timer_param();
			p->trans_id = event->trans_id;
			p->biz_stage = event->biz_stage;
			p->param = event->invoke_param;
			p->self = this;

			g_timer_start(_timer, event->delay_ms,
					stimer_handler::_timer_proc, p);

			break;
		}
		default:
			assert(0);
			break;
		}
	}

	void poll_timer()
	{
		uint64_t now_ms = g_now_ms();
		if (LIKELY(now_ms > _last_ms)) {
			g_timer_poll(_timer, now_ms - _last_ms);
		}
		_last_ms = now_ms;
	}

private:

	static void _timer_proc(g_timer_handle_t handle, void* param)
	{
		timer_param* p = (timer_param*) param;

		timer_timeout_event* invoke =
				p->biz_stage->allocate_event<timer_timeout_event>(p->trans_id);
		invoke->trans_id = p->trans_id;
		invoke->invoke_param = p->param;
		p->biz_stage->push_event(invoke);

		delete p;
	}

	static void _free_func(void* user_data)
	{
		timer_param* p = (timer_param*) user_data;
		delete p;
	}

	g_timer_t* _timer;
	uint64_t _last_ms;
};

class stimer_threadobj : public thread_obj
{
public:
	stimer_threadobj(int32_t thread_id, stimer_handler* handler, event_queue* ev_queue) :
		thread_obj(thread_id, handler, ev_queue) {}
	virtual ~stimer_threadobj() {}

	virtual void run()
	{
		const int POLLING_COUNT = 10;
		const int SLEEP_IDLE_COUNT = 10;
		int32_t count = 0;
		int32_t idle_count = 0;
		double sec = 0.001;	// 1 millisecond
		while (!_stop) {
			count++;
			event_type* ev;
			if ((ev = _ev_queue->pop_event()) != NULL) {
				_handler->on_event(ev);
				_ev_queue->destroy_event(ev);
			}
			else {
				if (++idle_count >= SLEEP_IDLE_COUNT) {
					g_thread_sleep(sec);
					((stimer_handler*) _handler)->poll_timer();
					idle_count = 0;
				}
			}

			if (count >= POLLING_COUNT) {
				((stimer_handler*) _handler)->poll_timer();
			}
		}
	}
};

stage* create_stimer(size_t queue_bytes = 2 * 1024 * 1024)
{
	dispatcher_base* dispatcher = new single_dispatcher();
	if (dispatcher == NULL) return NULL;
	stage* timer = stage_creator<stimer_handler, stimer_threadobj>::create_stage(
			"global_timer", 1, NULL, queue_bytes, dispatcher);
	return timer;
}

} //namespace

#endif /* STIMER_H_ */
