/*
 * t_tcp_write.cpp
 *
 *  Created on: 2012-8-28
 *      Author: x
 */

#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <string.h>

#include "sax/os_api.h"
#include "sax/os_net.h"

int total_read = 0;

char buf[1024 * 1024];

void eda_func(g_eda_t* mgr, int fd, void* user_data, int mask)
{
	printf("fd: %d mask: %d errno: %d %s\n", fd, mask, errno, strerror(errno));
	g_thread_sleep(1);
	if (mask == (EDA_WRITE | EDA_READ)) {
//		g_thread_sleep(5.0);
		//char buf[1023];
		//while (1) {
		int ret = g_tcp_write(fd, buf, sizeof(buf));
		total_read += ret;
		printf("first. total_send: %d ret: %d errno: %d %s\n", total_read, ret, errno, strerror(errno));
		if (ret < (int) sizeof(buf)) {
			g_thread_sleep(5);
		}
		//}
	}
	if (mask & EDA_ERROR) {
		//int ret = g_tcp_write(fd, "hello", 5);
		char buf[100];
		int ret = g_tcp_read(fd, buf, 10);
		printf("sedond. ret: %d errno: %d %s\n", ret, errno, strerror(errno));
	}
}

int main()
{
	signal(SIGPIPE, SIG_IGN);
	g_eda_t* eda = g_eda_open(100, eda_func, NULL);
	int listen_fd = g_tcp_listen(NULL, 4321, 100);
	int fd = g_tcp_accept(listen_fd, NULL, NULL);
	printf("set non block. ret = %d\n", g_set_non_block(fd));
	printf("accepted.\n");
	g_eda_add(eda, fd, EDA_WRITE | EDA_READ);

	while (1) {
		g_eda_poll(eda, 100);
	}
	return 0;
}
