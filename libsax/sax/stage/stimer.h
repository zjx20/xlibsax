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

			timer_param* p = SLAB_NEW(timer_param);
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
			timer_timeout_event* invoke = timer_timeout_event::new_event();
			invoke->trans_id = p->trans_id;
			invoke->invoke_param = p->param;
			p->biz_stage->push_event(invoke);
		}

		SLAB_DELETE(timer_param, p);
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
		double sec = 0.001;	// 1 millisecond
		while (!_stop) {
			event_type* event;
			if (_ev_queue.pop_front(event, sec)) {
				_handler->on_event(event);
				event->destroy();
			}

			((stimer_handler*) _handler)->poll_timer();
		}
	}
};

stage* get_global_stimer()
{
	static stage* timer =
			create_stage<stimer_handler, stimer_threadobj>(
					"global_timer", 1, NULL, 2048, new single_dispatcher());
	return timer;
}

} //namespace

#endif /* STIMER_H_ */
