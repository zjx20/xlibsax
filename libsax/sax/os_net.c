#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "os_net.h"
#include "compiler.h"

//-------------------------------------------------------------------------
#if defined(WIN32) || defined(_WIN32)

#undef FD_SETSIZE
#define FD_SETSIZE 1024 

#include <winsock2.h>
#include <windows.h>

typedef int socklen_t;
#define CLOSE_SOCKET(s) closesocket(s)

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
int g_set_keepalive(int fd, int idle, int intvl, int probes)
{
	struct s_tcp_keepalive {
	    u_long  onoff;
	    u_long  keepalivetime;
	    u_long  keepaliveinterval;
	};
	
	BOOL on = (idle>0 && intvl>0 && probes>0);
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

int g_non_block_delayed(int got)
{
	return (got == -1 && WSAGetLastError() == WSAEWOULDBLOCK);
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

int g_set_keepalive(int fd, int idle, int intvl, int probes)
{
	int on = (idle>0 && intvl>0 && probes>0);
	
	int ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
	if (ret != 0 || !on) return ret;
	
	ret = (0==setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, &idle, sizeof(idle)) 
		&& 0==setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl))
		&& 0==setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &probes, sizeof(probes)));
	if (ret) return 0;
	
	on = 0; // diable keepAlive when error
	setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
	return -1;
}

int g_tcp_read(int fd, void *buf, size_t count)
{
	return read(fd, buf, count);
}

int g_non_block_delayed(int got)
{
	return (got == -1 && (errno == EAGAIN || errno == EINTR));
}
#endif

//-------------------------------------------------------------------------
int g_tcp_listen(const char *addr, int port, int backlog/* = 511*/)
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
		if (inet_aton(addr, &sa.sin_addr) == 0) goto quit;
	}
	
	if (bind(ts, (struct sockaddr *)&sa, sizeof(sa)) == -1) goto quit;
	
	if (listen(ts, backlog) != -1) return ts;
	
quit:
	CLOSE_SOCKET(ts); return -1;
}

int g_tcp_accept(int ts, char *ip, int *port)
{
	for (;;) {
		struct sockaddr_in sa;
		struct linger lin={0};
		socklen_t saLen = sizeof(sa);
		int on = 1, fd;
		fd = accept(ts, (struct sockaddr*)&sa, &saLen);
		if (fd == -1) {
#if !defined(WIN32) && !defined(_WIN32)
			if (errno == EINTR) continue;
#endif
			return -1;
		}
//		// set linger, avoid TIME_WAIT state
//		lin.l_onoff=1;
//		lin.l_linger=0;
//		setsockopt(fd, SOL_SOCKET, SO_LINGER,
//			(const char *) &lin, sizeof(lin));
		
		setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, 
			(const char *) &on, sizeof(on));
		
		if (ip) strcpy(ip, inet_ntoa(sa.sin_addr));
		if (port) *port = ntohs(sa.sin_port);
		return fd;
	}
}

int g_tcp_connect(const char *addr, int port, int non_block)
{
	int fd, on = 1;
	struct sockaddr_in sa;
	struct linger lin={0};

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		return -1;
	}

//	// set linger, avoid TIME_WAIT state
//	lin.l_onoff=1;
//	lin.l_linger=0;
//	if (setsockopt(fd, SOL_SOCKET, SO_LINGER,
//		(const char *) &lin, sizeof(lin)) == -1) goto quit;

	if (non_block && g_set_non_block(fd) != 0) goto quit;

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, 
		(const char *) &on, sizeof(on)) == -1) goto quit;
	
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
		(const char *) &on, sizeof(on)) == -1) goto quit;
	
	sa.sin_family = AF_INET;
	sa.sin_port = htons((u_short)port);
	
	if (inet_aton(addr, &sa.sin_addr) == 0)
	{
		struct hostent *he = gethostbyname(addr);
		if (!he) goto quit;
		memcpy(&sa.sin_addr, he->h_addr, sizeof(struct in_addr));
	}
	
	if (connect(fd, (struct sockaddr*)&sa, sizeof(sa)) == 0) return fd;

#if !defined(WIN32) && !defined(_WIN32)
	if (errno == EINPROGRESS && non_block) return fd;
#endif

quit:
	CLOSE_SOCKET(fd); return -1;
}

void g_tcp_close(int fd)
{
	CLOSE_SOCKET(fd);
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
		if (inet_aton(addr, &sa.sin_addr) == 0) goto quit;
	}
	
	if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) == -1) goto quit;
	return fd;
	
quit:
	CLOSE_SOCKET(fd); return -1;
}

void g_udp_close(int fd)
{
	CLOSE_SOCKET(fd);
}

int g_udp_read(int fd, void *buf, size_t count, char *ip, int *port)
{
	struct sockaddr_in sa;
	socklen_t len = 0;
	int got = recvfrom(fd, (char *) buf,
		count, 0, (struct sockaddr *)&sa, &len);
	if (got > 0) {
		if (ip) strcpy(ip, inet_ntoa(sa.sin_addr));
		if (port) *port = ntohs(sa.sin_port);
	}
	return got;
}

int g_udp_write(int fd, const void *buf, int n, const char *ip, int port)
{
	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons((u_short) port);
	if (inet_aton(ip, &sa.sin_addr) == 0) return -1;
	return sendto(fd, (const char *) buf,
		n, 0, (struct sockaddr *)&sa, sizeof(sa));
}


//-------------------------------------------------------------------------

/* the multiplexing layer supported by this system. */
#if defined(HAVE_EPOLL)

/*  Linux epoll(2) based */
#include <sys/epoll.h>

typedef struct epoll_event epoll_event;

#define MAX_EPOLL_EVENT 1024

typedef struct eda_item {
	g_eda_func* proc;
	void* user_data;
	int mask;
} eda_item;

struct g_eda_t {
	epoll_event		events[MAX_EPOLL_EVENT];	/* epoll_event array for polling */
	eda_item*		items;		/* context for FDs */
	int				epfd;		/* fd of epoll */
	int 			maxfds;		/* the max amount of FDs can polling at the same time */
};

g_eda_t* g_eda_open(int maxfds)
{
	assert(maxfds > 0);

	g_eda_t* h = malloc(sizeof(g_eda_t));
	if (!h) return NULL;

	h->epfd = epoll_create(1024);	// 1024 just a hint for kernel. for more details, man epoll_create()
	if (h->epfd == -1) {
		perror("in g_eda_open() calling epoll_create()");
		free(h);
		return NULL;
	}

	h->maxfds = maxfds;
	h->items = malloc(sizeof(eda_item) * maxfds);
	if (!(h->items)) {
		close(h->epfd);
		free(h);
		return NULL;
	}
	memset(h->items, 0, sizeof(eda_item) * maxfds);

	return h;
}

void g_eda_close(g_eda_t* mgr)
{
	assert(mgr);

	close(mgr->epfd);
	free(mgr->items);
	free(mgr);
}

int g_eda_add(g_eda_t* mgr, int fd, int mask,
	g_eda_func* proc, void* user_data)
{
	if (UNLIKELY( fd >= mgr->maxfds || mgr->items[fd].mask != EDA_NONE )) return -1;

	eda_item* item = mgr->items + fd;
	item->proc = proc;
	item->user_data = user_data;
	item->mask = mask;

	epoll_event ee;
	ee.events = 0;
	ee.events |= (mask & EDA_READ) ? EPOLLIN : 0;
	ee.events |= (mask & EDA_WRITE) ? EPOLLOUT : 0;
	ee.data.ptr = mgr->items + fd;

	if (LIKELY( epoll_ctl(mgr->epfd, EPOLL_CTL_ADD, fd, &ee) == 0 )) return 0;
	perror("in g_eda_add() calling epoll_ctl()");
	return -1;
}

int g_eda_del(g_eda_t* mgr, int fd)
{
	if (UNLIKELY( fd >= mgr->maxfds || mgr->items[fd].mask == EDA_NONE )) return -1;
	mgr->items[fd].mask = EDA_NONE;
	if (LIKELY( epoll_ctl(mgr->epfd, EPOLL_CTL_DEL, fd, NULL) == 0 )) return 0;
	perror("in g_eda_del() calling epoll_ctl()");
	return -1;
}

static void eda_mod_event(g_eda_t* mgr, int fd, int mask)
{
	epoll_event ee;
	ee.events = 0;
	ee.events |= (mask & EDA_READ) ? EPOLLIN : 0;
	ee.events |= (mask & EDA_WRITE) ? EPOLLOUT : 0;
	ee.data.ptr = mgr->items + fd;
	if (LIKELY( epoll_ctl(mgr->epfd, EPOLL_CTL_MOD, fd, &ee) == 0 )) {
		mgr->items[fd].mask = mask;
	}
	else {
		perror("in eda_mod_event() calling epoll_ctl()");
	}
}

void g_eda_sub(g_eda_t* mgr, int fd, int mask)
{
	if (UNLIKELY( fd >= mgr->maxfds || mgr->items[fd].mask == EDA_NONE )) return;

	int old_mask = mgr->items[fd].mask;
	int new_mask = ((old_mask & (~mask)) & EDA_ALL_MASK);
	if (LIKELY( new_mask != old_mask )) {
		eda_mod_event(mgr, fd, new_mask);
	}
}

void g_eda_set(g_eda_t* mgr, int fd, int mask)
{
	if (UNLIKELY( fd >= mgr->maxfds || mgr->items[fd].mask == EDA_NONE )) return;

	int new_mask = mask & EDA_ALL_MASK;
	if (LIKELY( mgr->items[fd].mask != new_mask )) {
		eda_mod_event(mgr, fd, new_mask);
	}
}

int g_eda_poll(g_eda_t* mgr, int msec)
{
	int nfds = epoll_wait(mgr->epfd, mgr->events, MAX_EPOLL_EVENT, msec);
	epoll_event* ee_ptr = mgr->events;
	int i;
	for (i = 0; i < nfds; i++, ee_ptr++) {
		eda_item* item = (eda_item*)ee_ptr->data.ptr;
		int fd = item - mgr->items;
		int mask = 0;
		mask |= (ee_ptr->events & EPOLLIN) ? EDA_READ : 0;
		mask |= (ee_ptr->events & EPOLLOUT) ? EDA_WRITE : 0;
		mask &= item->mask;

		// add EDA_ERROR automatic
		mask |= (ee_ptr->events & (EPOLLHUP | EPOLLERR)) ? EDA_ERROR : 0;

		item->proc(mgr, fd, item->user_data, mask);
	}

	return nfds;
}

#else

#error "select() based eda is not implement."

/* Select()-based */
#include <string.h>

typedef struct aeApiState 
{
    fd_set rfds, wfds;
    /* We need to have a copy of the fd sets as it's not 
     * safe to reuse FD sets after select(). */
    fd_set _rfds, _wfds;
} aeApiState;

static int aeApiCreate(g_eda_t *mgr) 
{
    aeApiState *state = malloc(sizeof(aeApiState));

    if (!state) return -1;
    FD_ZERO(&state->rfds);
    FD_ZERO(&state->wfds);
    mgr->apidata = state;
    return 0;
}

static void aeApiFree(g_eda_t *mgr) 
{
    free(mgr->apidata);
}

static int aeApiAddEvent(g_eda_t *mgr, int fd, int mask) 
{
    aeApiState *state = mgr->apidata;
    if (fd >= FD_SETSIZE) return -1;

    if (mask & EDA_READ) FD_SET(fd,&state->rfds);
    if (mask & EDA_WRITE) FD_SET(fd,&state->wfds);
    return 0;
}

static void aeApiDelEvent(g_eda_t *mgr, int fd, int mask) 
{
    aeApiState *state = mgr->apidata;
    if (fd >= FD_SETSIZE) return;

    if (mask & EDA_READ) FD_CLR(fd,&state->rfds);
    if (mask & EDA_WRITE) FD_CLR(fd,&state->wfds);
}

static int aeApiPoll(g_eda_t *mgr, struct timeval *tvp) 
{
    aeApiState *state = mgr->apidata;
    int retval, j, numevents = 0;

    memcpy(&state->_rfds,&state->rfds,sizeof(fd_set));
    memcpy(&state->_wfds,&state->wfds,sizeof(fd_set));

    retval = select(mgr->maxfd+1,
                &state->_rfds,&state->_wfds,NULL,tvp);
    if (retval > 0) {
        for (j = 0; j <= mgr->maxfd; j++) {
            int mask = 0;
            aeFileEvent *fe = &mgr->events[j];

            if (fe->mask == EDA_NONE) continue;
            if (fe->mask & EDA_READ && FD_ISSET(j,&state->_rfds))
                mask |= EDA_READ;
            if (fe->mask & EDA_WRITE && FD_ISSET(j,&state->_wfds))
                mask |= EDA_WRITE;
            mgr->fired[numevents].fd = j;
            mgr->fired[numevents].mask = mask;
            numevents++;
        }
    }
    return numevents;
}
#endif

//-------------------------------------------------------------------------
