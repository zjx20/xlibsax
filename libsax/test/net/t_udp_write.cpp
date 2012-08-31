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

#include <sys/socket.h>

char buf[1024 * 1024];

void eda_func(g_eda_t* mgr, int fd, void* user_data, int mask)
{
	printf("fd: %d mask: %d errno: %d %s\n", fd, mask, errno, strerror(errno));
	g_thread_sleep(1);
	if (mask & (EDA_WRITE)) {
//		g_thread_sleep(5.0);
		char buf[] = "hello udp!!!!!";
		//while (1) {
		//int ret = g_udp_write(fd, buf, sizeof(buf), "127.0.0.1", 4321);
		int ret = g_udp_write(-1, buf, sizeof(buf), "127.0.0.1", 4321);
		//int ret = g_udp_read(fd, buf, sizeof(buf), NULL, NULL);
		printf("first. ret: %d errno: %d %s\n", ret, errno, strerror(errno));
		g_eda_mod(mgr, fd, EDA_READ);
		if (ret < (int) sizeof(buf)) {
			g_thread_sleep(5);
		}
		//}
	}
	if (mask & EDA_READ) {
		char buf[6] = {0};
		uint32_t ip_n;
		uint16_t port_h;
		int ret1 = recvfrom(fd, NULL, 0, MSG_PEEK | MSG_TRUNC, NULL, NULL);
		printf("%d\n", ret1);
		int ret = g_udp_read(fd, buf, sizeof(buf)-1, &ip_n, &port_h);
		printf("ret: %d %u %u %s errno: %d %s\n", ret, ip_n, port_h, buf, errno, strerror(errno));
	}
	if (mask & EDA_ERROR) {
		//int ret = g_tcp_write(fd, "hello", 5);
		char buf[100];
		int ret = g_tcp_read(fd, buf, 10);
		printf("second. ret: %d errno: %d %s\n", ret, errno, strerror(errno));
	}
}

int main()
{
	signal(SIGPIPE, SIG_IGN);
	g_eda_t* eda = g_eda_open(100, eda_func, NULL);
	int bind_fd = g_udp_open(NULL, 4321);
	g_set_non_block(bind_fd);
	g_eda_add(eda, bind_fd, EDA_WRITE | EDA_READ);

	while (1) {
		g_eda_poll(eda, 100);
	}
	return 0;
}
