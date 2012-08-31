/*
 * t_netutil.cpp
 *
 *  Created on: 2012-8-29
 *      Author: x
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <algorithm>
#include "sax/net/netutil.h"
#include "sax/logger/logger.h"
#include "sax/os_api.h"

volatile bool finish = false;

#define KEEP_ALIVE 1

#if KEEP_ALIVE
char response[] = "HTTP/1.1 200 OK\nServer: libsax/0.01\nDate: Thu, 30 Aug 2012 08:20:48 GMT\nContent-Type: text/html\nContent-Length: 151\nLast-Modified: Thu, 30 Aug 2012 03:04:37 GMT\nConnection: keep-alive\nAccept-Ranges: bytes\n\n<html>\n<head>\n<title>Welcome to nginx!</title>\n</head>\n<body bgcolor=\"white\" text=\"black\">\n<center><h1>Welcome to nginx!</h1></center>\n</body>\n</html>\n";
#else
char response[] = "HTTP/1.1 200 OK\nServer: libsax/0.01\nDate: Thu, 30 Aug 2012 03:15:02 GMT\nContent-Type: text/html\nContent-Length: 151\nLast-Modified: Thu, 30 Aug 2012 03:04:37 GMT\nConnection: close\nAccept-Ranges: bytes\n\n<html>\n<head>\n<title>Welcome to nginx!</title>\n</head>\n<body bgcolor=\"white\" text=\"black\">\n<center><h1>Welcome to nginx!</h1></center>\n</body>\n</html>\n";
#endif

struct my_handler : public sax::transport_handler
{
	my_handler(sax::transport* trans) : _trans(trans) {}

	virtual void on_accepted(const sax::transport::id& new_conn,
			const sax::transport::id& from)
	{
		//static ip_buf[20];
		//printf("new conn: %s:%d fd: %d\n", g_inet_ntoa(new_conn));
		//printf("new conn. fd: %d thread_id: %ld\n", new_conn.fd, g_thread_id());
		LOG_TRACE("new conn. thread_id: " << g_thread_id() << " fd: " << new_conn.fd);
	}

	virtual void on_tcp_recieved(const sax::transport::id& tid, sax::buffer* buf)
	{
		char temp[1024];
		while (buf->remaining()) {
			size_t len = std::min(sizeof(temp), buf->remaining());
			buf->get((uint8_t*) temp, len);
			_trans->send(tid, response, strlen(response));
		}
		buf->compact();

		LOG_TRACE("close conn in biz. thread_id: " << g_thread_id() << " fd: " << tid.fd);
#if !KEEP_ALIVE
		_trans->close(tid);
#endif
	}

	virtual void on_udp_recieved(const sax::transport::id& tid,
			const char* data, size_t length, uint32_t ip_n, uint16_t port_h) {}

	virtual void on_closed(const sax::transport::id& tid, int err)
	{
		LOG_TRACE("close conn. thread_id: " << g_thread_id() << " fd: " << tid.fd << " errno: " << err);
		//printf("conn closed. fd: %d errno: %d %s\n", tid.fd, err, strerror(err));
		//finish = true;
	}

private:
	sax::transport* _trans;
};

void* thread_func(void* param)
{
	sax::transport* trans = (sax::transport*) param;
	while (!finish) {
		trans->poll(100);
	}
	delete trans;
	return 0;
}

void sig_handler(int signum)
{
	finish = true;
	printf("shutdown\n");
}

int main(int argc, char* argv[])
{
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	INIT_GLOBAL_LOGGER("netutil_test.txt", 10, 100*1024*1024, sax::logger::SAX_OFF);

	int thread_num = 1;

	if (argc > 1) {
		thread_num = atoi(argv[1]);
	}

	g_thread_t* ts = new g_thread_t[thread_num];

	sax::transport trans;
	my_handler* handler = new my_handler(&trans);

	if (trans.init(10000, handler)) {
		printf("transport inited.\n");
		sax::transport::id tid;
		if (trans.listen(NULL, 6543, 100, tid)) {
			if (thread_num > 1) {
				for (int i=1;i<thread_num;i++) {
					sax::transport* trans_clone = new sax::transport();
					if (!trans_clone->clone(trans, new my_handler(trans_clone))) assert(0);
					if (!trans_clone->listen_clone(tid)) assert(0);
					ts[i] = g_thread_start(thread_func, trans_clone);
				}
			}
			printf("start listen.\n");
			while (!finish) {
				trans.poll(100);
			}

			trans.close(tid);
		}
	}

	if (thread_num > 1) {
		for (int i=1;i<thread_num;i++) {
			g_thread_join(ts[i], NULL);
		}
	}

	delete[] ts;

	return 0;
}
