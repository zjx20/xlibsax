#include <sax/c++/stage.h>
#include <stdio.h>

class myhandler : public sax::handler_base
{
	virtual bool init(sax::timer_base *timer, void* param) {return true;}
	virtual void on_start(int thread_id) {printf("start: %d\n", thread_id);}
	virtual void on_event(const sax::event_type *ev, int thread_id) {printf("%d: t=%d, p=%p\n", thread_id, ev->get_type(), ev);}
	virtual void on_finish(int thread_id) {printf("finish: %d\n", thread_id);}
	virtual ~myhandler() {}
};

struct myevent : public sax::event_base<100>
{
	int x, y;
	double z;
};

int main( int argc, char *argv[] )
{
	sax::stage *s = sax::create_stage<myhandler>(
		"my_stage_%d", 16, 3, false, 1024*1024, NULL);
	myevent *e;
	
	e = s->new_event<myevent>();
	printf( "ev=%p\n", e);
	s->push_event(e);

	e = s->new_event<myevent>();
	printf( "ev=%p\n", e);
	s->push_event(e);
	
	getchar();
	delete s; return 0;
}
