/**
 * @file sysutil.cpp
 * @brief implement for c++ wrapper defined in os_api.h
 *
 * @author Qingshan
 * @date 2011-3-28
 */

#include "sysutil.h"

namespace sax {

struct PRIV_INFO
{
	thread_mgr *mgr_;
	int id_;
	PRIV_INFO(thread_mgr *mgr, int id): mgr_(mgr), id_(id) {}
};

thread_mgr::thread_mgr(): _stop_flag(1), _num_alive(0)
{
}

thread_mgr::~thread_mgr()
{
	this->stop();
}

bool thread_mgr::is_stopped()
{
	return _stop_flag;
}

long thread_mgr::proc(void *user)
{
	PRIV_INFO *info = (PRIV_INFO *) user;
	thread_mgr *mgr = info->mgr_;
	
	int id = info->id_;
	delete info; // release the info from mgr
	
	g_lock_add((long *)&mgr->_num_alive, +1);
	
	mgr->on_routine(id); // loop biz
	
	g_lock_add((long *)&mgr->_num_alive, -1);
	
	return 0;
}

int thread_mgr::run(int n)
{
	// check whether stopped!
	if (_num_alive > 0) return -1;
	
	_stop_flag = 0;
	g_lock_set((long *)&_num_alive, 0);
	
	int j = 0;
	for (; j < n; j++)
	{
		PRIV_INFO *info = new PRIV_INFO(this, j);
		int ret = g_thread_start_detached(&thread_mgr::proc, info);
		if (!ret) break;
	}
	return j;
}

void thread_mgr::stop()
{
	_stop_flag = 1;
	while (_num_alive > 0)
	{
		g_thread_sleep(0.05); // wait 50 ms
	}
}


void timer_base::proc(uint32_t id, void *client, void *param)
{
	timer_base *t = (timer_base *) client;
	t->on_timeout(id, param);
}

} // namespace

