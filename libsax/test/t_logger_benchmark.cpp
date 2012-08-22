/*
 * t_logger_benchmark.cpp
 *
 *  Created on: 2012-8-22
 *      Author: x
 */
//#define LOGGING_ASYNC 1
#include "sax/logger/logger.h"

#if !defined(LOGGING_ASYNC)
sax::logger::log_file_writer<sax::mutex_type>* logger =
		new sax::logger::log_file_writer<sax::mutex_type>(
				"t_logger_benchmark.txt", 10, 100 * 1024 * 1024, sax::logger::SAX_TRACE);
#else
sax::stage* logger = sax::stage_creator<sax::logger::log_handler>::create_stage(
		"logging", 1, NULL, 10*1024*1024, new sax::single_dispatcher());
#endif

void* log(void* param)
{
	size_t& counter = *(size_t*) param;

	while (1) {
		int choose = rand() % 5;
		switch(choose)
		{
		case 0:
		{
			LOG_TRACE(logger, "fasdfjkawue" << 34879204 << 123.764646 << ' ' << counter);
			break;
		}
		case 1:
		{
			LOG_DEBUG(logger, "fasdfjkawuesdfadfjakl;" << 34879204 << 123.764646 << ' ' << counter);
			break;
		}
		case 2:
		{
			LOG_INFO(logger, "fasdfjkawuevxcvmzkljkl;ujdl;ajjkl;sfj;" << 34879204 << 123.764646 << ' ' << counter);
			break;
		}
		case 3:
		{
			LOG_WARN(logger, "fasdfjkawueqw3rjklj;klsdvjkls;vuinmefklasdfjakl;fjakl;jdfakl;sjdf;" << 34879204 << 123.764646 << ' ' << counter);
			break;
		}
		case 4:
		{
			LOG_ERROR(logger, "fasdfjkawue123456v415aw34f56a1f3a41331f3as57e8f6d1534f56asd4fas2d3f13asd4fsdfasdfasdf" << 34879204 << 123.764646 << ' ' << counter);
			break;
		}
		}

		counter++;
	}

	return 0;
}

int main()
{
	const int threads = 2;
	volatile size_t counter[threads] = {0};
	for (int i=0; i<threads; i++) {
		g_thread_start(log, (void*) &counter[i]);
	}

	size_t last_sum = 0;
	while (1) {
		size_t sum = 0;
		for (int i=0; i<threads; i++) {
			sum += counter[i];
		}
		printf("%lu\n", sum - last_sum);
		last_sum = sum;
		g_thread_sleep(1.0);
	}

	return 0;
}
