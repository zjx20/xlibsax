/*
 * log_file_writer.h
 *
 *  Created on: 2012-8-22
 *      Author: x
 */

#ifndef _SAX_LOG_FILE_WRITER_H_
#define _SAX_LOG_FILE_WRITER_H_

#include <cstdio>
#include <cassert>
#include <string>
#include "sax/os_types.h"
#include "sax/os_api.h"
#include "sax/compiler.h"
#include "sax/sysutil.h"

namespace sax {
namespace logger {

enum log_level
{
	// adding "SAX_" prefix to prevent compile fail, because "TRACE" can be a macro in some system
	SAX_TRACE = 1,
	SAX_DEBUG,
	SAX_INFO,
	SAX_WARN,
	SAX_ERROR,
	SAX_OFF
};

template <class LOCK_TYPE>
class log_file_writer
{
public:
	log_file_writer(const std::string& logfile_name, int32_t max_logfiles,
			size_t size_per_logfile, log_level level = (log_level)0)
	{
		_logfile_name = logfile_name;
		_max_logfiles = max_logfiles;
		_size_per_logfile = size_per_logfile;
		_level = level;

		_curr_logfiles = 0;

		if (_logfile_name != "stderr" && _logfile_name != "stdout") {
			int32_t i;
			for (i = 1; i < _max_logfiles; i++) {
				char num[12];
				g_snprintf(num, sizeof(num), "%d", i);
				if (!g_exists_file((_logfile_name + '.' + num).c_str())) {
					break;
				}
			}
			_curr_logfiles = i - 1;
		}

		open();
	}

	~log_file_writer()
	{
		close();
	}

	void set_max_logfiles(int32_t amount)
	{
		assert(amount > 0);
		auto_lock<LOCK_TYPE> scoped_lock(_lock);
		_max_logfiles = amount;
	}

	void set_size_per_logfile(size_t amount)
	{
		auto_lock<LOCK_TYPE> scoped_lock(_lock);
		_size_per_logfile = amount;
	}

	void set_logfile_name(const std::string& name)
	{
		auto_lock<LOCK_TYPE> scoped_lock(_lock);

		close();
		_logfile_name = name;
		open();
	}

	void set_log_level(log_level level)
	{
		auto_lock<LOCK_TYPE> scoped_lock(_lock);
		_level = level;
	}

	log_level get_log_level()
	{
		return _level;
	}

	void log(const char* line, size_t size)
	{
		auto_lock<LOCK_TYPE> scoped_lock(_lock);
		fwrite(line, sizeof(char), size, _curr_logfile);
		_curr_size += size;
		if (UNLIKELY(_curr_size >= _size_per_logfile)) {
			rotate();
		}
	}

private:
	void open()
	{
		assert(!_logfile_name.empty());

		if (_logfile_name == "stderr") {
			_curr_logfile = stderr;
			_curr_size = 0;
			return;
		}
		else if (_logfile_name == "stdout") {
			_curr_logfile = stdout;
			_curr_size = 0;
			return;
		}

		FILE* file = fopen(_logfile_name.c_str(), "a");
		if (file == NULL) {
			fprintf(stderr, "[Error] Failed to open file (%s), log to stdout instead.", _logfile_name.c_str());
			_curr_logfile = stdout;
			_curr_size = 0;
			return;
		}

		setvbuf(file, NULL, _IOLBF, BUFSIZ);    // TODO: test performance degradation
		fseek(file, 0, SEEK_END);

		_curr_size = ftell(file);
		_curr_logfile = file;
	}

	void close()
	{
		if (_curr_logfile != NULL &&
				_logfile_name != "stderr" &&
				_logfile_name != "stdout") {
			fclose(_curr_logfile);
			_curr_logfile = NULL;
		}
	}

	void rotate()
	{
		if (_curr_logfile == NULL ||
				_logfile_name == "stderr" ||
				_logfile_name == "stdout") {
			_curr_size = 0;	// avoid rotating every time when log to std streams
			return;
		}

		if (_curr_logfiles < _max_logfiles) _curr_logfiles++;

		char num[12];
		g_snprintf(num, sizeof(num), "%d", _curr_logfiles);
		std::string last = _logfile_name + '.' + num;
		remove(last.c_str());
		for (int32_t i = _curr_logfiles - 1; i > 0; i--) {
			g_snprintf(num, sizeof(num), "%d", i);
			std::string name = _logfile_name + '.' + num;
			rename(name.c_str(), last.c_str());
			last = name;
		}

		close();
		rename(_logfile_name.c_str(), last.c_str());
		open();
	}

private:
	LOCK_TYPE _lock;
	int32_t _max_logfiles;
	size_t _size_per_logfile;
	std::string _logfile_name;

	int32_t _curr_logfiles;
	size_t _curr_size;
	FILE* _curr_logfile;

	log_level _level;
};

} // namespace logger
} // namespace sax

#endif /* _SAX_LOG_FILE_WRITER_H_ */
