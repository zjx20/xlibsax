#include <sax/stage/stage.h>
#include <stdio.h>
#include <string>

volatile size_t sink_counter = 0;
volatile size_t source_counter = 0;

const size_t stop_counter = 10000000ull;

struct test_event : public sax::user_event_base</*sax::event_type::USER_TYPE_START+*/1, test_event>
{
	int a,b;
	std::string* s;
};

struct test_event1 : public sax::user_event_base</*sax::event_type::USER_TYPE_START+*/2, test_event1>
{
	int a;
	std::string* s;
};

struct test_event2 : public sax::user_event_base</*sax::event_type::USER_TYPE_START+*/3, test_event2>
{
	int a,b;
};

class source : public sax::handler_base
{
	virtual bool init(void* param) {_dest = (sax::stage*)param; return true;}
	virtual void on_start(int thread_id)
	{
		printf("source start: %d\n", thread_id);

		while (1) {
			//test_event* ev = test_event::new_event();
			//if (ev == NULL) printf("line:%d new event return NULL.\n", __LINE__);
			test_event ev;
			_dest->push_event(&ev);
			source_counter++;
//			if (source_counter % 100000 == 0) {
//				g_thread_sleep(0.01);
//			}
			if (source_counter >= stop_counter) {
				break;
			}
		}
	}
	virtual void on_event(const sax::event_type *ev) {}
	virtual void on_finish(int thread_id) {printf("source finish: %d\n", thread_id);}
	virtual ~source() {}
private:
	sax::stage* _dest;
};

class sink : public sax::handler_base
{
	virtual bool init(void* param) {return true;}
	virtual void on_start(int thread_id) {printf("sink start: %d\n", thread_id);}
	virtual void on_event(const sax::event_type *ev)
	{
		sink_counter++;
	}
	virtual void on_finish(int thread_id) {printf("sink finish: %d\n", thread_id);}
	virtual ~sink() {}
};

class midware : public sax::handler_base
{
public:
	virtual bool init(void* param) {_dest = (sax::stage*)param; return true;}
	virtual void on_start(int thread_id) {printf("midware start: %d\n", thread_id);}
	virtual void on_event(const sax::event_type *ev)
	{
		//test_event1* ev1 = test_event1::new_event();
		//if (ev1 == NULL) printf("line:%d new event return NULL.\n", __LINE__);
		test_event1 ev1;
		_dest->push_event(&ev1);
	}
	virtual void on_finish(int thread_id) {printf("midware finish: %d\n", thread_id);}
	virtual ~midware() {}
private:
	sax::stage* _dest;
};

int main( int argc, char *argv[] )
{
	sax::stage* ssink = sax::create_stage<sink, sax::thread_obj>(
			"sink", 1, NULL, 102400, new sax::single_dispatcher());

	sax::stage* smid = sax::create_stage<midware, sax::thread_obj>(
			"midware", 2, ssink, 102400, new sax::default_dispatcher());

	sax::stage* ssource = sax::create_stage<source, sax::thread_obj>(
			"source", 1, smid, 102400, new sax::single_dispatcher());
	
	size_t last_sink_counter = sink_counter;
	size_t last_source_counter = source_counter;
	while (1) {
		g_thread_sleep(1);
		size_t curr_sink_counter = sink_counter;
		size_t curr_source_counter = source_counter;
		printf("source: %lu sink: %lu source_rate: %lu sink_rate: %lu\n",
				curr_source_counter, curr_sink_counter,
				curr_source_counter - last_source_counter,
				curr_sink_counter - last_sink_counter);
		last_sink_counter = curr_sink_counter;
		last_source_counter = curr_source_counter;

		if (sink_counter >= stop_counter) {
			break;
		}
	}

	sax::stage_mgr::get_instance()->stop_all();

	return 0;
}
