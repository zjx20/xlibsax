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
	uint64_t callback_send;
	uint64_t total_send;
	uint64_t async_send;

	my_handler(sax::transport* trans) : sax::transport_handler(trans)
	{
		callback_send = 0;
		total_send = 0;
		async_send = 0;
	}

	virtual void on_tcp_send(const sax::transport::id& tid, size_t send_bytes)
	{
		callback_send += send_bytes;
	}

	virtual void on_accepted(const sax::transport::id& new_conn,
			const sax::transport::id& from, uint32_t ip_n, uint16_t port_h)
	{
		//static ip_buf[20];
		//printf("new conn: %s:%d fd: %d\n", g_inet_ntoa(new_conn));
		//printf("new conn. fd: %d thread_id: %ld\n", new_conn.fd, g_thread_id());
		LOG_TRACE("new conn. thread_id: " << g_thread_id() << " fd: " << new_conn.fd);
	}

	virtual void on_tcp_received(const sax::transport::id& tid, sax::linked_buffer* buf)
	{
		char temp[1024];
		while (buf->remaining()) {
			size_t len = std::min(sizeof(temp), (size_t) buf->remaining());
			buf->get((uint8_t*) temp, len);
			total_send += strlen(response);
			_trans->send(tid, response, strlen(response));
			async_send += total_send - callback_send;
		}
		buf->compact();

#if !KEEP_ALIVE
		LOG_TRACE("close conn in biz. thread_id: " << g_thread_id() << " fd: " << tid.fd);
		_trans->close(tid);
#endif
	}

	virtual void on_udp_received(const sax::transport::id& tid,
			const char* data, size_t length, uint32_t ip_n, uint16_t port_h) {}

	virtual void on_closed(const sax::transport::id& tid, int err)
	{
		_trans->close(tid);
		LOG_TRACE("close conn. thread_id: " << g_thread_id() << " fd: " << tid.fd << " errno: " << err);
		//printf("conn closed. fd: %d errno: %d %s\n", tid.fd, err, strerror(err));
		//finish = true;
	}
};

void print_send_stat(sax::transport* trans)
{
	// WARNING: hack code
	my_handler* handler = static_cast<my_handler*>((sax::transport_handler*) ((void**) trans)[0]);
	printf("total send: %lu async send: %lu callback send: %lu\n",
			handler->total_send, handler->async_send, handler->callback_send);
}

void* thread_func(void* param)
{
	sax::transport* trans = (sax::transport*) param;
	while (!finish) {
		trans->poll(100);
	}
	print_send_stat(trans);
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

	INIT_GLOBAL_LOGGER("netutil_test.txt", 10, 100*1024*1024, sax::logger::SAX_TRACE);

	int thread_num = 1;

	if (argc > 1) {
		thread_num = atoi(argv[1]);
	}

	g_thread_t* ts = new g_thread_t[thread_num];

	sax::transport trans;
	my_handler* handler = new my_handler(&trans);

	if (trans.init(1023, handler)) {
		printf("transport inited.\n");
		sax::transport::id tid;
		if (trans.listen(NULL, 6543, 511, tid)) {
			printf("start listen.\n");
			if (thread_num > 1) {
				for (int i=1;i<thread_num;i++) {
					sax::transport* trans_clone = new sax::transport();
					if (!trans_clone->clone(trans, new my_handler(trans_clone))) assert(0);
					if (!trans_clone->listen_clone(tid)) assert(0);
					ts[i] = g_thread_start(thread_func, trans_clone);
				}
			}

			while (!finish) {
				trans.poll(100);
			}

			print_send_stat(&trans);
		}
	}

	for (int i=1;i<thread_num;i++) {
		g_thread_join(ts[i], NULL);
	}

	delete[] ts;

	DESTROY_GLOBAL_LOGGER();

	return 0;
}
