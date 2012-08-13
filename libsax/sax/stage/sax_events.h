/*
 * sax_events.h
 *
 *  Created on: 2012-7-30
 *      Author: x
 */

#ifndef SAX_EVENTS_H_
#define SAX_EVENTS_H_

#include "event_type.h"
#include "sax/os_types.h"
#include "sax/compiler.h"

namespace sax {

////////////////////////////////////////////////////////////////////////

/**
 * brief: add a timer to stimer stage
 * sender: any stage
 * recver: stimer stage
 * parameters:
 *   trans_id: use for the sender stage to identify which timer is being invoked
 *   biz_stage: for sending timer_timeout_event
 *   invoke_param: for timer_timeout_event
 *   delay_ms: delay time, in millisecond
 */
struct add_timer_event : public sax_event_base<__LINE__, add_timer_event>
{
	size_t		trans_id;
	stage*		biz_stage;
	void*		invoke_param;
	uint32_t	delay_ms;
};

/**
 * brief: a timer timeout
 * sender: stimer stage
 * recver: any stage
 * parameters:
 *   trans_id: same as add_timer_event::trans_id
 *   invoke_param: same as add_timer_event::invoke_param
 */
struct timer_timeout_event : public sax_event_base<__LINE__, timer_timeout_event>
{
	size_t	trans_id;
	void*	invoke_param;
};

namespace hide {
template <size_t BODY_SIZE>
struct log_event
{
	char body[BODY_SIZE];
};
} // namespace hide

#define DECL_LOG_EVENT(size)	\
	template <>	struct hide::log_event<64> : public sax_event_base<__LINE__, log_event<64> > {}; \
	typedef hide::log_event<64> log_event##size;

DECL_LOG_EVENT(64);			// log_event64
DECL_LOG_EVENT(128);		// log_event128
DECL_LOG_EVENT(256);		// log_event256
DECL_LOG_EVENT(512);		// log_event512
DECL_LOG_EVENT(1024);		// log_event1024
DECL_LOG_EVENT(2048);		// log_event2048
DECL_LOG_EVENT(4096);		// log_event4096
DECL_LOG_EVENT(8192);		// log_event8192

#undef DECL_LOG_EVENT

STATIC_ASSERT(__LINE__ < event_type::USER_TYPE_START,
		sax_event_type_id_must_smaller_than__event_type__USER_TYPE_START);

} // namespace

#endif /* SAX_EVENTS_H_ */
