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
// cached timezone offset for g_localtime()
extern int timezone_offset;
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
	__LOGGING_LOG_SERIALIZE(x, buf, serializer); \
	logger->log(serializer.data(), serializer.length())

#define __LOGGING_LOG_LARGE_LOG(logger, size, x) \
	char* buf = new char[size]; \
	__LOGGING_LOG_SYNC(logger, x, buf); \
	delete[] buf

#define __LOGGING_LOG_DO(logger, size_level, x) \
	char buf[size_level]; \
	__LOGGING_LOG_SYNC(logger, x, buf)
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


#define LOGGING_SCOPE(logger, level) \
	if (__LOGGING_GET_LEVEL(logger) <= sax::logger::level)

#define __LOG_BASE(logger, x, level, file, line_num) \
	LOGGING_SCOPE(logger, level) { \
		using namespace sax::logger; \
		/*log_header: "[20120822 11:34:27.456789][TRACE][filename.cpp:123] "*/ \
		char log_header[26 /*datetime*/ + \
		                (2+sizeof(#level)-4-1) /*log level, minus "SAX_"*/ + \
						(3+sizeof(file)-1+sizeof(#line_num)-1) /*source*/ + \
						1 /*space*/ + 1 /*'\0'*/]; \
		uint64_t now_us = g_now_us(); \
		time_t now_sec = (time_t) (now_us / 1000000); \
		struct tm st; \
		g_localtime(&now_sec, &st, timezone_offset); \
		sprintf(log_header, "[%04d%02d%02d %02d:%02d:%02d.%06d][%s][%s:%d] ", \
				st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, \
				st.tm_hour, st.tm_min, st.tm_sec, (int) (now_us % 1000000), \
				#level + 4, file, line_num); \
		__LOGGING_LOG(logger, log_header << x); \
	}

#define LOG_TRACE(logger, x) __LOG_BASE(logger, x, SAX_TRACE, __FILE__, __LINE__)
#define LOG_DEBUG(logger, x) __LOG_BASE(logger, x, SAX_DEBUG, __FILE__, __LINE__)
#define LOG_INFO(logger, x)  __LOG_BASE(logger, x, SAX_INFO,  __FILE__, __LINE__)
#define LOG_WARN(logger, x)  __LOG_BASE(logger, x, SAX_WARN,  __FILE__, __LINE__)
#define LOG_ERROR(logger, x) __LOG_BASE(logger, x, SAX_ERROR, __FILE__, __LINE__)


#endif /* _SAX_LOGGER_H_ */
