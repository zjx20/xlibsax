#ifndef __NETUTIL_H_2011__
#define __NETUTIL_H_2011__

#include <new>
#include "os_types.h"
#include "os_net.h"

namespace sax {

class transport;

struct transport_id
{
	uint32_t   seq;
	int32_t    fd;
	transport* trans;

	transport_id() : seq(0), fd(0), trans(NULL) {}
	transport_id(uint64_t _seq, int32_t _fd, transport* _trans) :
		seq(_seq), fd(_fd), trans(_trans)
	{}

	transport_id(const transport_id& tid)
	{
		seq = tid.seq;
		fd = tid.fd;
		trans = tid.trans;
	}

	transport_id& operator = (const transport_id& tid)
	{
		seq = tid.seq;
		fd = tid.fd;
		trans = tid.trans;
		return *this;
	}

	bool operator == (const transport_id& tid) const
	{
		return seq == tid.seq && trans == tid.trans;
	}

	bool operator < (const transport_id& tid) const
	{
		if (trans != tid.trans) return trans < tid.trans;
		if (seq != tid.seq) return seq < tid.seq;
		return fd < tid.fd;
	}
};

struct transport_context
{
	enum TYPE {TCP_LISTEN, UDP_BIND, TCP_CONNECTION};
	uint16_t     type;
	uint16_t     port_h;
	uint32_t     ip_n;
	transport_id tid;
};

struct transport_handler
{
	virtual void on_accepted(const transport_id& new_conn,
			const transport_id& from) {}
	virtual void on_tcp_recieved(const transport_id& tid) {}
	virtual void on_udp_recieved(const transport_id& tid) {}
	virtual void on_closed(const transport_id& tid) {}
	virtual ~transport_handler() {}
};

class transport
{
public:
	transport();
	~transport();

	bool init(int32_t maxfds, transport_handler* handler);
	bool clone(const transport& source);	// for reusing _ctx and multithread handling

	bool listen(const char* addr, uint16_t port_h, transport_id& tid
			, int32_t backlog = 511);
	bool listen_clone(const transport_id& source);	// for multithread accept

	bool bind(const char* addr, uint16_t port_h, transport_id& tid);
	bool bind_clone(const transport_id& source);	// for multithread recv

	bool connect(const char* addr, uint16_t port_h, transport_id& tid);

	bool send(const transport_id& tid, char* buf, int32_t length);

	bool send_udp(const transport_id& tid, const char* addr, uint16_t port_h,
			char* buf, int32_t length);	// use the binded socket to send
	bool send_udp(const char* addr, uint16_t port_h,
			char* buf, int32_t length);	// use a random port to send

	void close(const transport_id& tid);

	void poll(uint32_t millseconds);

private:
	bool add_fd(int fd, int eda_mask, const char* addr, uint16_t port_h);

	static void _eda_callback(g_eda_t* mgr, int fd, void* user_data, int mask);

private:
	transport_handler* _handler;
	transport_context* _ctx;
	g_eda_t* _eda;
	int32_t  _maxfds;
	uint32_t _seq;

	bool     _inited;
	bool     _cloned;
};

//bool global_init();
//void global_free();
//
//struct protocol {
//	enum type {TCP=0, UDP=1, FILE=2};
//};
//
///// @brief a unified structure for containing event body data
//struct event_body
//{
//	enum type {ONEWAY=1, REQUEST=2, ANSWER=3, INFORM=4};
//
//	uint32_t seqid;
//	char fn[36];
//	event_body::type dir;
//	void *argument;
//};
//
///// @brief streamer define interfaces for [event] serialization
//struct streamer {
//	virtual int decode(sax::binary &b, sax::event_body &e) = 0;
//	virtual int encode(sax::event_body &e, sax::binary &b) = 0;
//	virtual ~streamer() {}
//};
//
///// @note spec should be like: tcp://127.0.0.1:15000
//class transport : public nocopy
//{
//public:
//	transport(sax::stage *biz);
//	~transport();
//	uint64_t listen(const char *spec, streamer *str);
//	uint64_t connect(const char *spec, streamer *str);
//	bool close(uint64_t cid);
//	bool dispatch(uint64_t cid, const event_body &e);
//
//	void halt();
//	bool start(int queue_cap, int mem_size);
//
//private:
//	struct sock_mgr *_mgr;
//};
//
//struct seda_event : public sax_event_base<0>
//{
//	protocol::type proto;
//	int  port;
//	char addr[24];
//
//	sax::event_body body;
//
//	uint64_t cid;
//	sax::transport *sink;
//};

} // namespace

#endif//__NETUTIL_H_2011__
