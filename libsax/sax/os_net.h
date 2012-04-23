#ifndef __OS_NET_H_2010_05__
#define __OS_NET_H_2010_05__

#include <stdlib.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

// functions for the socket accessing
int g_net_start();
void g_net_clean();

int g_tcp_listen(const char *addr, int port);
int g_tcp_accept(int ts, char *ip, int *port);
int g_tcp_connect(const char *addr, int port, int non_block);
void g_tcp_close(int fd);
int g_tcp_read(int fd, void *buf, size_t count);
int g_tcp_write(int fd, const void *buf, size_t count);

int g_udp_open(const char *addr, int port);
void g_udp_close(int fd);
int g_udp_read(int fd, void *buf, size_t count, char *ip, int *port);
int g_udp_write(int fd, const void *buf, int n, const char *ip, int port);

int g_set_non_block(int fd);
int g_set_keepalive(int fd, int idle, int intvl, int probes); // seconds
int g_non_block_delayed(int got);

// An event-driven programming library for event-loop
#define EDA_NONE 0
#define EDA_READ 1
#define EDA_WRITE 2
#define EDA_ERROR 4

typedef struct g_eda_t g_eda_t;

typedef void g_eda_func(
	g_eda_t *mgr, int fd, void *clientData, int mask);

g_eda_t *g_eda_open(void);
void g_eda_close(g_eda_t *mgr);
int g_eda_add(g_eda_t *mgr, int fd, int mask, 
	g_eda_func *proc, void *clientData);
void g_eda_sub(g_eda_t *mgr, int fd, int mask);
void g_eda_set(g_eda_t *mgr, int fd, int mask);
int g_eda_loop(g_eda_t *mgr, int msec);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif //__OS_NET_H_2010_05__
