/*
* t_cothread.cpp
*
*  Created on: 2013-9-30
*      Author: jianxiongzhou
*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <vector>
#include <string>

#include "cothread.h"

#include <pcl.h>

#define COTHREAD_COUNT     1000
#define COTHREAD_STACKSIZE (32*1024+1)

sax::cothread_base* cothreads[COTHREAD_COUNT] = {0};

char co_getchar()
{
	sax::cothread_base* co = sax::cothread_base::get_current();
	co->yield();
	if (co->is_interrupted()) {
		printf("interrupted in co_getchar()\n");
	}
	return (char)rand() % 128;
}

class echo_cothread : public sax::cothread_base
{
public:
	echo_cothread(sax::cothread_mgr* mgr, std::vector<char>* channel, const std::string& name) :
		sax::cothread_base(mgr),
			_channel(channel), _name(name)
	{}

	void run()
	{
		char c = _channel->front();
		_channel->erase(_channel->begin());

		printf("%s running, c = %c\n", _name.c_str(), c);

		assert(is_running());

		printf("%s before co_getchar\n", _name.c_str());
		c = co_getchar();
		if (this->is_interrupted(true)) {
			printf("%s interrupted.\n", _name.c_str());
		}
		printf("%s after co_getchar\n", _name.c_str());

		assert(is_running());
	}

private:
	std::vector<char>* _channel;
	std::string _name;
};


class echo_cothread_factory : public sax::cothread_factory
{
public:
	echo_cothread_factory(std::vector<char>* channel)
	{
		_channel = channel;
	}

	sax::cothread_base* create(sax::cothread_mgr* mgr)
	{
		static int count = 0;

		char name[100];

		snprintf(name, sizeof(name), "cothread_%d", count);
		cothreads[count] = new echo_cothread(mgr, _channel, name);

		return cothreads[count++];
	}

	void destroy(sax::cothread_base* co)
	{
		delete co;
	}

private:
	std::vector<char>* _channel;
};


sax::spin_type stream_lock;
std::vector<char> ss;
volatile bool stop = false;

void* thread_func(void* params)
{
	co_thread_init();

	sax::cothread_mgr mgr(COTHREAD_COUNT, COTHREAD_STACKSIZE);

	printf("new thread started.\n");

	std::vector<char> channel;

	echo_cothread_factory factory(&channel);

	mgr.init(&factory);

	while (!stop) {
		char c;

		stream_lock.enter();
		if (ss.size() > 0) {
			c = ss.front();
			ss.erase(ss.begin());

			channel.push_back(c);
		}
		stream_lock.leave();

		bool idle = true;

		while (mgr.try_resume()) {
			idle = false;
		}

		if (channel.size() > 0) {
			idle = !mgr.call_cothread() && idle;
		}

		g_thread_sleep(0.1);
		if (idle) {
			g_thread_sleep(0.1);
		}
	}

	printf("going to stop\n");

	mgr.signal_stop(true);

	while (mgr.try_resume());	// do nothing

	printf("resumed all cothreads\n");

	co_thread_cleanup();

	return NULL;
}

int main(int argc, char* argv[])
{
	g_thread_start(thread_func, NULL);

	char c;
	while ((c = getchar()) != EOF) {
		if (c == '\n') continue;
		//printf("[main] getchar: %c\n", c);
		stream_lock.enter();
		ss.push_back(c);
		stream_lock.leave();

		int tmp = c;
		if (c >= '1' && c <= '9' && (tmp <= (int)'0' + COTHREAD_COUNT)) {
			printf("here %d\n", cothreads[c - '0' - 1]->is_suspended());

//			if (cothreads[c - '0' - 1]->is_suspended()) {
				printf("[main] going to resume cothread\n");
				int result = cothreads[c - '0' - 1]->resume();
				if (!result) {
					printf("[main] error, resume failed.\n");
				}
//			}
			g_thread_sleep(0.2);
		}
	}

	stop = true;

	g_thread_sleep(3);

	return 0;
}
