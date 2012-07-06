#include "os_api.h"
#include "os_net.h"
#include "c++/linklist.h"
#include "netutil.h"

#include <string.h>
#include <stdio.h>

namespace sax {

//-------------------------------------------------------------------------
bool global_init()
{
	g_slab_setup(g_shm_unit());
	(void) g_now_uid();
	(void) g_now_hr();
	return (g_net_start() != 0);
}

void global_free()
{
	g_net_clean();
}

//-------------------------------------------------------------------------
struct connection {
	protocol::type proto;
	int  port;
	char addr[24];
	int  fd;
	struct streamer *str;
	struct sock_mgr *mgr;
	binary rbuf;
	binary wbuf;
	uint64_t stamp;
	connection *_prev;
	connection *_next;
};

static inline void conn_init(
	connection *conn, protocol::type proto, const char *addr, 
	int port, int fd, struct streamer *str, struct sock_mgr *mgr)
{
	conn->proto = proto;
	conn->port = port;
	g_strlcpy(conn->addr, addr, sizeof(conn->addr));
	conn->fd = fd;
	conn->str = str;
	conn->mgr = mgr;
	conn->stamp = g_now_hr();
}

struct sock_mgr {
public:
	friend class nio_handler;
	friend class transport;
	sock_mgr(transport *owner, sax::stage *biz) : 
		_owner(owner), _nio(NULL), _biz(biz)
	{ 
		_eda = g_eda_open(64*1024);
		_alloc.init2(NULL, 64*1024); //max_conn=64K
	}
	~sock_mgr() { 
		g_eda_close(_eda);
		if (_nio) delete _nio;
	}
	uint64_t tcp_accept(streamer *str, int fd, 
		protocol::type proto, const char *addr, int port)
	{
		connection *conn = _alloc.alloc_obj();
		if (!conn) return 0;
		conn_init(conn, proto, addr, port, fd, str, this);
		
		if (0 != g_eda_add(_eda, fd, EDA_READ, &on_tcp_accept, conn))
		{
			_alloc.free_obj(conn);
			return 0;
		}
		_alive.push_back(conn);
		return send_inform(_biz, conn, "ListenEvent");
	}
	uint64_t tcp_stream(streamer *str, int fd, 
		protocol::type proto, const char *addr, int port)
	{
		connection *conn = _alloc.alloc_obj();
		if (!conn) return 0;
		conn_init(conn, proto, addr, port, fd, str, this);
		
		if (0 != g_eda_add(_eda, fd, 
			EDA_READ | EDA_WRITE | EDA_ERROR, &on_tcp_stream, conn))
		{
			_alloc.free_obj(conn);
			return 0;
		}
		_alive.push_back(conn);
		return send_inform(_biz, conn, "ConnectEvent");
	}
	bool tcp_remove(connection *conn)
	{
		send_inform(_biz, conn, "ClosedEvent");
		int fd = conn->fd;
		g_eda_sub(_eda, fd, EDA_READ | EDA_WRITE | EDA_ERROR);
		g_tcp_close(fd);
		_alive.erase(conn);
		return _alloc.free_obj(conn);
	}
	
private:
	transport *_owner;
	sax::stage *_nio;
	struct g_eda_t *_eda;
	sax::stage *_biz;
	linklist<connection> _alive;
	spool<connection> _alloc;

protected:
	static void on_tcp_accept(
		struct g_eda_t *_eda, int fd, void *client, int mask);
	static void on_tcp_stream(
		struct g_eda_t *_eda, int fd, void *client, int mask);
	
	static uint64_t send_inform(
		sax::stage *biz, connection *conn, const char *fn);
	static uint64_t send_message(
		sax::stage *biz, connection *conn, const event_body &e);
};

void sock_mgr::on_tcp_accept(
	struct g_eda_t *_eda, int fd, void *client, int mask)
{
	connection *conn = (connection *) client;
	for (;;) {
		int port = 0;
		char ip[24];
		
		int new_fd = g_tcp_accept(fd, ip, &port);
		if (new_fd < 0) break;
		
		if (g_set_non_block(new_fd) == 0) {
			uint64_t cid = conn->mgr->tcp_stream(
				conn->str, new_fd, protocol::TCP, ip, port);
			if (0 != cid) continue;
		}
		g_tcp_close(new_fd);
	}
}

void sock_mgr::on_tcp_stream(
	struct g_eda_t *_eda, int fd, void *client, int mask)
{
	connection *conn = (connection *) client;
	sax::stage *biz = conn->mgr->_biz;
	do {
		if (mask & EDA_ERROR) {
			printf("EDA detect error for fd=%d, closed.", fd);
			break;
		}
		if (mask & EDA_READ) {
			char buf[8*1024];
			int got;
			binary &b = conn->rbuf;
			while ((got = g_tcp_read(fd,buf,sizeof(buf))) > 0) 
			{
				if (!b.ensure(got)) break;
				memcpy(b.getFree(), buf, got);
				if (!b.pour(got)) break;
			}
			
			if (!g_non_block_delayed(got)) {
				printf("tcp read or buffering error for fd=%d, closed.", fd);
				break;
			}
			
			event_body e;
			got = conn->str->decode(b, e);
			if (got < 0) {
				printf("streamer errored for fd=%d, closed.", fd);
				break;
			}
			if (got > 0) {
				if (0 == send_message(biz, conn, e)) {
					printf("send_message failed: %s", e.fn);
				}
			}
		}
		if (mask & EDA_WRITE) {
			binary &b = conn->wbuf;
			int got = b.getDataLen();
			if (got > 0) {
				got = g_tcp_write(fd, b.getData(), got);
				if (g_non_block_delayed(got)) return;
				if (got > 0) {
					b.drain(got);
					got = b.getDataLen(); // renew
				}
				else {
					printf("tcp write or buffering error for fd=%d, closed.", fd);
					break;
				}
			}
			if (got == 0) {
				g_eda_sub(conn->mgr->_eda, conn->fd, EDA_WRITE);
			}
		}
		return;
	} while(0);
	
	conn->mgr->tcp_remove(conn);
}

uint64_t sock_mgr::send_inform(
	sax::stage *biz, connection *conn, const char *fn)
{
	seda_event *ev = biz->new_event<seda_event>();
	if (ev) {
		ev->proto = conn->proto;
		ev->port = conn->port;
		g_strlcpy(ev->addr, conn->addr, sizeof(ev->addr));
		
		ev->body.dir = event_body::INFORM;
		g_strlcpy(ev->body.fn, fn, sizeof(ev->body.fn));
		ev->body.seqid = 0;
		ev->body.argument = NULL;
		
		uint64_t cid = conn->mgr->_alloc.get_key(conn);
		ev->cid = cid;
		ev->sink = conn->mgr->_owner;
		
		if (biz->push_event(ev, 0.5)) return cid;
		biz->del_event(ev);
	}
	return 0;
}

uint64_t sock_mgr::send_message(
	sax::stage *biz, connection *conn, const event_body &e)
{
	seda_event *ev = biz->new_event<seda_event>();
	if (ev) {
		ev->proto = conn->proto;
		ev->port = conn->port;
		g_strlcpy(ev->addr, conn->addr, sizeof(ev->addr));
		
		ev->body.dir = e.dir;
		g_strlcpy(ev->body.fn, e.fn, sizeof(ev->body.fn));
		ev->body.seqid = e.seqid;
		ev->body.argument = e.argument;
		
		uint64_t cid = conn->mgr->_alloc.get_key(conn);
		ev->cid = cid;
		ev->sink = conn->mgr->_owner;
		
		if (biz->push_event(ev, 0.5)) return cid;
		biz->del_event(ev);
	}
	return 0;
}

//-------------------------------------------------------------------------

static const char *c_spec_fmt = " %[^:]://%[^:]:%d";

transport::transport(sax::stage *biz)
{
	_mgr = new sock_mgr(this, biz);
}

transport::~transport()
{
	if (_mgr) delete _mgr;
}

uint64_t transport::listen(const char *spec, streamer *str)
{
	char proto[256], addr[256];
	int  port = 0;
	if (spec && str && strlen(spec)<256 && 
		3 == sscanf(spec, c_spec_fmt, proto, addr, &port))
	{
		if (strcmp(proto, "tcp") == 0) {
			int fd = g_tcp_listen(addr, port, 511);	// FIXME: magic number
			if (fd < 0) return 0;
			
			if (g_set_non_block(fd) == 0) {
				uint64_t cid = _mgr->tcp_accept(str, fd, protocol::TCP, addr, port);
				if (0 != cid) return cid;
			}
			g_tcp_close(fd); return 0;
		}
	}
	return 0;
}

uint64_t transport::connect(const char *spec, streamer *str)
{
	char proto[256], addr[256];
	int port = 0;
	if (spec && str && strlen(spec)<256 && 
		3 == sscanf(spec, c_spec_fmt, proto, addr, &port))
	{
		if (strcmp(proto, "tcp") == 0) {
			int fd = g_tcp_connect(addr, port, true);
			if (fd < 0) return 0;
			
			uint64_t cid = _mgr->tcp_stream(str, fd, protocol::TCP, addr, port);
			if (0 != cid) return cid;
			
			g_tcp_close(fd); return 0;
		}
	}
	return 0;
}

bool transport::close(uint64_t cid)
{
	connection *conn = _mgr->_alloc.get_block(cid);
	return (conn && _mgr->tcp_remove(conn));
}

bool transport::dispatch(uint64_t cid, const event_body &e)
{
	connection *conn = _mgr->_alloc.get_block(cid);
	return (sock_mgr::send_message(_mgr->_nio, conn, e) != 0);
}

class nio_handler : public sax::handler_base
{
public:
	nio_handler() : _mgr(NULL) {}
	virtual bool init(sax::timer_base *timer, void* param) 
	{
		_mgr = (sock_mgr *) param;
		_timer = timer; return true;
	}
	virtual bool on_pump(double sec)
	{
		int msec = (int)(sec*1000 + 0.5);
		g_eda_poll(_mgr->_eda, msec);
		return true;
	}
	virtual void on_event(const sax::event_type *ev, int thread_id) 
	{
		if (ev->get_type() == 0)
		{
			seda_event *se = (seda_event *) ev;
			connection *conn = _mgr->_alloc.get_block(se->cid);
			if (!conn) {
				printf("can't find the connection for: %" PRId64, se->cid);
			}
			else if (conn->mgr != _mgr) {
				printf("unmatched mgr pointers when on_event");
			}
			else if (conn->str->encode(se->body, conn->wbuf) <= 0) {
				printf("failed to encode seda_event: %s", se->body.fn);
			}
			else if (0 != g_eda_add(_mgr->_eda, conn->fd, 
				EDA_WRITE, &sock_mgr::on_tcp_stream, conn))
			{
				printf("failed to g_eda_add event for: %s", se->body.fn);
			}
		}
	}
	virtual ~nio_handler() {}
	
private:
	sock_mgr *_mgr;
	timer_base *_timer;
};

void transport::halt() 
{
	if (_mgr->_biz) _mgr->_biz->halt();
	if (_mgr->_nio) _mgr->_nio->halt();
}

bool transport::start(int queue_cap, int mem_size)
{
	if (!_mgr->_nio) {
		_mgr->_nio = create_stage<nio_handler>(
			"net_io", queue_cap, 1, true, mem_size, _mgr);
		return (NULL != _mgr->_nio);
	}
	return false;
}
//-------------------------------------------------------------------------

} // namespace
