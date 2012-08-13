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
		size_t trans_id;
		stage* biz_stage;
		void* param;
		stimer_handler* self;
	};

public:

	stimer_handler() : _timer(NULL), _destroying(false) {}

	virtual ~stimer_handler()
	{
		_destroying = true;
		if (_timer) g_timer_destroy(_timer, 1);
	}

	virtual bool init(void* param)
	{
		_timer = g_timer_create(0);
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
		g_timer_poll(_timer, 0);
	}

private:

	static void _timer_proc(g_timer_handle_t handle, void* param)
	{
		timer_param* p = (timer_param*) param;

		if (LIKELY(!p->self->_destroying)) {
			timer_timeout_event invoke;
			invoke.trans_id = p->trans_id;
			invoke.invoke_param = p->param;
			p->biz_stage->push_event(&invoke);
		}

		delete p;
	}

	g_timer_t* _timer;
	bool _destroying;
};

class stimer_threadobj : public thread_obj
{
public:
	stimer_threadobj(int32_t thread_id, stimer_handler* handler, uint32_t queue_capacity) :
		thread_obj(thread_id, handler, queue_capacity) {}
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
			if ((ev = _ev_queue.pop_event()) != NULL) {
				_handler->on_event(ev);
				_ev_queue.destroy_event(ev);
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

stage* create_stimer(size_t queue_size = 2 * 1024 * 1024)
{
	dispatcher_base* dispatcher = new single_dispatcher();
	if (dispatcher == NULL) return NULL;
	stage* timer = create_stage<stimer_handler, stimer_threadobj>(
			"global_timer", 1, NULL, queue_size, dispatcher);
	return timer;
}

} //namespace

#endif /* STIMER_H_ */
