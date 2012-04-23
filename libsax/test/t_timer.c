#include <sax/os_api.h>
#include <sax/timer.h>

#include <stdio.h>

static void echo(uint32_t id, void *client, void *param)
{
	printf("echo: %u   %p  %p\n", id, client, param);
}

int main( int argc, char *argv[] )
{
	g_timer_t *stw = g_timer_create(4, 50, 20);
	printf( "Hello: %p!\n", stw);
	printf( "%u\n", g_timer_start(stw, 80, &echo, stw, (void *)0x80));
	printf( "%u\n", g_timer_start(stw, 70, &echo, stw, (void *)0x70));
	printf( "%u\n", g_timer_start(stw, 50, &echo, stw, (void *)0x50));
	printf( "%u\n", g_timer_start(stw, 60, &echo, stw, (void *)0x60));
	printf( "%u\n", g_timer_start(stw, 50, &echo, stw, (void *)0x50));
	g_thread_sleep(0.075);
	g_timer_loop(stw);
	printf( "%u\n", g_timer_start(stw, 50, &echo, stw, (void *)0x50));
	printf( "%u\n", g_timer_start(stw, 30, &echo, stw, (void *)0x30));
	g_thread_sleep(0.05);
	g_timer_loop(stw);
	g_timer_destroy(stw);
	return 0;
}
