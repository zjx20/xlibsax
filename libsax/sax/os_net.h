#ifndef __OS_NET_H_2010_05__
#define __OS_NET_H_2010_05__

#include <stdlib.h>
#include "os_types.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

// functions for the socket accessing
int g_net_start();
void g_net_clean();

// convert hostname or ip string to network byte order uint32_t ip
int g_inet_aton(const char* addr, uint32_t* ip);
// convert network byte order uint32_t ip to ip string, thread safe
const char* g_inet_ntoa(uint32_t ip, char* ip_s, int32_t len);

uint32_t g_htonl(uint32_t hostlong);
uint16_t g_htons(uint16_t hostshort);
uint32_t g_ntohl(uint32_t netlong);
uint16_t g_ntohs(uint16_t netshort);

// addr == NULL for binding to 0.0.0.0
int g_tcp_listen(const char *addr, int port, int backlog);

int g_tcp_accept(int ts, uint32_t* ip_n, uint16_t* port_h);
int g_tcp_connect(const char *addr, int port, int non_block);
int g_tcp_read(int fd, void *buf, size_t count);
int g_tcp_write(int fd, const void *buf, size_t count);

int g_udp_open(const char *addr, int port);
// return the real packet size if it was truncated
int g_udp_read(int fd, void *buf, size_t count, uint32_t* ip_n, uint16_t* port_h);
int g_udp_write(int fd, const void *buf, int n, const char *ip, int port);
int g_udp_write2(int fd, const void *buf, int n, uint32_t ip_n, uint16_t port_h);

void g_close_socket(int fd);

int g_set_non_block(int fd);
int g_set_linger(int fd, int onoff, int linger);
int g_set_keepalive(int fd, int idle, int intvl, int count); // seconds

// An event-driven programming library for event-loop
#define EDA_NONE 0
#define EDA_READ 1
#define EDA_WRITE 2
#define EDA_ERROR 4

const int EDA_ALL_MASK = (EDA_READ | EDA_WRITE | EDA_ERROR);

typedef struct g_eda_t g_eda_t;

typedef void g_eda_func(
	g_eda_t* mgr, int fd, void* user_data, int mask);

g_eda_t* g_eda_open(int maxfds, g_eda_func* proc, void* user_data);
void g_eda_close(g_eda_t* mgr);
int g_eda_add(g_eda_t* mgr, int fd, int mask);
int g_eda_del(g_eda_t* mgr, int fd);
void g_eda_mod(g_eda_t* mgr, int fd, int mask);
int g_eda_poll(g_eda_t* mgr, int msec);	// msec == -1 for wait forever

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif //__OS_NET_H_2010_05__
