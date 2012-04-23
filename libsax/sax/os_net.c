#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "os_net.h"

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
int g_tcp_listen(const char *addr, int port)
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
	
	// the magic constant 511 is from nginx
	if (listen(ts, 511) != -1) return ts;
	
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
		// set linger, avoid TIME_WAIT state
		lin.l_onoff=1;
		lin.l_linger=0;
		setsockopt(fd, SOL_SOCKET, SO_LINGER, 
			(const char *) &lin, sizeof(lin));
		
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

	// set linger, avoid TIME_WAIT state
	lin.l_onoff=1;
	lin.l_linger=0;
	if (setsockopt(fd, SOL_SOCKET, SO_LINGER, 
		(const char *) &lin, sizeof(lin)) == -1) goto quit;

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
	if (errno == 115 && non_block) return fd;
#endif // EINPROGRESS = 115

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
#include "os_api.h"

/* Max number of fd supported */
#define EDA_SETSIZE (1024*10)

#pragma pack(4)
/* File event structure */
typedef struct aeFileEvent {
    g_eda_func *proc;
    void *clientData;
    int mask; /* one of EDA_(READABLE|WRITABLE) */
} aeFileEvent;

/* A fired event */
typedef struct aeFiredEvent {
    int fd;
    int mask;
} aeFiredEvent;

/* State of an event based program */
struct g_eda_t {
    aeFileEvent events[EDA_SETSIZE]; /* Registered events */
    aeFiredEvent fired[EDA_SETSIZE]; /* Fired events */
    void *apidata; /* This is used for polling API specific data */
    g_mutex_t *mutex;
    int maxfd;
};
#pragma pack()

/* the multiplexing layer supported by this system. */
#if defined(HAVE_EPOLL)

/*  Linux epoll(2) based */
#include <sys/epoll.h>

typedef struct aeApiState 
{
    int epfd;
    struct epoll_event events[EDA_SETSIZE];
} aeApiState;

static int aeApiCreate(g_eda_t *mgr) 
{
    aeApiState *state = malloc(sizeof(aeApiState));

    if (!state) return -1;
    state->epfd = epoll_create(1024); /* 1024 is just an hint for the kernel */
    if (state->epfd == -1) return -1;
    mgr->apidata = state;
    return 0;
}

static void aeApiFree(g_eda_t *mgr) 
{
    aeApiState *state = mgr->apidata;

    close(state->epfd);
    free(state);
}

static int aeApiAddEvent(g_eda_t *mgr, int fd, int mask) 
{
    aeApiState *state = mgr->apidata;
    struct epoll_event ee;
    /* If the fd was already monitored for some event, we need a MOD
     * operation. Otherwise we need an ADD operation. */
    int op = mgr->events[fd].mask == EDA_NONE ?
            EPOLL_CTL_ADD : EPOLL_CTL_MOD;

    ee.events = 0;
    mask |= mgr->events[fd].mask; /* Merge old events */
    if (mask & EDA_READ) ee.events |= EPOLLIN;
    if (mask & EDA_WRITE) ee.events |= EPOLLOUT;
    ee.data.u64 = 0; /* avoid valgrind warning */
    ee.data.fd = fd;
    if (epoll_ctl(state->epfd,op,fd,&ee) == -1) return -1;
    return 0;
}

static void aeApiDelEvent(g_eda_t *mgr, int fd, int delmask) 
{
    aeApiState *state = mgr->apidata;
    struct epoll_event ee;
    int mask = mgr->events[fd].mask & (~delmask);

    ee.events = 0;
    if (mask & EDA_READ) ee.events |= EPOLLIN;
    if (mask & EDA_WRITE) ee.events |= EPOLLOUT;
    ee.data.u64 = 0; /* avoid valgrind warning */
    ee.data.fd = fd;
    if (mask != EDA_NONE) {
        epoll_ctl(state->epfd,EPOLL_CTL_MOD,fd,&ee);
    } else {
        /* Note, Kernel < 2.6.9 requires a non null event pointer even for
         * EPOLL_CTL_DEL. */
        epoll_ctl(state->epfd,EPOLL_CTL_DEL,fd,&ee);
    }
}

static int aeApiPoll(g_eda_t *mgr, struct timeval *tvp) 
{
    aeApiState *state = mgr->apidata;
    int retval, numevents = 0;

    retval = epoll_wait(state->epfd,state->events,EDA_SETSIZE,
            tvp ? (tvp->tv_sec*1000 + tvp->tv_usec/1000) : -1);
    if (retval > 0) {
        int j;

        numevents = retval;
        for (j = 0; j < numevents; j++) {
            int mask = 0;
            struct epoll_event *e = state->events+j;

            if (e->events & EPOLLIN) mask |= EDA_READ;
            if (e->events & EPOLLOUT) mask |= EDA_WRITE;
            if ((e->events & EPOLLERR) || (e->events & EPOLLHUP)) mask |= EDA_ERROR;
            mgr->fired[j].fd = e->data.fd;
            mgr->fired[j].mask = mask;
        }
    }
    return numevents;
}
#else 

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

g_eda_t *g_eda_open(void) 
{
    g_eda_t *mgr;
    int i;

    mgr = malloc(sizeof(*mgr));
    if (!mgr) return NULL;
    mgr->maxfd = -1;
    if (aeApiCreate(mgr) == -1) {
        free(mgr);
        return NULL;
    }
    mgr->mutex = g_mutex_init();
    /* Events with mask == EDA_NONE are not set. So let's initialize the
     * vector with it. */
    for (i = 0; i < EDA_SETSIZE; i++)
        mgr->events[i].mask = EDA_NONE;
    return mgr;
}

void g_eda_close(g_eda_t *mgr) 
{
	g_mutex_enter(mgr->mutex);
    aeApiFree(mgr);
    g_mutex_leave(mgr->mutex);
    g_mutex_free(mgr->mutex);
    free(mgr);
}

/* FD_SET is broken on windows (it adds the fd to a set 
 * twice or more, which eventually leads to overflows). 
 * Then we call g_eda_add/g_eda_sub only when changes.
 */
int g_eda_add(g_eda_t *mgr, int fd, int mask,
        g_eda_func *proc, void *clientData)
{
	int neu, ret=0;
	aeFileEvent *fe;
	
    if (fd >= EDA_SETSIZE) return -1;
    fe = &mgr->events[fd];
    
    g_mutex_enter(mgr->mutex);
    neu = mask & (EDA_READ | EDA_WRITE | EDA_ERROR);
    neu &= (~fe->mask);
    if (neu != EDA_NONE) {
    	ret = aeApiAddEvent(mgr,fd,neu);
        if (ret == -1) goto quit;
        fe->mask |= neu;
    }
    fe->proc = proc;
    fe->clientData = clientData;
    if (fd > mgr->maxfd) mgr->maxfd = fd;
    
    quit:
    g_mutex_leave(mgr->mutex);
    return ret;
}

void g_eda_sub(g_eda_t *mgr, int fd, int mask)
{
	aeFileEvent *fe;
    if (fd >= EDA_SETSIZE) return;
    fe = &mgr->events[fd];
	
    mask &= fe->mask;
    if (mask == EDA_NONE) return;
    
    g_mutex_enter(mgr->mutex);
    fe->mask &= (~mask);
    if (fd == mgr->maxfd && fe->mask == EDA_NONE) {
        /* Update the max fd */
        int j = mgr->maxfd-1;
        for (; j >= 0; j--)
            if (mgr->events[j].mask != EDA_NONE) break;
        mgr->maxfd = j;
    }
    aeApiDelEvent(mgr, fd, mask);
    g_mutex_leave(mgr->mutex);
}

void g_eda_set(g_eda_t *mgr, int fd, int mask)
{
    aeFileEvent *fe;
    int add = 0,del = 0;
    if (fd >= EDA_SETSIZE) return;
    fe = &mgr->events[fd];
    add = (~fe->mask & mask);
    del = (fe->mask & ~mask);

    g_mutex_enter(mgr->mutex);
    fe->mask = mask;
    if (fd == mgr->maxfd && fe->mask == EDA_NONE) {
        /* Update the max fd */
        int j = mgr->maxfd-1;
        for (; j >= 0; j--)
            if (mgr->events[j].mask != EDA_NONE) break;
        mgr->maxfd = j;
    }
    if(fe->mask != EDA_NONE && fd > mgr->maxfd) mgr->maxfd = fd;
    if(add != 0) aeApiAddEvent(mgr, fd, add);
    if(del != 0) aeApiDelEvent(mgr, fd, del);
    g_mutex_leave(mgr->mutex);
}

/* Process every pending file event */
int g_eda_loop(g_eda_t *mgr, int msec)
{
    int processed = 0, numevents, j;
    struct timeval tv, *tvp;
    
    if (msec >= 0) {
        tv.tv_sec = (msec/1000);
        tv.tv_usec = (msec%1000)*1000;
        tvp = &tv;
    } else {
        /* Otherwise we can block */
        tvp = NULL; /* wait forever */
    }
    
    /* Note that we want call select() even if there are no
     * file events to process in order to sleep */
    if (mgr->maxfd == -1) {
    	if (tvp) select(0, NULL, NULL, NULL, tvp);
    	return 0;
    }

	g_mutex_enter(mgr->mutex);
    numevents = aeApiPoll(mgr, tvp);
    g_mutex_leave(mgr->mutex);
    
    for (j = 0; j < numevents; j++) {
        aeFileEvent *fe = &mgr->events[mgr->fired[j].fd];
        int mask = mgr->fired[j].mask;
        int fd = mgr->fired[j].fd;
	/* note the fe->mask & mask & ... code: maybe an already processed
         * event removed an element that fired and we still didn't
         * processed, so we check if the event is still valid. */
        mask &= fe->mask;
        if (mask & (EDA_READ|EDA_WRITE|EDA_ERROR)) {
            fe->proc(mgr,fd,fe->clientData,mask);
        }
        processed++;
    }
    
    return processed; /* return the number of processed */
}
//-------------------------------------------------------------------------

