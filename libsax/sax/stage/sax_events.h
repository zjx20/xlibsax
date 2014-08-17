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
	uint64_t	trans_id;
	stage*		biz_stage;
	void*		invoke_param;
	uint32_t	delay_ms;
};

/**
 * brief: a timer timeout
 * sender: stimer stage
 * recver: any stage
 * parameters:
 *   trans_id: same as add_timer_event::trans_id, and use as shard_key
 *   invoke_param: same as add_timer_event::invoke_param
 */
struct timer_timeout_event : public sax_event_base<__LINE__, timer_timeout_event>
{
	uint64_t	trans_id;
	void*		invoke_param;
};

template <size_t BODY_SIZE>
struct log_event;

#define DECL_LOG_EVENT(size)	\
	template <>	struct log_event<size> : public sax_event_base<__LINE__, log_event<size> > \
	{ char body[size]; }; \
	typedef log_event<size> log_event##size

DECL_LOG_EVENT(128);		// sax::log_event128
DECL_LOG_EVENT(256);		// sax::log_event256
DECL_LOG_EVENT(512);		// sax::log_event512
DECL_LOG_EVENT(1024);		// sax::log_event1024
DECL_LOG_EVENT(2048);		// sax::log_event2048
DECL_LOG_EVENT(4096);		// sax::log_event4096
DECL_LOG_EVENT(8192);		// sax::log_event8192

#undef DECL_LOG_EVENT

STATIC_ASSERT(__LINE__ < event_type::USER_TYPE_START,
		sax_event_type_id_must_smaller_than__event_type__USER_TYPE_START);

} // namespace

#endif /* SAX_EVENTS_H_ */
