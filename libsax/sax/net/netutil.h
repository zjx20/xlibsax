#ifndef __NETUTIL_H_2012__
#define __NETUTIL_H_2012__

#include <new>
#include "sax/os_types.h"
#include "sax/os_net.h"
#include "buffer.h"
#include "linked_buffer.h"

#define MAX_UDP_PACKAGE_SIZE 2048

namespace sax {

struct transport_handler;

class transport
{
public:
	struct id
	{
		int32_t    seq;
		int32_t    fd;
		transport* trans;

		id() : seq(0), fd(0), trans(NULL) {}
		id(uint64_t _seq, int32_t _fd, transport* _trans) :
			seq(_seq), fd(_fd), trans(_trans)
		{}

		id(const id& tid)
		{
			seq = tid.seq;
			fd = tid.fd;
			trans = tid.trans;
		}

		id& operator = (const id& tid)
		{
			seq = tid.seq;
			fd = tid.fd;
			trans = tid.trans;
			return *this;
		}

		bool operator == (const id& tid) const
		{
			return seq == tid.seq && trans == tid.trans;
		}

		bool operator < (const id& tid) const
		{
			if (trans != tid.trans) return trans < tid.trans;
			if (seq != tid.seq) return seq < tid.seq;
			return fd < tid.fd;
		}
	};

private:
	struct context
	{
		enum TYPE {TCP_LISTEN, UDP_BIND, TCP_CONNECTION};
		uint16_t type;
		uint16_t port_h;
		uint32_t ip_n;
		id       tid;
		linked_buffer* read_buf;
		linked_buffer* write_buf;
	};

public:
	transport();
	~transport();

	bool init(int32_t maxfds, transport_handler* handler);
	// for reusing _ctx and multithread handling.
	// be careful when use the same handler in multithread.
	bool clone(const transport& source, transport_handler* handler);

	bool listen(const char* addr, uint16_t port_h, int32_t backlog/* = 511*/,
			id& tid);
	bool listen_clone(const id& source);	// for multithread accept

	bool bind(const char* addr, uint16_t port_h, id& tid);
	bool bind_clone(const id& source);	// for multithread recv

	bool connect(const char* addr, uint16_t port_h, id& tid);

	bool send(const id& tid, const char* buf, int32_t length);

	// use the binded socket to send
	bool send_udp(const id& tid, uint32_t dest_ip_n,
			uint16_t dest_port_h, const char* buf, int32_t length);
	// use a random port to send
	bool send_udp(uint32_t dest_ip_n, uint16_t dest_port_h,
			const char* buf, int32_t length);

	void close(const id& tid);

	void poll(uint32_t millseconds);

	inline int32_t maxfds() {return _maxfds;}

	bool has_outdata(const id& tid);

private:

	bool add_fd(int fd, int eda_mask, const char* addr, uint16_t port_h,
			context::TYPE type);
	bool add_fd(int fd, int eda_mask, uint32_t ip_n, uint16_t port_h,
			context::TYPE type);

	void toggle_write(int fd, bool on = true);

	static void eda_callback(g_eda_t* mgr, int fd, void* user_data, int mask);

	static bool handle_tcp_accept(transport* trans, int fd);
	static bool handle_tcp_write(transport* trans, int fd, context& ctx);
	static bool handle_tcp_read(transport* trans, int fd, context& ctx);

	static bool handle_udp_read(transport* trans, int fd, context& ctx);

private:
	transport_handler* _handler;
	context*   _ctx;
	g_eda_t*   _eda;
	int32_t    _maxfds;
	int32_t    _seq;

	bool       _inited;
	bool       _cloned;
};

struct transport_handler
{
	transport_handler(transport* trans) : _trans(trans) {}
	virtual ~transport_handler() {}

	virtual void on_accepted(const transport::id& new_conn,
			const transport::id& from) = 0;
	virtual void on_tcp_send(const transport::id& tid, size_t send_bytes) = 0;
	virtual void on_tcp_received(const transport::id& tid, linked_buffer* buf) = 0;
	virtual void on_udp_received(const transport::id& tid, const char* data,
			size_t length, uint32_t ip_n, uint16_t port_h) = 0;
	virtual void on_closed(const transport::id& tid, int err) = 0;

protected:
	transport* _trans;
};

} // namespace

#endif//__NETUTIL_H_2012__
