/*
 * main.cpp
 *
 *  Created on: 2011-8-3
 *      Author: x_zhou
 */

#include "sax/os_net.h"

#include <iostream>
#include <string.h>
#include <stdlib.h>

void callback(g_eda_t *mgr, int fd, void *clientData, int mask)
{
    std::cout << "fd: " << fd << " clientData: " << clientData << " mask: " << mask << std::endl;
    static char line[1024] = {0};
    char cli_ip[256];
    int cli_port;
    if (clientData == (void *)1) {
        int connfd;
        while((connfd = g_tcp_accept(fd, cli_ip, &cli_port)) >= 0) {
            std::cout << "client connected. ip: " << cli_ip << " port: " << cli_port
                    << std::endl;
            g_set_non_block(connfd);
            g_set_keepalive(connfd, 5, 5, 3);
            g_eda_add(mgr, connfd, EDA_READ | EDA_ERROR, callback, (void*) 0);
        }
    }
    else {
    	if(mask & EDA_ERROR) {
			std::cout << "error happen.\n";
		}
        if (mask & EDA_READ) {
        	std::cout << "read event\n";
            while(1) {
                int n = g_tcp_read(fd, line, sizeof(line));
                std::cout << "n:" << n << std::endl;
                if (n < 0) {
                    g_eda_sub(mgr, fd, EDA_READ);
                    g_eda_add(mgr, fd, EDA_WRITE, callback, 0); // TODO: add g_eda_set()
                    break;
                }
                else if (n == 0) {
                    g_eda_sub(mgr, fd, EDA_READ | EDA_WRITE | EDA_ERROR);
                    g_tcp_close(fd);
                    std::cout << "client close." << std::endl;
                    break;
                }
                line[n] = '\0';
                std::cout << "client send: " << line << std::endl;
            }
        }
        if(mask & EDA_WRITE) {
            g_eda_sub(mgr, fd, EDA_WRITE);
            g_eda_add(mgr, fd, EDA_READ, callback, 0);
            g_tcp_write(fd, line, strlen(line));
            std::cout << "write done.\n";
        }
    }
}

int main() {
    g_eda_t *ep_mgr = g_eda_open();
    int listenfd = g_tcp_listen("0.0.0.0", 55666);
    std::cout << "listenfd: " << listenfd << std::endl;
    g_set_non_block(listenfd);

    g_eda_add(ep_mgr, listenfd, EDA_READ, callback, (void*) 1);

    for (;;)
    {
        int nfds = g_eda_loop(ep_mgr, 500);
        if(nfds>0) std::cout << "processed event count: " << nfds << std::endl;
    }

    return 0;
}
