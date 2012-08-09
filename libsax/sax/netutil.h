#ifndef __NETUTIL_H_2011__
#define __NETUTIL_H_2011__

#include "os_types.h"
#include "binary.h"
#include "c++/stage.h"

namespace sax {

bool global_init();
void global_free();

struct protocol {
	enum type {TCP=0, UDP=1, FILE=2};
};

/// @brief a unified structure for containing event body data
struct event_body
{
	enum type {ONEWAY=1, REQUEST=2, ANSWER=3, INFORM=4};
	
	uint32_t seqid;
	char fn[36];
	event_body::type dir;
	void *argument;
};

/// @brief streamer define interfaces for [event] serialization
struct streamer {
	virtual int decode(sax::binary &b, sax::event_body &e) = 0;
	virtual int encode(sax::event_body &e, sax::binary &b) = 0;
	virtual ~streamer() {}
};

/// @note spec should be like: tcp://127.0.0.1:15000
class transport : public nocopy
{
public:
	transport(sax::stage *biz);
	~transport();
	uint64_t listen(const char *spec, streamer *str);
	uint64_t connect(const char *spec, streamer *str);
	bool close(uint64_t cid);
	bool dispatch(uint64_t cid, const event_body &e);
	
	void halt();
	bool start(int queue_cap, int mem_size);
	
private:
	struct sock_mgr *_mgr;
};

struct seda_event : public sax_event_base<0>
{
	protocol::type proto;
	int  port;
	char addr[24];
	
	sax::event_body body;
	
	uint64_t cid;
	sax::transport *sink;
};

} // namespace

#endif//__NETUTIL_H_2011__
