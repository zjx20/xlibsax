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

struct timer_timeout_event : public sax_event_base<__LINE__, timer_timeout_event>
{
	size_t	trans_id;
	void*	invoke_param;
};

STATIC_ASSERT(__LINE__ < event_type::USER_TYPE_START,
		sax_event_type_id_must_smaller_than__event_type__USER_TYPE_START);

} // namespace

#endif /* SAX_EVENTS_H_ */
