/*
 * t_tcp_lazy_client.cpp
 *
 *  Created on: 2012-8-28
 *      Author: x
 */

#include <stdio.h>
#include <stdlib.h>
#include "sax/os_net.h"
#include "sax/os_api.h"

char buf[1024*1024];

int main(int argc, char* argv[])
{
	if (argc < 3) {
		printf("usage: %s ip port\n", argv[0]);
		exit(1);
	}
	int fd = g_tcp_connect(argv[1], atoi(argv[2]), 0);
	printf("connected. fd: %d\n", fd);
	while (1) {
		g_tcp_write(fd, buf, sizeof(buf));
		g_thread_sleep(1.0);
	}
	return 0;
}
