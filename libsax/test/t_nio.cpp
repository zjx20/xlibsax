#include <sax/netutil.h>
#include <stdio.h>

class myhandler : public sax::handler_base
{
	virtual bool init(sax::timer_base *timer, void* param) {return true;}
	virtual void on_event(const sax::event_type *ev, int thread_id) 
	{
		printf("%d: t=%d, p=%p\n", thread_id, ev->get_type(), ev);
		sax::seda_event *se = (sax::seda_event *) ev;
		printf("fn=%s, args=%p, cid=%"PRId64"\n", se->body.fn, se->body.argument, se->cid);
		if (se->body.dir == sax::event_body::REQUEST) se->sink->dispatch(se->cid, se->body);
	}
	virtual ~myhandler() {}
};

class mystreamer : public sax::streamer
{
public:
	int decode(sax::binary &b, sax::event_body &e)
	{
		printf("decode: %d, %d: %s\n", b.getDataLen(), b.getFreeLen(), b.getData());
		e.dir = sax::event_body::REQUEST;
		g_strlcpy(e.fn, "TestHello", sizeof(e.fn));
		e.seqid = 100;
		e.argument = NULL;
		int got = b.getDataLen();
		b.drain(got);
		return 1;
	}
	int encode(sax::event_body &e, sax::binary &b)
	{
		const char msg[] = "hello world.";
		int len = sizeof(msg)-1;
		b.ensure(len);
		memcpy(b.getFree(), msg, len);
		b.pour(len);
		printf("encode: %d, %d\n", b.getDataLen(), b.getFreeLen());
		return 1;
	}
	~mystreamer() {}
};


void test_nio()
{
	sax::stage *biz = sax::create_stage<myhandler>(
		"biz_%d", 16, 3, false, 1024*1024, NULL);
	sax::transport tr(biz);
	mystreamer st;
	tr.listen("tcp://127.0.0.1:12340", &st);
	tr.start(16, 1024*1024);
	getchar();
	
	tr.halt(); delete biz;
}

int main( int argc, char *argv[] )
{
	sax::global_init();
	test_nio();
	sax::global_free();
	return 0;
}
