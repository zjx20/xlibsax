/*
 * t_client.cpp
 *
 *  Created on: 2011-8-15
 *      Author: livingroom
 */
#include <string.h>
#include "log.h"
#include "transport.h"
#include "gen-cpp/test_types.h"
#include "gen-cpp/test_struct_typeid.h"
using namespace sax;

class myclient : public sax::task_handler {
public:
    myclient(sax::task_streamer *streamer) :
        _inner_streamer(streamer) {}
    virtual ~myclient() {}

public:
    virtual void on_user_request(sax::task &a_task) {
        LOG(INFO,"myclient : on_user_request");
    }

    virtual void on_task_finish(sax::task &a_task) {
        LOG(INFO,"myclient : on_task_finish...\n");
        LOG(INFO,"task.trans_key  : %lld\n",a_task.trans_key);
        LOG(INFO,"task.type   :  %d\n",a_task.type);
        LOG(INFO,"task.is_call  :  %d\n",a_task.is_call);
        LOG(INFO,"task.is_need_response  :  %d\n",a_task.is_need_response);
        LOG(INFO,"task.is_finish  :  %d\n",a_task.is_finish);
        LOG(INFO,"task.is_timeout  :  %d\n",a_task.is_timeout);
        LOG(INFO,"task.is_sended  :  %d\n",a_task.is_sended);
        LOG(INFO,"task.is_received  :  %d\n",a_task.is_received);
        switch(a_task.type) {
        case test::TType::T_TEST_RES:
        {
        	test::TestRes* res = (test::TestRes*)a_task.obj;
        	LOG_INFO("server test res: %s len: %llu", res->res.c_str(), res->res.size());
        	delete res;
        	break;
        }
        default:
        {
        	LOG_WARN("unknown type, type: %d", a_task.type);
        	break;
        }
        }
    }

    bool init()
    {
        if(!_epoll_mgr.init()) return false;
        if(!_conn_mgr.init()) return false;
        _inner_streamer->set_handler(this);
        if(!_inner_transport.init(_inner_streamer, &_epoll_mgr, &_conn_mgr)) return false;
        return true;
    }

    void start()
    {
        sax::task a_task;
        a_task.trans_key = (uint64_t)12345678987654321;
        a_task.type = test::TType::T_TEST_REQ;
        a_task.is_call = true;
        a_task.is_need_response = true;

        test::TestReq* req = new test::TestReq();
        req->req = std::string("hello");
        a_task.obj = req;
        a_task.is_thrift_data = true;

        _inner_transport.send(&a_task, "127.0.0.1", 55678);

        //_task_server.listen("127.0.0.1",55679);
        _epoll_mgr.start_service(200);
    }

private:
    sax::transport _inner_transport;
    sax::task_streamer *_inner_streamer;
    epoll_mgr _epoll_mgr;
    connection_mgr _conn_mgr;
};

int main()
{
    LOG_SET_LOG_LEVEL(sax::DEBUG);

    myclient client(new sax::task_streamer());
    if(!client.init())
    {
        LOG(ERROR,"init error...");
    }
    LOG(INFO, "init ok");

    client.start();
    return 0;
}


