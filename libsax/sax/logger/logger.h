/*
 * logger.h
 *
 *  Created on: 2012-8-13
 *      Author: x
 */

#ifndef _SAX_LOGGER_H_
#define _SAX_LOGGER_H_

#include <ctime>
#include "log_size_helper.h"
#include "log_serializer.h"
#include "log_file_writer.h"


namespace sax {
namespace logger {

const int32_t MAX_LOG_SIZE = 8 * 1024;
const int32_t LOG_TIME_LEN = sizeof("[20120822 11:34:27.456789]") - 1;
const int32_t LOG_TID_LEN = sizeof("[12345]") - 1;	// 5 digits

struct log_state
{
	int64_t last_minutes;
	int32_t last_second;
	char log_buf[MAX_LOG_SIZE];
};

extern thread_local log_state __state;

// only convert unix timestamp to date and time (the first 6 fields in struct tm)
void fast_localtime(int64_t unix_sec, struct tm* st);

void update_log_buf(int64_t timestamp_us, log_state& state);

}
}


#define __LOGGING_GET_LEVEL(logger) (logger)->get_log_level()

#define LOGGING_SCOPE(__logger, level) \
	if (__LOGGING_GET_LEVEL(__logger) <= sax::logger::level)

#ifdef LOG_OFF
#define __LOG_BASE(logger_, x, level, file, line_num, func)
#else

#define __LOG_FILE_LINE_PART(file, line_num) "["file":"#line_num"]"

#define __LOG_BASE(__logger, x, level, file, line_num, func) \
	LOGGING_SCOPE(__logger, level) { \
		using namespace sax::logger; \
		/*log_header: "[20120822 11:34:27.456789][12345][TRACE][filename.cpp:123][func]"*/ \
		int64_t now_us = g_now_us(); \
		update_log_buf(now_us, __state); \
		char* log_buf = __state.log_buf + LOG_TIME_LEN + LOG_TID_LEN; \
		log_buf[0] = '['; \
		memcpy(log_buf + 1, #level + 4, sizeof(#level) - 4 - 1); \
		log_buf[1 + sizeof(#level) - 4 - 1] = ']'; \
		log_buf += 1 + sizeof(#level) - 4 - 1 + 1; \
		memcpy(log_buf, __LOG_FILE_LINE_PART(file, line_num), \
				sizeof(__LOG_FILE_LINE_PART(file, line_num)) - 1); \
		log_buf += sizeof(__LOG_FILE_LINE_PART(file, line_num)) - 1; \
		log_buf[0] = '['; \
		memcpy(log_buf + 1, func, sizeof(func) - 1); \
		log_buf[sizeof(func)-1 + 1] = ']'; \
		log_buf += sizeof(func)-1 + 2; \
		log_serializer serializer(log_buf, \
				MAX_LOG_SIZE - (log_buf - __state.log_buf)); \
		size_t log_size = (int32_t) ((serializer << ' ' << x).end_of_line() - \
				__state.log_buf); \
		__logger->log(__state.log_buf, log_size); \
	}
#endif

#define LOGP_TRACE(logger, x) __LOG_BASE(logger, x, SAX_TRACE, __FILE__, __LINE__, __func__)
#define LOGP_DEBUG(logger, x) __LOG_BASE(logger, x, SAX_DEBUG, __FILE__, __LINE__, __func__)
#define LOGP_INFO(logger, x)  __LOG_BASE(logger, x, SAX_INFO,  __FILE__, __LINE__, __func__)
#define LOGP_WARN(logger, x)  __LOG_BASE(logger, x, SAX_WARN,  __FILE__, __LINE__, __func__)
#define LOGP_ERROR(logger, x) __LOG_BASE(logger, x, SAX_ERROR, __FILE__, __LINE__, __func__)


namespace sax {
namespace logger {

extern log_file_writer<mutex_type>* __global_sync_logger;
#define __GLOBAL_LOGGER sax::logger::__global_sync_logger

bool init_global_sync_logger(const std::string& logfile_name,
		int32_t max_logfiles, size_t size_per_logfile, log_level level);

void destroy_global_sync_logger();

#ifdef LOG_OFF
#define INIT_GLOBAL_LOGGER(...)
#define DESTROY_GLOBAL_LOGGER()
#else
#define INIT_GLOBAL_LOGGER(...) sax::logger::init_global_sync_logger(__VA_ARGS__)
#define DESTROY_GLOBAL_LOGGER() sax::logger::destroy_global_sync_logger()
#endif

}
}


#define LOG_TRACE(x) LOGP_TRACE(__GLOBAL_LOGGER, x)
#define LOG_DEBUG(x) LOGP_DEBUG(__GLOBAL_LOGGER, x)
#define LOG_INFO(x)  LOGP_INFO( __GLOBAL_LOGGER, x)
#define LOG_WARN(x)  LOGP_WARN( __GLOBAL_LOGGER, x)
#define LOG_ERROR(x) LOGP_ERROR(__GLOBAL_LOGGER, x)


#endif /* _SAX_LOGGER_H_ */
