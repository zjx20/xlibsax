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
// only convert unix timestamp to date and time (the first 6 fields in struct tm)
void fast_localtime(int64_t unix_sec, struct tm* st);
}
}


#undef __LOGGING_SYNC
#if defined(LOGGING_SYNC)
#define __LOGGING_SYNC 1
#elif defined(LOGGING_ASYNC)
#define __LOGGING_SYNC 0
#include "log_stage.h"
#else
#define __LOGGING_SYNC 1
#endif


#if __LOGGING_SYNC
#define __LOGGING_GET_LEVEL(logger) (logger)->get_log_level()
#else
#define __LOGGING_GET_LEVEL(logger) sax::logger::SAX_TRACE // TODO: fixme
#endif


#define __LOGGING_GET_LOG_SIZE(x, size) \
	if (sizeof(log_size_helper() << x << static_checker()) == \
			sizeof(static_checker_helper<true>)) \
	{ size = sizeof(log_size_helper() << x << log_size_traits()); } \
	else \
	{ size = (size_t) (log_size_helper() << x << log_size_traits()); } \
	++size; /*for '\n'*/ ++size /*for '\0'*/


#define __LOGGING_LOG_SERIALIZE(x, buf, serializer_name) \
	log_serializer serializer_name(buf); \
	serializer_name << x << log_serializer_eol()

#if __LOGGING_SYNC
#define __LOGGING_LOG_SYNC(logger, x, buf) \
	__LOGGING_LOG_SERIALIZE(x, buf, __sax_logging_serializer); \
	logger->log(__sax_logging_serializer.data(), __sax_logging_serializer.length())

#define __LOGGING_LOG_LARGE_LOG(logger, size, x) \
	char* __sax_logging_buf = new char[size]; \
	__LOGGING_LOG_SYNC(logger, x, __sax_logging_buf); \
	delete[] __sax_logging_buf

#define __LOGGING_LOG_DO(logger, size_level, x) \
	char __sax_logging_buf[size_level]; \
	__LOGGING_LOG_SYNC(logger, x, __sax_logging_buf)
#else
#define __LOGGING_LOG_LARGE_LOG(logger, size, x) // TODO: fixme
#define __LOGGING_LOG_DO(logger, size_level, x) \
	sax::log_event##size_level* ev = logger->allocate_event<sax::log_event##size_level>(); \
	__LOGGING_LOG_SERIALIZE(x, ev->body, serializer); \
	logger->push_event(ev)
#endif

#define __LOGGING_LOG(logger, x) \
	size_t size; \
	__LOGGING_GET_LOG_SIZE(x, size); \
	if (UNLIKELY(size > 8192)) { \
		__LOGGING_LOG_LARGE_LOG(logger, size, x); \
	} else { \
		if (LIKELY(size <= 1024)) { \
			if (LIKELY(size <= 256)) { \
				if (LIKELY(size <= 128)) { \
					__LOGGING_LOG_DO(logger, 128, x); \
				} else { \
					__LOGGING_LOG_DO(logger, 256, x); \
				} \
			} else { \
				if (size <= 512) { \
					__LOGGING_LOG_DO(logger, 512, x); \
				} else { \
					__LOGGING_LOG_DO(logger, 1024, x); \
				} \
			} \
		} else { \
			if (size <= 4096) { \
				if (size <= 2048) { \
					__LOGGING_LOG_DO(logger, 2048, x); \
				} else { \
					__LOGGING_LOG_DO(logger, 4096, x); \
				} \
			} else { \
				__LOGGING_LOG_DO(logger, 8192, x); \
			} \
		} \
	}


#define LOGGING_SCOPE(logger_, level) \
	if (__LOGGING_GET_LEVEL(logger_) <= sax::logger::level)

#ifdef LOG_OFF
#define __LOG_BASE(logger_, x, level, file, line_num)
#else
#define __LOG_BASE(logger_, x, level, file, line_num) \
	LOGGING_SCOPE(logger_, level) { \
		using namespace sax::logger; \
		/*log_header: "[20120822 11:34:27.456789][TRACE][filename.cpp:123] "*/ \
		char log_header[26 /*datetime*/ + \
		                (2+sizeof(#level)-4-1) /*log level, minus "SAX_"*/ + \
						(3+sizeof(file)-1+sizeof(#line_num)-1) /*source*/ + \
						1 /*space*/ + 1 /*'\0'*/]; \
		int64_t now_us = g_now_us(); \
		struct tm st; \
		fast_localtime(now_us / 1000000, &st); \
		sprintf(log_header, "[%04d%02d%02d %02d:%02d:%02d.%06d][%s][%s:%d] ", \
				st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, \
				st.tm_hour, st.tm_min, st.tm_sec, (int) (now_us % 1000000), \
				#level + 4, file, line_num); \
		__LOGGING_LOG(logger_, log_header << x); \
	}
#endif

#define LOGP_TRACE(logger, x) __LOG_BASE(logger, x, SAX_TRACE, __FILE__, __LINE__)
#define LOGP_DEBUG(logger, x) __LOG_BASE(logger, x, SAX_DEBUG, __FILE__, __LINE__)
#define LOGP_INFO(logger, x)  __LOG_BASE(logger, x, SAX_INFO,  __FILE__, __LINE__)
#define LOGP_WARN(logger, x)  __LOG_BASE(logger, x, SAX_WARN,  __FILE__, __LINE__)
#define LOGP_ERROR(logger, x) __LOG_BASE(logger, x, SAX_ERROR, __FILE__, __LINE__)


namespace sax {
namespace logger {
#if __LOGGING_SYNC
extern log_file_writer<mutex_type>* __global_sync_logger;
#define __GLOBAL_LOGGER sax::logger::__global_sync_logger

bool init_global_sync_logger(const std::string& logfile_name,
		int32_t max_logfiles, size_t size_per_logfile, log_level level);

#ifdef LOG_OFF
#define INIT_GLOBAL_LOGGER(...)
#else
#define INIT_GLOBAL_LOGGER(...) sax::logger::init_global_sync_logger(__VA_ARGS__)
#endif

#else
#endif
}
}


#define LOG_TRACE(x) LOGP_TRACE(__GLOBAL_LOGGER, x)
#define LOG_DEBUG(x) LOGP_DEBUG(__GLOBAL_LOGGER, x)
#define LOG_INFO(x)  LOGP_INFO( __GLOBAL_LOGGER, x)
#define LOG_WARN(x)  LOGP_WARN( __GLOBAL_LOGGER, x)
#define LOG_ERROR(x) LOGP_ERROR(__GLOBAL_LOGGER, x)


#endif /* _SAX_LOGGER_H_ */
