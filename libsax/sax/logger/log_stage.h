/*
 * log_stage.h
 *
 *  Created on: 2012-8-22
 *      Author: x
 */

#ifndef _SAX_LOG_STAGE_H_
#define _SAX_LOG_STAGE_H_

#include "sax/stage/stage.h"
#include "sax/stage/sax_events.h"

namespace sax {
namespace logger {

class log_handler : public handler_base
{
public:
	virtual bool init(void* param)
	{
		// fixme
		logger = new log_file_writer<sax::dummy_lock>("t_logger_test.txt", 10, 100*1024*1024, SAX_TRACE);
		return true;
	}
	virtual void on_event(const sax::event_type *ev)
	{
		switch(ev->get_type())
		{
		case sax::log_event128::ID:
		{
			sax::log_event128* event = (sax::log_event128*) ev;
			logger->log(event->body, strlen(event->body));
			break;
		}
		case sax::log_event256::ID:
		{
			sax::log_event256* event = (sax::log_event256*) ev;
			logger->log(event->body, strlen(event->body));
			break;
		}
		case sax::log_event512::ID:
		{
			sax::log_event512* event = (sax::log_event512*) ev;
			logger->log(event->body, strlen(event->body));
			break;
		}
		}
	}
private:
	log_file_writer<sax::dummy_lock>* logger;
};

class log_stage : public sax::stage
{
public:

};

} // namespace logger
} // namespace sax


#endif /* _SAX_LOG_STAGE_H_ */
