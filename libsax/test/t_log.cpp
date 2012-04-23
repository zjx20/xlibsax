/*
 * t_log.cpp
 *
 *  Created on: 2011-8-5
 *      Author: x_zhou
 */

#include "sax/net/log.h"
#include "sax/os_api.h"

#include <stdio.h>

using namespace sax;

int main() {
	LOG_SET_LOG_LEVEL(INFO);

	LOG(INFO, "print message to stderr");
	LOG(DEBUG, "should be filtered");

	LOG_SET_LOG_FILE("test.log");
	printf("print message to stdout.\n");
	LOG_REDIRECT_STDOUT();
	LOG_REDIRECT_STDERR();
	printf("redirect stdout & stderr to log file\n");

	LOG_SET_CHECK_LOG_FILE(true);

	int t;
	LOG_SET_MAX_LOG_FILE_SIZE(60);
	char msg[51] = "12345678901234567890123456789012345678901234567890";
	LOG(INFO, "test rotate_log %s", msg);

	LOG_SET_MAX_LOG_FILE_NUM(2);
	t = 3;
	while(t--) {
		g_thread_sleep(1.5);	// to avoid rotate to same log file
		LOG(INFO, "test max_log_file_num %s", msg);
	}

	LOG_SET_MAX_LOG_FILE_SIZE(600000);

//	t = 5;
//	while(t--) {
//		g_thread_sleep(10);
//		LOG(INFO, "test delete the log file when biz running");
//	}

	return 0;
}
