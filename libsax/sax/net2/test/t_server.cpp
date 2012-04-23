/*
 * t_main.cpp
 *
 *  Created on: 2011-8-4
 *      Author: livingroom
 */
#include <string.h>
#include "log.h"
#include "transport.h"
#include "tcp_connection.h"
#include "task_streamer.h"
#include "thrift_task_streamer.h"
#include "gen-cpp/test_types.h"
#include "gen-cpp/test_struct_typeid.h"
#include "time.h"
#include "sax/os_api.h"

using namespace sax;

class myserver : public sax::task_handler, public sax::thrift_task_handler {
public:
    myserver(sax::task_streamer *tk_streamer,sax::thrift_task_streamer *tt_streamer) :
    	_inner_streamer(tk_streamer), _outer_streamer(tt_streamer) {}
    virtual ~myserver() {}

public:
    virtual void on_user_request(sax::task* a_task) {
        LOG(INFO,"myserver : on_user_request...");

        LOG(INFO,"task.trans_key  : %lld\n",a_task->trans_key);
        LOG(INFO,"task.type   :  %d\n",a_task->type);
        LOG(INFO,"task.is_call  :  %d\n",a_task->is_call);
        LOG(INFO,"task.is_need_response  :  %d\n",a_task->is_need_response);
        LOG(INFO,"task.is_finish  :  %d\n",a_task->is_finish);
        LOG(INFO,"task.is_timeout  :  %d\n",a_task->is_timeout);
        LOG(INFO,"task.is_sended  :  %d\n",a_task->is_sended);
        LOG(INFO,"task.is_recieved  :  %d\n",a_task->is_recieved);

        switch(a_task->type) {
        case test::TType::T_TEST_REQ:
        {
        	test::TestReq* req = (test::TestReq*)a_task->obj;
        	LOG_INFO("req: %d", *((int*)req));
        	LOG_INFO("client test req: %s len: %llu", req->req.c_str(), req->req.size());
        	test::TestRes* res = new test::TestRes();
        	res->res = "world";
        	_inner_transport.response(a_task, res, test::TType::T_TEST_RES);
        	delete req;
        	break;
        }
        default:
        {
        	LOG_WARN("unknown type, type: %d", a_task->type);
        	break;
        }
        }

        delete a_task;
    }

    virtual void on_task_finish(sax::task* a_task) {

    }

    virtual void on_thrift_client_request(thrift_task &a_task) {
        LOG(INFO,"myserver : on_thrift_client_request...");
        LOG(INFO,"a_task.seqid           : %d",a_task.seqid);
        LOG(INFO,"a_task.fname.c_str()   :  %s",a_task.fname.c_str());
        LOG(INFO,"a_task.ttype           :  %d",a_task.ttype);
        LOG(INFO,"a_task.obj_type        :  %d\n",a_task.obj_type);

        //printf("%u , %u\n",_count,_last_count);
        if(_count > 10000) {
            int64_t cur = g_now_ms();
            printf("1\n");
            printf("QPS : %lf ,  pass time : %lf",1.0 * _count/(1.0*(cur-_start)/1000),(1.0*(cur-_start)/1000));
            _count = 0;
            _start = cur;
        }

        switch(a_task.obj_type) {
        case test::TType::T_TEST_REQ:
        {
            test::TestReq* req = (test::TestReq*)a_task.obj;
            LOG_INFO("req: %d", *((int*)req));
            LOG_INFO("client test req: %s len: %llu", req->req.c_str(), req->req.size());
            test::TestRes* res = new test::TestRes();
            res->res = "world";
            _outer_transport.response(&a_task, res, test::TType::T_TEST_RES);
            delete req;
            _count++;
            break;
        }
        default:
        {
            LOG_WARN("unknown type, type: %d", a_task.obj_type);
            break;
        }
        }
    }

    bool init()
    {
        _start = g_now_ms();
        _count = 0;
        _last_count = 0;
        if(!_epoll_mgr.init()) return false;
        if(!_conn_mgr.init()) return false;
        _inner_streamer->set_handler(this);
        _outer_streamer->set_handler(this);
        if(!_inner_transport.init(_inner_streamer, &_epoll_mgr, &_conn_mgr)) return false;
        if(!_outer_transport.init(_outer_streamer, &_epoll_mgr, &_conn_mgr)) return false;
        return true;
    }

    void start()
    {
        _outer_transport.listen("0.0.0.0",55876);
        _outer_streamer->start_handle();
        _conn_mgr.monitor_connections();
        _epoll_mgr.start_service(500);
    }

private:
    sax::transport _inner_transport;
    sax::task_streamer *_inner_streamer;
    sax::transport _outer_transport;
    sax::thrift_task_streamer *_outer_streamer;
    epoll_mgr _epoll_mgr;
    connection_mgr _conn_mgr;
    uint32_t _count;
    uint32_t _last_count;
    int64_t  _start;
};

int main()
{
    myserver server(new sax::task_streamer(), new sax::thrift_task_streamer());
    if(!server.init())
    {
        LOG(ERROR,"init error..\n");
    }
    LOG(INFO,"init ok.\n");
    server.start();
    return 0;
}

