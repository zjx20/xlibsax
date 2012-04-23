#include <sax/os_api.h>
#include <stdio.h>

int main( int argc, char *argv[] )
{
	printf( "zone=%d!\n", g_timezone());
	
	printf( "t1=%" PRId64 ", t2=%" PRId64 "!\n", g_now_hr(), g_now_us());
	g_thread_sleep(0.050);
	printf( "t1=%" PRId64 ", t2=%" PRId64 "!\n", g_now_hr(), g_now_us());
	g_thread_sleep(0.200);
	printf( "t1=%" PRId64 ", t2=%" PRId64 "!\n", g_now_hr(), g_now_us());
	g_thread_sleep(0.500);
	printf( "t1=%" PRId64 ", t2=%" PRId64 "!\n", g_now_hr(), g_now_us());
	g_thread_sleep(0.500);
	printf( "t1=%" PRId64 ", t2=%" PRId64 "!\n", g_now_hr(), g_now_us());
	g_thread_sleep(0.500);
	printf( "t1=%" PRId64 ", t2=%" PRId64 "!\n", g_now_hr(), g_now_us());
	g_thread_sleep(0.500);
	printf( "t1=%" PRId64 ", t2=%" PRId64 "!\n", g_now_hr(), g_now_us());
	return 0;
}
