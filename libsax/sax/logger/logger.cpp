/*
 * logger.cpp
 *
 *  Created on: 2012-8-22
 *      Author: x
 */

#include <stdio.h>
#include <time.h>
#include "sax/compiler.h"
#include "sax/os_api.h"
#include "sax/sysutil.h"
#include "log_file_writer.h"
#include "logger.h"

namespace sax {
namespace logger {


log_file_writer<mutex_type>* __global_sync_logger =
		new log_file_writer<mutex_type>("stdout",
				100, (size_t) -1, /* it doesn't matter */
				SAX_TRACE);


bool init_global_sync_logger(const std::string& logfile_name,
		int32_t max_logfiles, size_t size_per_logfile, log_level level)
{
	log_file_writer<mutex_type>* old = __global_sync_logger;
	__global_sync_logger = new log_file_writer<mutex_type>(
			logfile_name, max_logfiles, size_per_logfile, level);

	if (__global_sync_logger != NULL) {
		if (old) delete old;
		return true;
	}

	__global_sync_logger = old;
	return false;
}

void destroy_global_sync_logger()
{
	if (__global_sync_logger != NULL) {
		delete __global_sync_logger;
	}
}

////////////////////////////////////////////////////////////////////

thread_local log_state __state = {0};

////////////////////////////////////////////////////////////////////

static void fast_int2str(int32_t num, char* str, int32_t digits)
{
	char* p = str + digits - 1;
	for (int32_t i = 0; i < digits; i++) {
		*p = (char) (num % 10) + '0';
		num /= 10;
		--p;
	}
}

void update_log_buf(int64_t timestamp_us, log_state& state)
{
	/*time format: [20120822 11:34:27.456789]*/
	int64_t secs = timestamp_us / 1000000;
	int32_t us_part = (int32_t) (timestamp_us % 1000000);
	int64_t minutes = secs / 60;
	int32_t second = (int32_t) (secs % 60);

	if (UNLIKELY(minutes != state.last_minutes)) {
		struct tm st;
		fast_localtime(secs, &st);

		/*date=20120822*/
		int32_t date = (st.tm_year + 1900) * 10000 +
				(st.tm_mon + 1) * 100 +
				st.tm_mday;

		/*time=11034027, and then rewrite '0' as ':'*/
		int32_t time = st.tm_hour * 1000000 +
				st.tm_min * 1000 +
				st.tm_sec;

		// "_20120822_________________"
		fast_int2str(date, state.log_buf + 1, 8);

		// "_20120822_11034027________"
		fast_int2str(time, state.log_buf + 10, 8);

		// "_20120822_11034027_456789_"
		fast_int2str(us_part, state.log_buf + 19, 6);

		state.log_buf[0]  = '[';	// "[20120822_11034027_456789_"
		state.log_buf[9]  = ' ';	// "[20120822 11034027_456789_"
		state.log_buf[12] = ':';	// "[20120822 11:34027_456789_"
		state.log_buf[15] = ':';	// "[20120822 11:34:27_456789_"
		state.log_buf[18] = '.';	// "[20120822 11:34:27.456789_"
		state.log_buf[25] = ']';	// "[20120822 11:34:27.456789]"

		if (UNLIKELY(state.last_minutes == 0)) {
			// the first time update log_buf, write the tid part
			int32_t tid = (int32_t) g_thread_id();
			fast_int2str(tid, state.log_buf + LOG_TIME_LEN + 1, 5);
			state.log_buf[LOG_TIME_LEN] = '[';
			state.log_buf[LOG_TIME_LEN + 6] = ']';
		}

		state.last_minutes = minutes;
		state.last_second = second;
	}
	else {
		// just need to update second and microsecond part
		if (UNLIKELY(state.last_second != second)) {
			state.last_second = second;
			fast_int2str(second * 10000000 + us_part,
					state.log_buf + 16, 9);
			state.log_buf[18] = '.';
		}
		else {
			fast_int2str(us_part, state.log_buf + 19, 6);
		}
	}
}

////////////////////////////////////////////////////////////////////
static int s_tz_offset = g_timezone() * 3600;

#ifdef __isleap
#undef __isleap
#define __isleap(year) \
	((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))
#endif//__isleap

static const uint16_t __mon_yday[2][13] =
{
	/* Normal years.  */
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
	/* Leap years.  */
	{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};

#define SECS_PER_HOUR  (60 * 60)
#define SECS_PER_DAY   (SECS_PER_HOUR * 24)

#define DIV(a, b) ((a) / (b) - ((a) % (b) < 0))
#define LEAPS_THRU_END_OF(y) (DIV (y, 4) - DIV (y, 100) + DIV (y, 400))

/*ported from glibc 2.16.0*/
void fast_localtime(int64_t unix_sec, struct tm* st)
{
	unix_sec += s_tz_offset;
	int64_t days = unix_sec / SECS_PER_DAY;
	int64_t rem  = unix_sec % SECS_PER_DAY;

	st->tm_hour = rem / SECS_PER_HOUR;
	rem %= SECS_PER_HOUR;
	st->tm_min = rem / 60;
	st->tm_sec = rem % 60;

	int y = 1970;
	do {
		/* Guess a corrected year, assuming 365 days per year.  */
		int64_t yg = y + days / 365 - (days % 365 < 0);

		/* Adjust DAYS and Y to match the guessed year.  */
		days -= ((yg - y) * 365
				+ LEAPS_THRU_END_OF (yg - 1)
				- LEAPS_THRU_END_OF (y - 1));
		y = yg;
	} while (days < 0 || days >= (__isleap(y) ? 366 : 365));
	st->tm_year = y - 1900;
	const uint16_t* ip = __mon_yday[__isleap(y)];
	for (y = 11; days < (int64_t) ip[y]; --y) continue;
	days -= ip[y];
	st->tm_mon = y;
	st->tm_mday = days + 1;
}

} // namespace logger
} // namespace sax
