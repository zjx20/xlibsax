#ifndef __LOGGER_H_2011__
#define __LOGGER_H_2011__

/// @file logger.h
/// @author Qingshan
/// @date 2011.9.15
/// @brief a simple logging/stat system implemented by stage,
/// which supports both printf-style and stream-style
/// 
/// @remark it's easy to use this class in projects, @n
/// Example (1), logging:
/// <pre>
///   // preparing, declare a logger object:
///   #include <sax/logger.h>
///   sax::logger log;
///   log.start("log/mytest", 1024*1024*10, 5);
///   log.setLevel(sax::LEVEL_DEBUG);
///   ...  ...  ...
///   LOG_WARN(&log, "got: %f, (PI)", 3.14f); //printf-style
///   ...  ...  ...
///   LOG(WARN,&log) << "got: " << 3.14 << ", (PI)"; // stream
///   ...  ...  ...
///   log.halt(); // terminate its actions
/// </pre>
/// Example (2), stat:
/// <pre>
///   uint64_t key = log.markBegin("SendMsg");
///   ...  ...  ...
///   log.markDone(key, true);
/// </pre>

#include "os_types.h"
#include <stdlib.h>

// Make GCC quiet.
#ifdef __DEPRECATED
#undef __DEPRECATED
#include <strstream>
#define __DEPRECATED
#else
#include <strstream>
#endif

namespace sax {

enum LEVEL_ENUM {
	LEVEL_TRACE=0, 
	LEVEL_DEBUG=1, 
	LEVEL_INFO=2, 
	LEVEL_WARN=3, 
	LEVEL_ERROR=4, 
	LEVEL_OFF=5
};

/// @brief declare logger structure
struct logger
{
public:
	friend class logger_helper;
	friend class logger_stream;
	inline logger() : _level(LEVEL_TRACE), _biz(NULL) {}
	~logger();
	
	bool start(const char *path, size_t size, size_t num);
	void halt();
	
	void setLevel(LEVEL_ENUM neu);
	void output(LEVEL_ENUM level, 
		const char *file, int line, const char *fmt, ...);
	
	uint64_t markBegin(const char *cmd);
	bool markDone(uint64_t key, bool succeed);
	
protected:
	bool dispatch(const char *msg, size_t len);

private:
	LEVEL_ENUM _level;
	struct stage *_biz;
};

/// a helper class to walk around the variadic macros
class logger_helper {
public:
	inline logger_helper(LEVEL_ENUM level, const char *file, 
		int line) : _level(level), _file(file), _line(line) {}
	void operator ()(logger *log, char *fmt, ...);
private:
	LEVEL_ENUM _level; 
	const char *_file; 
	int _line;
};

/// @brief a helper class for stream-output
class logger_stream {
public:
	logger_stream(LEVEL_ENUM level, 
		int line, const char *file, logger *log);
	~logger_stream();
	inline std::ostream &stream() { return *_os; }
private:
	LEVEL_ENUM _level; 
	int _line;
	const char *_file; 
	logger *_log; 
	std::ostrstream *_os;
	char _buf[8*1024];
};

} // namespace


// Interface (1): some useful macros for printf-style loggings:
#if defined(__GNUC__) 

// variadic macros: just for C99
#define LOG_TRACE(log, fmt, ...) (log)->output( \
	sax::LEVEL_TRACE, __FILE__, __LINE__, fmt, ## __VA_ARGS__)
#define LOG_DEBUG(log, fmt, ...) (log)->output( \
	sax::LEVEL_DEBUG, __FILE__, __LINE__, fmt, ## __VA_ARGS__)
#define LOG_INFO(log, fmt, ...)  (log)->output( \
	sax::LEVEL_INFO,  __FILE__, __LINE__, fmt, ## __VA_ARGS__)
#define LOG_WARN(log, fmt, ...)  (log)->output( \
	sax::LEVEL_WARN,  __FILE__, __LINE__, fmt, ## __VA_ARGS__)
#define LOG_ERROR(log, fmt, ...) (log)->output( \
	sax::LEVEL_ERROR, __FILE__, __LINE__, fmt, ## __VA_ARGS__)

#else // NON-gcc

#define LOG_TRACE sax::logger_helper(sax::LEVEL_TRACE, __FILE__, __LINE__)
#define LOG_DEBUG sax::logger_helper(sax::LEVEL_DEBUG, __FILE__, __LINE__)
#define LOG_INFO  sax::logger_helper(sax::LEVEL_INFO,  __FILE__, __LINE__)
#define LOG_WARN  sax::logger_helper(sax::LEVEL_WARN,  __FILE__, __LINE__)
#define LOG_ERROR sax::logger_helper(sax::LEVEL_ERROR, __FILE__, __LINE__)

#endif //__GNUC__


// Interface (2): a useful macro for stream-style loggings:
#define LOG(severity, log) sax::logger_stream( \
	sax::LEVEL_##severity, __LINE__, __FILE__, log).stream()


#endif//__LOGGER_H_2011__

