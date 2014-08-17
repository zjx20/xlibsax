/*
 * t_http.cpp
 *
 *  Created on: 2013-9-25
 *      Author: x
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <algorithm>
#include "sax/net/netutil.h"
#include "sax/logger/logger.h"
#include "sax/os_api.h"

#include "http_parser.h"

volatile bool finish = false;

#define KEEP_ALIVE 0

#if KEEP_ALIVE
char response[] = "HTTP/1.1 200 OK\nServer: libsax/0.01\nDate: Thu, 30 Aug 2012 08:20:48 GMT\nContent-Type: text/html\nContent-Length: 151\nLast-Modified: Thu, 30 Aug 2012 03:04:37 GMT\nConnection: keep-alive\nAccept-Ranges: bytes\n\n<html>\n<head>\n<title>Welcome to nginx!</title>\n</head>\n<body bgcolor=\"white\" text=\"black\">\n<center><h1>Welcome to nginx!</h1></center>\n</body>\n</html>\n";
#else
char response[] = "HTTP/1.1 200 OK\nServer: libsax/0.01\nDate: Thu, 30 Aug 2012 03:15:02 GMT\nContent-Type: text/html\nContent-Length: 151\nLast-Modified: Thu, 30 Aug 2012 03:04:37 GMT\nConnection: close\nAccept-Ranges: bytes\n\n<html>\n<head>\n<title>Welcome to nginx!</title>\n</head>\n<body bgcolor=\"white\" text=\"black\">\n<center><h1>Welcome to nginx!</h1></center>\n</body>\n</html>\n";
#endif

// TODO: reject non content-length request
// TODO: limit request max size

struct http_context
{
	http_parser        parser;
	bool               close_conn;
	sax::transport::id tid;
	void*              handler;
	int32_t            body_length;
	uint8_t            body[8*1024];	// TODO: hard coded
};

static int _on_body(http_parser* parser, const char *at, size_t length);

struct simple_http_handler : public sax::transport_handler
{
	http_context*        contexts;
	http_parser_settings settings;

	simple_http_handler(sax::transport* trans, int32_t maxfds) : sax::transport_handler(trans)
	{
		contexts = new http_context[maxfds + 1];

		memset(&settings, 0, sizeof(settings));
		settings.on_body = _on_body;
	}

	~simple_http_handler()
	{
		delete[] contexts;
		contexts = NULL;
	}

	virtual void on_tcp_send(const sax::transport::id& tid, size_t send_bytes)
	{
		// do nothing
	}

	virtual void on_accepted(const sax::transport::id& new_conn,
			const sax::transport::id& from, uint32_t ip_n, uint16_t port_h)
	{
		char ip_buf[20] = {0};
		g_inet_ntoa(ip_n, ip_buf, sizeof(ip_buf));
		LOG_TRACE("new conn. from: " << ip_buf << ":" << port_h <<
				" trans: " << _trans << " thread_id: " << g_thread_id() <<
				" fd: " << new_conn.fd << " seq: " << new_conn.seq);

		http_context* context = contexts + new_conn.fd;
		context->close_conn = false;
		context->body_length = 0;
		context->tid = new_conn;
		context->handler = this;
		http_parser_init(&context->parser, HTTP_REQUEST);
		context->parser.data = context;
	}

	virtual void on_tcp_recieved(const sax::transport::id& tid, sax::linked_buffer* buf)
	{
		http_context* context = contexts + tid.fd;
		uint32_t limit;
		size_t nparsed;

		assert(context->tid == tid);

		while (buf->remaining() && !context->close_conn) {
			char *ptr = buf->direct_get(limit);
			nparsed = http_parser_execute(&context->parser, &settings, ptr, limit);
			buf->commit_get(ptr, limit);

			if (nparsed != limit)
			{
				// error happened
				http_errno http_err = HTTP_PARSER_ERRNO(&context->parser);
				LOG_DEBUG("[http_parser] " << http_errno_name(http_err) << ": \"" <<
						http_errno_description(http_err) << "\", the connection will be closed." <<
						" trans: " << _trans << " thread_id: " << g_thread_id() <<
						" fd: " << tid.fd << " seq: " << tid.seq);

				_trans->close(tid);
				return;
			}
		}

		buf->compact();

		if (context->close_conn)
		{
			_trans->close(tid);
		}
	}

	void handle_request(http_context* context)
	{
		LOG_TRACE("--------------------- http body parsed");
		context->body[context->body_length] = '\0';
		LOG_TRACE("http body: \n" << (char*)context->body);

		_trans->send(context->tid, response, strlen(response));

		context->body_length = 0;

		if (!http_should_keep_alive(&context->parser))
		{
			// should not close conn directly at here
			//_trans->close(context->tid);	wrong!!

			context->close_conn = true;
		}
	}

	virtual void on_udp_recieved(const sax::transport::id& tid,
			const char* data, size_t length, uint32_t ip_n, uint16_t port_h) {throw "not implemented";}

	virtual void on_closed(const sax::transport::id& tid, int err)
	{
		_trans->close(tid);
		LOG_TRACE("close conn. thread_id: " << g_thread_id() << " fd: " << tid.fd << " errno: " << err);
		//printf("conn closed. fd: %d errno: %d %s\n", tid.fd, err, strerror(err));
		//finish = true;
	}
};

int _on_body(http_parser* parser, const char *at, size_t length)
{
	http_context* context = (http_context*) parser->data;

	assert(context->body_length + length < sizeof(context->body));

	memcpy(context->body + context->body_length, at, length);
	context->body_length += length;

	if (http_body_is_final(&context->parser))
	{
		simple_http_handler* handler = (simple_http_handler*)context->handler;
		handler->handle_request(context);
	}

	return 0;
}

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

	char logname[100];
	snprintf(logname, sizeof(logname), "%s.log", argv[0]);
	INIT_GLOBAL_LOGGER(logname, 10, 100*1024*1024, sax::logger::SAX_INFO);

	int thread_num = 1;

	if (argc > 1) {
		thread_num = atoi(argv[1]);
	}

	g_thread_t* ts = new g_thread_t[thread_num];

	int32_t maxfds = 10230;

	sax::transport trans;
	simple_http_handler* handler = new simple_http_handler(&trans, maxfds);

	printf("shm_unit: %u\n", g_shm_unit());

	if (trans.init(maxfds, handler)) {
		printf("transport inited.\n");
		sax::transport::id tid;
		if (trans.listen(NULL, 6543, 511, tid)) {
			printf("start listen.\n");
			if (thread_num > 1) {
				for (int i=1;i<thread_num;i++) {
					sax::transport* trans_clone = new sax::transport();
					if (!trans_clone->clone(trans, new simple_http_handler(trans_clone, maxfds))) assert(0);
					if (!trans_clone->listen_clone(tid)) assert(0);
					ts[i] = g_thread_start(thread_func, trans_clone);
				}
			}

			while (!finish) {
				trans.poll(100);
			}
		}
	}

	for (int i=1;i<thread_num;i++) {
		g_thread_join(ts[i], NULL);
	}

	delete[] ts;

	DESTROY_GLOBAL_LOGGER();

	return 0;
}
