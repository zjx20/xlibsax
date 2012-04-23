#include <stdio.h>
#include <sax/logger.h>
#include <sax/os_api.h>
int main( int argc, char *argv[] )
{
	sax::logger log;
	log.start("abcd", 1024*16, 5);
	log.setLevel(sax::LEVEL_DEBUG);
	LOG_TRACE(&log, "Hello World!");
	
	g_thread_sleep(0.05);
	LOG_DEBUG(&log, "Hello World!");
	
	g_thread_sleep(0.05);
	LOG_INFO(&log, "Hello World: %d", 1);
	g_thread_sleep(0.05);
	LOG_WARN(&log, "Hello World: %f, (PI)", 3.14f);
	g_thread_sleep(0.05);
	LOG_ERROR(&log, "Hello World!");
	
	g_thread_sleep(0.05);
	LOG(INFO, &log) << "Hello World: " << 1;
	g_thread_sleep(0.05);
	LOG(WARN, &log) << "Hello World: " << 3.14 << ", (PI)";
	g_thread_sleep(0.05);
	LOG(ERROR, &log) << "Hello World!";
	
	for(int i=0; i<10; i++) {
		LOG_INFO(&log, "%d: %s", i, "12345678901234567890123456789012345678901234567890");
		g_thread_sleep(.05);
	}
	getchar();
	return 0;
}
