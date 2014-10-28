#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "os_net.h"
#include "compiler.h"

//-------------------------------------------------------------------------
#if defined(WIN32) || defined(_WIN32)

#undef FD_SETSIZE
#define FD_SETSIZE 2048

#include <winsock2.h>
#include <windows.h>

typedef int socklen_t;
#define CLOSE_SOCKET(s) closesocket(s)

#define _SHUTDOWN_READ  SD_RECEIVE
#define _SHUTDOWN_WRITE SD_SEND
#define _SHUTDOWN_BOTH  SD_BOTH

int g_net_start()
{
	WSADATA wsa;
	int err = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (err != 0) return err;

	if (LOBYTE(wsa.wVersion) != 2 ||
		HIBYTE(wsa.wVersion) != 2)
	{
		WSACleanup(); return -1;
	}
	return 0;
}

void g_net_clean()
{
	WSACleanup();
}

int g_set_non_block(int fd)
{
    unsigned long ul = 1;
    if (ioctlsocket(fd, FIONBIO, &ul) == SOCKET_ERROR)
        return -1;
    else
        return 0;
}

/// @brief for windows, using SIO_KEEPALIVE_VALS and WSAIoctl
/// @note http://msdn.microsoft.com/en-us/library/dd877220(v=VS.85).aspx
/// part of code is copied from MSTcpIP.h for independence
int g_set_keepalive(int fd, int idle, int intvl, int count)
{
	struct s_tcp_keepalive {
	    u_long  onoff;
	    u_long  keepalivetime;
	    u_long  keepaliveinterval;
	};

	BOOL on = (idle>0 && intvl>0 && count>0);
	if (on) {
		struct s_tcp_keepalive val;
		DWORD got = 0;
		val.onoff = TRUE;
		val.keepalivetime = idle*1000L;
	 	val.keepaliveinterval = intvl*1000L;
#ifndef SIO_KEEPALIVE_VALS
#define SIO_KEEPALIVE_VALS  _WSAIOW(IOC_VENDOR, 4)
#endif
		return (WSAIoctl(fd, SIO_KEEPALIVE_VALS, &val, sizeof(val),
			NULL, 0, &got, NULL, NULL) == SOCKET_ERROR ? -1 : 0);
	}
	return setsockopt(fd, SOL_SOCKET,
		SO_KEEPALIVE, (const char *) &on, sizeof(on));
}

int g_tcp_read(int fd, void *buf, size_t count)
{
	return recv(fd, (char *) buf, count, 0);
}

static int inet_aton(const char *addr, struct in_addr *inn)
{
	unsigned long t = inet_addr(addr);
	if (t == INADDR_NONE) {return 0;}
	else {inn->s_addr = t; return 1;}
}

#else // NOT WIN32:

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>

#define CLOSE_SOCKET(s) close(s)

#define _SHUTDOWN_READ  SHUT_RD
#define _SHUTDOWN_WRITE SHUT_WR
#define _SHUTDOWN_BOTH  SHUT_RDWR

int g_net_start()
{
	return 0;
}

void g_net_clean()
{
}

int g_set_non_block(int fd)
{
	int flags = fcntl(fd,F_GETFL, 0);
	if (!(flags & O_NONBLOCK))
	{
		flags |= O_NONBLOCK;
		return fcntl(fd, F_SETFL, flags);
	}
	return 0;
}

int g_set_keepalive(int fd, int idle, int intvl, int count)
{
	int on = (idle>0 && intvl>0 && count>0);

	int ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
	if (ret != 0 || !on) return ret;

#ifdef __APPLE_CC__
	// there is no way to set retry interval and retry times on mac OSX
	ret = (0==setsockopt(fd, IPPROTO_TCP, TCP_KEEPALIVE, &idle, sizeof(idle)));
#else
	ret = (0==setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, &idle, sizeof(idle))
		&& 0==setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl))
		&& 0==setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &count, sizeof(count)));
#endif

	if (ret) return 0;

	on = 0; // disable keepAlive when error
	setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
	return -1;
}

int g_tcp_read(int fd, void *buf, size_t count)
{
	return read(fd, buf, count);
}

#endif

//-------------------------------------------------------------------------

int g_fd_setsize()
{
    return FD_SETSIZE;
}

int g_inet_aton(const char* addr, uint32_t* ip)
{
	if (inet_aton(addr, (struct in_addr*) ip) == 0)
	{
		struct hostent *he = gethostbyname(addr);
		if (!he) return -1;
		memcpy(ip, he->h_addr, sizeof(struct in_addr));
	}
	return 0;
}

const char* g_inet_ntoa(uint32_t ip, char* ip_s, int32_t len)
{
	return inet_ntop(AF_INET, &ip, ip_s, len);
}

uint32_t g_htonl(uint32_t hostlong)
{
	return htonl(hostlong);
}

uint16_t g_htons(uint16_t hostshort)
{
	return htons(hostshort);
}

uint32_t g_ntohl(uint32_t netlong)
{
	return ntohl(netlong);
}

uint16_t g_ntohs(uint16_t netshort)
{
	return ntohs(netshort);
}

int g_set_linger(int fd, int onoff, int linger)
{
	struct linger lin = {0};
	lin.l_onoff  = onoff;
	lin.l_linger = linger;
	return setsockopt(fd, SOL_SOCKET, SO_LINGER,
			(const char*) &lin, sizeof(lin));
}

int g_tcp_listen(const char *addr, int port, int backlog)
{
	int ts, on = 1;
 	struct sockaddr_in sa;

	if ((ts = socket(AF_INET, SOCK_STREAM, 0)) == -1) return -1;

	if (setsockopt(ts, SOL_SOCKET, SO_REUSEADDR,
		(const char *) &on, sizeof(on)) == -1) goto quit;

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons((u_short)port);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	if (addr) {
		if (g_inet_aton(addr, &sa.sin_addr.s_addr) != 0) goto quit;
	}

	if (bind(ts, (struct sockaddr *)&sa, sizeof(sa)) == -1) goto quit;

	if (listen(ts, backlog) != -1) return ts;

quit:
	CLOSE_SOCKET(ts); return -1;
}

int g_tcp_accept(int ts, uint32_t* ip_n, uint16_t* port_h)
{
	do {
		struct sockaddr_in sa;
		socklen_t saLen = sizeof(sa);
		int on = 1, fd;
		fd = accept(ts, (struct sockaddr*)&sa, &saLen);
		if (fd == -1) {
#if !defined(WIN32) && !defined(_WIN32)
			if (errno == EINTR) continue;
#endif
			return -1;
		}

		setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
			(const char *) &on, sizeof(on));

		if (ip_n) *ip_n = sa.sin_addr.s_addr;
		if (port_h) *port_h = ntohs(sa.sin_port);
		return fd;
	} while(1);

	return -1;	// never reach
}

int g_tcp_connect(const char *addr, int port, int non_block)
{
	int fd, on = 1;
	struct sockaddr_in sa;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		return -1;
	}

	if (non_block && g_set_non_block(fd) != 0) goto quit;

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
		(const char *) &on, sizeof(on)) == -1) goto quit;

	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
		(const char *) &on, sizeof(on)) == -1) goto quit;

	sa.sin_family = AF_INET;
	sa.sin_port = htons((u_short)port);

	if (g_inet_aton(addr, &sa.sin_addr.s_addr) != 0) goto quit;

	if (connect(fd, (struct sockaddr*)&sa, sizeof(sa)) == 0) return fd;

#if !defined(WIN32) && !defined(_WIN32)
	if (errno == EINPROGRESS && non_block) return fd;
#endif

quit:
	CLOSE_SOCKET(fd); return -1;
}

int g_tcp_connect_block(const char *addr, int port, int timeout_ms)
{
    int fd, on = 1;
    struct sockaddr_in sa;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        return -1;
    }

    if (g_set_non_block(fd) != 0) goto quit;

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
        (const char *) &on, sizeof(on)) == -1) goto quit;

    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
        (const char *) &on, sizeof(on)) == -1) goto quit;

    sa.sin_family = AF_INET;
    sa.sin_port = htons((u_short)port);

    if (g_inet_aton(addr, &sa.sin_addr.s_addr) != 0) goto quit;

    if (connect(fd, (struct sockaddr*)&sa, sizeof(sa)) != 0) {
#if !defined(WIN32) && !defined(_WIN32)
        if (errno != EINPROGRESS) goto quit;
#endif

        struct timeval tm;
        tm.tv_sec  = timeout_ms / 1000;
        tm.tv_usec = (timeout_ms % 1000) * 1000;

        fd_set rset, wset, eset;
        FD_ZERO(&rset);
        FD_SET(fd, &rset);

        FD_ZERO(&wset);
        FD_SET(fd, &wset);

        FD_ZERO(&eset);
        FD_SET(fd, &eset);

        if (select(fd + 1, &rset, &wset, &eset, &tm) > 0) {
            int len=sizeof(int);
            int err = -1;
            getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, (socklen_t*)&len);
            if(err != 0) goto quit;
            if (FD_ISSET(fd, &eset)) goto quit;
        }
        else {
            // timeout
            goto quit;
        }

    }

    return fd;

quit:
    CLOSE_SOCKET(fd); return -1;
}

int g_tcp_write(int fd, const void *buf, size_t count)
{
	return send(fd, (const char *)buf, count, 0);
}

//-------------------------------------------------------------------------
int g_udp_open(const char *addr, int port)
{
	int fd;
	struct sockaddr_in sa;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		return -1;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons((u_short)port);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	if (addr) {
		if (g_inet_aton(addr, &sa.sin_addr.s_addr) != 0) goto quit;
	}

	if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) == -1) goto quit;
	return fd;

quit:
	CLOSE_SOCKET(fd); return -1;
}

int g_udp_read(int fd, void *buf, size_t count, uint32_t* ip_n, uint16_t* port_h)
{
	struct sockaddr_in sa;
	socklen_t len = sizeof(struct sockaddr_in);
	int got = recvfrom(fd, (char *) buf,
		count, MSG_TRUNC, (struct sockaddr *)&sa, &len);
	if (got > 0) {
		if (ip_n) *ip_n = sa.sin_addr.s_addr;
		if (port_h) *port_h = ntohs(sa.sin_port);
	}
	return got;
}

int g_udp_write(int fd, const void *buf, int n, const char *ip, int port)
{
	uint32_t ip_n;
	if (g_inet_aton(ip, &ip_n) != 0) return -1;
	return g_udp_write2(fd, buf, n, ip_n, (uint16_t) port);
}

int g_udp_write2(int fd, const void *buf, int n, uint32_t ip_n, uint16_t port_h)
{
	int closefd = 0;
	int ret = 0;
	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port_h);
	sa.sin_addr.s_addr = ip_n;
	if (fd < 0) {
		fd = socket(AF_INET, SOCK_DGRAM, 0);
		if (fd == -1) return -1;
		closefd = 1;
	}
	ret = sendto(fd, (const char *) buf,
			n, 0, (struct sockaddr *)&sa, sizeof(sa));
	if (closefd) CLOSE_SOCKET(fd);
	return ret;
}

void g_shutdown_socket(int fd, int how)
{
    switch(how) {
    case SHUTDOWN_READ:  shutdown(fd, _SHUTDOWN_READ);  break;
    case SHUTDOWN_WRITE: shutdown(fd, _SHUTDOWN_WRITE); break;
    case SHUTDOWN_BOTH:  shutdown(fd, _SHUTDOWN_BOTH);  break;
    }
}

void g_close_socket(int fd)
{
	CLOSE_SOCKET(fd);
}

//-------------------------------------------------------------------------

/* the multiplexing layer supported by this system. */
#if defined(HAVE_EPOLL)

/*  Linux epoll(2) based */
#include <sys/epoll.h>

typedef struct epoll_event epoll_event;

#define MAX_EPOLL_EVENT 1024

struct g_eda_t {
	epoll_event events[MAX_EPOLL_EVENT];
	int         epfd;
	g_eda_func* proc;
	void*       user_data;
};

g_eda_t* g_eda_open(int maxfds, g_eda_func* proc, void* user_data)
{
	assert(maxfds > 0);

	g_eda_t* h = malloc(sizeof(g_eda_t));
	if (!h) return NULL;

	h->epfd = epoll_create(1024);	// 1024 just a hint for kernel. for more details, man epoll_create()
	if (h->epfd == -1) {
		fprintf(stderr, "error occurred when calling epoll_create() in g_eda_open(). errno: %d %s\n",
				errno, strerror(errno));
		free(h);
		return NULL;
	}

	h->proc = proc;
	h->user_data = user_data;

	return h;
}

void g_eda_close(g_eda_t* mgr)
{
	assert(mgr);

	close(mgr->epfd);
	free(mgr);
}

int g_eda_add(g_eda_t* mgr, int fd, int mask)
{
	epoll_event ee;
	ee.events = 0;
	ee.events |= (mask & EDA_READ) ? EPOLLIN : 0;
	ee.events |= (mask & EDA_WRITE) ? EPOLLOUT : 0;
	ee.data.u64 = fd;	// make valgrind silence

	if (LIKELY( epoll_ctl(mgr->epfd, EPOLL_CTL_ADD, fd, &ee) == 0 )) return 0;
	fprintf(stderr, "error occurred when calling epoll_ctl() in g_eda_add(). fd: %d errno: %d %s\n",
			fd, errno, strerror(errno));
	return -1;
}

int g_eda_del(g_eda_t* mgr, int fd)
{
	if (LIKELY( epoll_ctl(mgr->epfd, EPOLL_CTL_DEL, fd, NULL) == 0 )) return 0;
	fprintf(stderr, "error occurred when calling epoll_ctl() in g_eda_del(). fd: %d errno: %d %s\n",
			fd, errno, strerror(errno));
	return -1;
}

void g_eda_mod(g_eda_t* mgr, int fd, int mask)
{
	epoll_event ee;
	ee.events = 0;
	ee.events |= (mask & EDA_READ) ? EPOLLIN : 0;
	ee.events |= (mask & EDA_WRITE) ? EPOLLOUT : 0;
	ee.data.u64 = fd;	// make valgrind silence
	if (LIKELY( epoll_ctl(mgr->epfd, EPOLL_CTL_MOD, fd, &ee) == 0 )) return;
	fprintf(stderr, "error occurred when calling epoll_ctl() in g_eda_mod(). fd: %d errno: %d %s\n",
			fd, errno, strerror(errno));
}

int g_eda_poll(g_eda_t* mgr, int msec)
{
	int nfds = epoll_wait(mgr->epfd, mgr->events, MAX_EPOLL_EVENT, msec);
	epoll_event* ee_ptr = mgr->events;
	int i;
	for (i = 0; i < nfds; i++, ee_ptr++) {
		int fd = (int) ee_ptr->data.u64;
		int mask = 0;
		mask |= (ee_ptr->events & EPOLLIN) ? EDA_READ : 0;
		mask |= (ee_ptr->events & EPOLLOUT) ? EDA_WRITE : 0;

		// add EDA_ERROR automatically
		mask |= (ee_ptr->events & (EPOLLHUP | EPOLLERR)) ? EDA_ERROR : 0;

		mgr->proc(mgr, fd, mgr->user_data, mask);
	}

	return nfds;
}

#else

/* Select()-based */
#include <string.h>

struct g_eda_t {
	fd_set rfds, wfds, efds;

    /* We need to have a copy of the fd sets as it's not
     * safe to reuse FD sets after select(). */
	fd_set crfds, cwfds, cefds;

	int         maxfd;
	g_eda_func* proc;
	void*       user_data;
};

g_eda_t* g_eda_open(int maxfds, g_eda_func* proc, void* user_data)
{
	if (maxfds > FD_SETSIZE) {
		fprintf(stderr, "maxfds could not exceed FD_SETSIZE=%d.\n", FD_SETSIZE);
		return NULL;
	}

	assert(maxfds > 0);

	g_eda_t* h = malloc(sizeof(g_eda_t));
	if (!h) return NULL;

	FD_ZERO(&h->rfds);
	FD_ZERO(&h->wfds);
	FD_ZERO(&h->efds);

	h->maxfd = 0;
	h->proc = proc;
	h->user_data = user_data;

	return h;
}

void g_eda_close(g_eda_t* mgr)
{
	assert(mgr);
	free(mgr);
}

int g_eda_add(g_eda_t* mgr, int fd, int mask)
{
	if (UNLIKELY(fd > FD_SETSIZE)) return -1;

	assert(fd >= 0);

	if (mask & EDA_READ) FD_SET(fd, &mgr->rfds);
	if (mask & EDA_WRITE) FD_SET(fd, &mgr->wfds);
	FD_SET(fd, &mgr->efds);

	if (mgr->maxfd < fd) mgr->maxfd = fd;

	return 0;
}

int g_eda_del(g_eda_t* mgr, int fd)
{
	if (UNLIKELY(fd > FD_SETSIZE)) return -1;

	assert(fd >= 0);

	FD_CLR(fd, &mgr->rfds);
	FD_CLR(fd, &mgr->wfds);
	FD_CLR(fd, &mgr->efds);

	if (mgr->maxfd == fd && fd != 0) {
	    // be careful, fd can be zero...
		int j = fd - 1;
		do {
			if (FD_ISSET(j, &mgr->efds)) break;
			--j;
		} while (j > 0);

		mgr->maxfd = j;
	}

	return 0;
}

void g_eda_mod(g_eda_t* mgr, int fd, int mask)
{
	if (UNLIKELY(fd > FD_SETSIZE)) return;

	assert(fd >= 0 && FD_ISSET(fd, &mgr->efds));

	if ((mask & EDA_READ) ^ FD_ISSET(fd, &mgr->rfds)) {
		if (mask & EDA_READ) FD_SET(fd, &mgr->rfds);
		else FD_CLR(fd, &mgr->rfds);
	}

	if ((mask & EDA_WRITE) ^ FD_ISSET(fd, &mgr->wfds)) {
		if (mask & EDA_WRITE) FD_SET(fd, &mgr->wfds);
		else FD_CLR(fd, &mgr->wfds);
	}
}

int g_eda_poll(g_eda_t* mgr, int msec)
{
	struct timeval tv;
	tv.tv_sec = msec / 1000;
	tv.tv_usec = msec % 1000 * 1000;

	memcpy(&mgr->crfds,&mgr->rfds,sizeof(fd_set));
	memcpy(&mgr->cwfds,&mgr->wfds,sizeof(fd_set));
	memcpy(&mgr->cefds,&mgr->efds,sizeof(fd_set));

	int retval = select(mgr->maxfd + 1,
			&mgr->crfds, &mgr->cwfds, &mgr->cefds, &tv);
	int count = 0;
	if (retval > 0) {
		int j;
		for (j = 0; j <= mgr->maxfd; j++) {
			int mask = 0;

			if (FD_ISSET(j, &mgr->crfds))
				mask |= EDA_READ;
			if (FD_ISSET(j, &mgr->cwfds))
				mask |= EDA_WRITE;
			if (FD_ISSET(j, &mgr->cefds))
				mask |= EDA_ERROR;

			count++;

			if (mask != 0) {
				mgr->proc(mgr, j, mgr->user_data, mask);
			}
		}
	}

	return count;
}
#endif

//-------------------------------------------------------------------------
