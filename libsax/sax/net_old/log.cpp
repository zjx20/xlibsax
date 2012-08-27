/**
 * @file log.cpp
 * @brief log util, ported from tb-common-utils/tbsys/tblog
 *
 * @author x_zhou
 * @date 2011-8-5
 */

#include "log.h"
#include <string.h>

namespace sax {

logger logger::_logger;

logger::logger() {
	_log_file = stderr;
	_level = CLOSE;
	_check_log_file = false;
	_max_log_file_size = 0x040000000;	// 1 gigabyte
	_max_log_file_num = -1;				// do not delete log files by default
	_redirect_stdout = false;
	_redirect_stderr = false;
}

logger::~logger() {
	close_log_file();
}

void logger::set_log_level(log_level level) {
	_level = level;
}

void logger::set_log_file(const std::string &filename) {
	close_log_file();

	_log_file_name = filename;
	FILE *file = fopen(_log_file_name.c_str(), "a+");
	if (file == NULL) return;

	if(_redirect_stdout) freopen(_log_file_name.c_str(), "a", stdout);
	if(_redirect_stderr) freopen(_log_file_name.c_str(), "a", stderr);

	_log_file = file;
}

bool logger::check_log_file() {
	if(_log_file == stderr || _log_file == stdout) return true;
	if(!_log_file_name.empty()) return g_exists_file(_log_file_name.c_str());
	return false;
}

void logger::close_log_file() {
	if(!_log_file && _log_file != stderr && _log_file != stdout) {
		fclose(_log_file);
		_log_file = NULL;
	}
}

void logger::log_message(const char *level, const char *file, int line,
        const char *function, const char *fmt, ...) {
	_file_mutex.enter();

    if (_check_log_file && !check_log_file()) {
    	if(!_log_file_name.empty()) set_log_file(_log_file_name);	// log file may has been deleted
    	else return;
    }

	time_t t = time(NULL);
	struct tm tm;
	localtime_r((const time_t*)&t, &tm);

	char tmpdata[1000];
	char buffer[1100];

	va_list args;
	va_start(args, fmt);
	vsnprintf(tmpdata, sizeof(tmpdata), fmt, args);
	va_end(args);

	size_t size = snprintf(buffer, sizeof(buffer),
			"[%04d-%02d-%02d %02d:%02d:%02d] %-5s %s (%s:%d) %s\n",
			tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			level, function, file, line, tmpdata);

	// remove '\n' at tail of buffer
    while (size >= 2 && buffer[size-2] == '\n') size--;
    buffer[size] = '\0';

    size_t pos = 0;
    while (size > 0) {
    	size_t success = fwrite(buffer + pos, sizeof(buffer[0]), size, _log_file);
        pos += success;
        size -= success;
    }

    if(_log_file == stdout || _log_file == stderr) fflush(_log_file);

    if (_log_file != stdout && _log_file != stderr && _max_log_file_size) {
    	int64_t offset = ftell(_log_file);
		if (offset >= _max_log_file_size) {
			rotate_log();
		}
    }

    _file_mutex.leave();
}

void logger::rotate_log() {
	close_log_file();
	char new_name[256];
	time_t t = time(NULL);
	struct tm tm;
	localtime_r(&t, &tm);
	sprintf(new_name, "%s.%04d%02d%02d%02d%02d%02d", _log_file_name.c_str(),
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
			tm.tm_min, tm.tm_sec);

	if (_max_log_file_num > 0 && _file_list.size() >= (size_t)_max_log_file_num) {
		std::string old_file = _file_list.front();
		_file_list.pop_front();
		remove(old_file.c_str());
	}
	rename(_log_file_name.c_str(), new_name);
	_file_list.push_back(new_name);

	set_log_file(_log_file_name);	// reopen log file
}

void logger::set_max_log_file_size(uint64_t size) {
	if (size > 0x40000000) size = 0x40000000; //1GB
	_max_log_file_size = size;
}

void logger::set_max_log_file_num(int16_t num) {
	_max_log_file_num = num;
}

void logger::redirect_stdout() {
	_redirect_stdout = true;
	if(!_log_file_name.empty())
		freopen(_log_file_name.c_str(), "a+", stdout);
}

void logger::redirect_stderr() {
	_redirect_stderr = true;
	if(!_log_file_name.empty())
		freopen(_log_file_name.c_str(), "a+", stderr);
}

}

/////////////
