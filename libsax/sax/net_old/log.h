#ifndef __SAX_LOG_H__
#define __SAX_LOG_H__

/**
 * @file log.h
 * @brief log util, ported from tb-common-utils/tbsys/tblog
 *
 * 支持的特性：
 * 1. 默认将log写到标准错误流
 * 2. printf风格的log
 * 3. 写log时自动检测log文件是否存在，如不存在则重建
 * 4. 可设置单个log文件的最大大小，超过时自动重命名log文件，后面的log会记录在同名的新文件中
 * 5. 可设置log文件个数（对log文件重命名时会增加文件个数），自动删除最久的log文件
 * 6. 应用程序在编译时加入LOG_OFF宏，可去除目标代码中所有log调用
 * 7. 可重定向标准输出流和标准错误流到log文件
 *
 * 注意事项：
 * 1. 单个可执行程序共用一个logger，写log的调用是线程安全的，但设置logger的调用并非线程安全
 * 2. 目前在linux平台下编译通过，windows平台下未测试
 *
 * @author x_zhou
 * @date 2011-8-5
 */

#include <stdarg.h>
#include <time.h>
#include <stdio.h>
#include <strings.h>

#include <string>
#include <deque>

#include "sax/sysutil.h"
#include "sax/c++/linklist.h"

#if (defined(_MSC_VER) && defined(WIN32)) || (defined(__BORLANDC__) && defined(__WIN32__))
#define _INT64_FMT	"I64d"
#define _UINT64_FMT	"I64u"
#else
#define _INT64_FMT	"lld"
#define _UINT64_FMT	"llu"
#endif

#ifndef LOG_OFF
#define COMPILE_LOG(x) (x)
#else
#define COMPILE_LOG(x)
#endif

#define LOG_LEVEL(level) #level, __FILE__, __LINE__, __FUNCTION__
#define LOGGER sax::logger::_logger
#define LOG_PRINT(level, ...) LOGGER.log_message(LOG_LEVEL(level), __VA_ARGS__)
#define LOG_BASE(level, ...) (level>LOGGER._level) ? (void)0 : LOG_PRINT(level, __VA_ARGS__)
#define LOG(level, _fmt_, args...) COMPILE_LOG(LOG_BASE(level, "[%ld] " _fmt_, g_thread_id(), ##args))
#define LOG_US(level, _fmt_, args...) COMPILE_LOG(LOG_BASE(level, "[%ld]["_INT64_FMT"] " _fmt_, g_thread_id(), g_now_us(), ##args))

#define LOG_ERROR(...) LOG(sax::ERROR, __VA_ARGS__)
#define LOG_WARN(...) LOG(sax::WARN, __VA_ARGS__)
#define LOG_INFO(...) LOG(sax::INFO, __VA_ARGS__)
#define LOG_DEBUG(...) LOG(sax::DEBUG, __VA_ARGS__)

#define LOG_SET_LOG_LEVEL(level) COMPILE_LOG(LOGGER.set_log_level(level))
#define LOG_SET_LOG_FILE(filename) COMPILE_LOG(LOGGER.set_log_file(filename))
#define LOG_SET_CHECK_LOG_FILE(v) COMPILE_LOG(LOGGER.set_check_log_file(v))
#define LOG_SET_MAX_LOG_FILE_SIZE(size) COMPILE_LOG(LOGGER.set_max_log_file_size(size))
#define LOG_SET_MAX_LOG_FILE_NUM(num) COMPILE_LOG(LOGGER.set_max_log_file_num(num))
#define LOG_REDIRECT_STDOUT() COMPILE_LOG(LOGGER.redirect_stdout())
#define LOG_REDIRECT_STDERR() COMPILE_LOG(LOGGER.redirect_stderr())

namespace sax {

#undef ERROR
#undef WARN
#undef INFO
#undef DEBUG
#undef CLOSE

enum log_level {
	ERROR = 0, WARN, INFO, DEBUG,

	CLOSE
/* be the last one */
};

class logger {
public:
	logger();
	~logger();

	void log_message(const char *level, const char *file, int line,
	        const char *function, const char *fmt, ...);

	        void set_log_level(log_level level);

	        void set_log_file(const std::string &filename);
	        void set_check_log_file(bool v) {_check_log_file = v;}

	        void redirect_stdout();
	        void redirect_stderr();

	        void set_max_log_file_size(uint64_t max_log_file_size=0x40000000);

	        // negative or zero for not delete old log file
	        void set_max_log_file_num(int16_t max_log_file_num= -1);

        private:
	        void rotate_log();
	        bool check_log_file();
	        void close_log_file();

        private:
	        FILE *_log_file;
	        std::string _log_file_name;
	        bool _check_log_file;
	        int16_t _max_log_file_num;
	        int64_t _max_log_file_size;
	        bool _redirect_stdout;
	        bool _redirect_stderr;
	        std::deque<std::string> _file_list;
	        mutex_type _file_mutex;

        public:
	        static logger _logger;
	        log_level _level;
        };

        } // namespace

#endif	// __SAX_LOG_H__
