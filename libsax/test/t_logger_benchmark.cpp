/*
 * t_logger_benchmark.cpp
 *
 *  Created on: 2012-8-22
 *      Author: x
 */
//#define LOGGING_ASYNC 1
#include "sax/logger/logger.h"

#if !defined(LOGGING_ASYNC)
sax::logger::log_file_writer<sax::spin_type>* logger =
		new sax::logger::log_file_writer<sax::spin_type>(
				"t_logger_benchmark.txt", 10, 1000 * 1024 * 1024, sax::logger::SAX_TRACE);
#else
sax::stage* logger = sax::stage_creator<sax::logger::log_handler>::create_stage(
		"logging", 1, NULL, 10*1024*1024, new sax::single_dispatcher());
#endif

volatile bool finished = false;

void* log(void* param)
{
	size_t& counter = *(size_t*) param;

	while (!finished) {
		int choose = rand() % 5;
		switch(choose)
		{
		case 0:
		{
			LOGP_TRACE(logger, "fasdfjkawue " << 0 << 34879204 << ' ' << 123.764646 << ' ' << counter);
			break;
		}
		case 1:
		{
			LOGP_DEBUG(logger, "fasdfjkawuesdfadfjakl; " << (int32_t) (1<<31) << ' ' << 34879204 << 123.764646 << ' ' << counter);
			break;
		}
		case 2:
		{
			LOGP_INFO(logger, "fasdfjkawuevxcvmzkljkl;ujdl;ajjkl;sfj;" << 34879204 << 123.764646 << ' ' << counter);
			break;
		}
		case 3:
		{
			LOGP_WARN(logger, "fasdfjkawueqw3rjklj;klsdvjkls;vuinmefklasdfjakl;fjakl;jdfakl;sjdf;" << 34879204 << 123.764646 << ' ' << counter);
			break;
		}
		case 4:
		{
			LOGP_ERROR(logger, "fasdfjkawue123456v415aw34f56a1f3a41331f3as57e8f6d1534f56asd4fas2d3f13asd4fsdfasdfasdf" << 34879204 << 123.764646 << ' ' << counter);
			break;
		}
		}

		counter++;
	}

	return 0;
}

int main()
{
	const int threads = 8;
	volatile size_t counter[threads] = {0};
	g_thread_t handles[threads];
	for (int i=0; i<threads; i++) {
		handles[i] = g_thread_start(log, (void*) &counter[i]);
	}

	size_t last_sum = 0;
	while (1) {
		size_t sum = 0;
		for (int i=0; i<threads; i++) {
			sum += counter[i];
		}
		printf("%lu %lu\n", sum, sum - last_sum);
		last_sum = sum;
		g_thread_sleep(1.0);
		if (sum > 5000000) break;
	}

	finished = true;

	for (int i=0; i<threads; i++) {
		g_thread_join(handles[i], NULL);
	}

	delete logger;

//	tm t;
//
//	clock_t start = clock();
//	int64_t now = g_now_us() / 1000000;
//	for (int i=0;i<10000000;i++) {
//		g_localtime(now, &t, 8 * 3600);
//	}
//	clock_t end = clock();
//
//	printf("g_localtime: %lu clocks\n", end - start);
//
//	start = clock();
//	now = g_now_us() / 1000000;
//	for (int i=0;i<10000000;i++) {
//		sax::logger::fast_localtime(now, &t);
//	}
//	end = clock();
//
//	printf("fast_localtime: %lu clocks\n", end - start);

	return 0;
}
