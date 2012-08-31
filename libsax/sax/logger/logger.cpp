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

namespace sax {
namespace logger {


log_file_writer<mutex_type>* __global_sync_logger = NULL;


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

bool init_global_sync_logger(const std::string& logfile_name,
		int32_t max_logfiles, size_t size_per_logfile, log_level level)
{
	__global_sync_logger = new log_file_writer<mutex_type>(
			logfile_name, max_logfiles, size_per_logfile, level);
	return __global_sync_logger != NULL;
}

} // namespace logger
} // namespace sax
