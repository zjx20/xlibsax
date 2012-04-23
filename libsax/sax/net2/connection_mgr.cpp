/*
 * connection_mgr.cpp
 *
 *  Created on: 2011-8-8
 *      Author: livingroom
 */
#include "connection_mgr.h"
#include <sstream>
#include "log.h"

namespace sax {

bool connection_mgr::init() {
    _use_len = 0;
    if(!_use_list.empty()) return false;
    _conn_map.clear();
    return true;
}

bool connection_mgr::add_connection(connection_base *conn) {
    auto_mutex lock(&_conn_mtx);

    _use_list.push_back(conn);
    _use_len++;
    //print_list(_use_list);
    return true;
}

bool connection_mgr::del_connection(connection_base *conn) {

    _use_list.erase(conn);
    _use_len--;
    //print_list(_use_list);
    delete conn;
    return true;
}

connection_base* connection_mgr::get_connection(const char *ip, const int &port) {
    int64_t key = generate_id(ip, port);
    LOG(INFO, "Get connetion key : %lld",key);
    if(key == -1) return NULL;
    connection_map::iterator it = _conn_map.find(key);
    if(it == _conn_map.end()) {
        return NULL;
    }
    return it->second;
}

//connection_base* connection_mgr::get_connection(const uint64_t &id) {
//    auto_mutex lock(&_map_mtx);
//    connection_map::iterator it = _conn_map.find(id);
//    if(it == _conn_map.end()) {
//        return NULL;
//    }
//    return it->second;
//}

int64_t connection_mgr::generate_id(const char *ip, const int &port) {
    if(NULL == ip) return -1;
    std::string str_ip = ip;
    size_t t1 = str_ip.find('.');
    if(t1 == std::string::npos) return -1;
    size_t t2 = str_ip.find('.', t1+1);
    if(t2 == std::string::npos) return -1;
    size_t t3 = str_ip.find('.', t2+1);
    if(t3 == std::string::npos) return -1;
    size_t n1 = atoi(str_ip.substr(0,t1).c_str());
    size_t n2 = atoi(str_ip.substr(t1+1,t2).c_str());
    size_t n3 = atoi(str_ip.substr(t2+1,t3).c_str());
    size_t n4 = atoi(str_ip.substr(t3+1).c_str());
    if( !(n1>=0 && n1<=256 && n2>=0 && n2<=256 && n3>=0 && n3<=256 && n4>=0 && n4<=256) ) return -1;
    return ( ((int64_t)((n1 << 24) + (n2 << 16) + (n3 << 8) + (n4))) << 16 ) + port;
}

uint32_t connection_mgr::generate_id() {
	return ++_gen_id;
}

bool connection_mgr::add_connection_map(connection_base *conn,
                             const char *ip, const int &port) {
    int64_t key = generate_id(ip, port);
    if(key == -1) return false;
    connection_map::iterator it = _conn_map.find(key);
    if(it != _conn_map.end()) return false;
    _conn_map.insert(connection_map::value_type(key, conn));
    return true;
}

//bool connection_mgr::add_connection_map(connection_base *conn, const uint64_t &id) {
//    auto_mutex lock(&_map_mtx);
//    connection_map::iterator it = _conn_map.find(id);
//    if(it != _conn_map.end()) return false;
//    _conn_map.insert(connection_map::value_type(id, conn));
//    return true;
//}

//bool connection_mgr::del_connection_map(const uint64_t &id) {
//    auto_mutex lock(&_map_mtx);
//    connection_map::iterator it = _conn_map.find(id);
//    if(it == _conn_map.end()) return false;
//    _conn_map.erase(it);
//    return true;
//}

void connection_mgr::monitor_connections() {
    LOG_INFO("connection_mgr is starting to monitor_connections...");
    _handler_run_flag = true;
    _handler_t = g_thread_start(this->monitor_connections_do, this);
}

void connection_mgr::disable_monitor() {
    LOG_INFO("connection_mgr is stopping monitor_connections...");
    _handler_run_flag = false;
    g_thread_join(_handler_t, NULL);
}

long connection_mgr::monitor_connections_do(void *data) {
    connection_mgr *conn_mgr = (connection_mgr *)data;
    LOG_INFO("monitor_connections_do thread running...%d" ,conn_mgr->_handler_run_flag);
    while(conn_mgr->_handler_run_flag) {
        conn_mgr->check_connections();

        sleep(5);
    }
    return 0;
}

void connection_mgr::check_connections() {
    auto_mutex lock(&_conn_mtx);

    connection_base *head = _use_list.head();
    while(head)
    {
        connection_base *tmp = head;
        head = head->_next;
        printf("fd : %d, is_connected : %d , task count : %ld\n",tmp->get_fd(),tmp->_is_connected, tmp->get_task_count());
        if(tmp && !tmp->_is_connected && 0 == tmp->get_task_count()) {
            printf("connection is disconnected and no task.Deleting...fd : %d\n",tmp->get_fd());
            del_connection(tmp);
        }
    }
}

void connection_mgr::print_list(connection_list &x) {
    //auto_mutex auto_mutex(_mutex);

    std::ostringstream out;
    int cnt = 0;
    connection_base *head = x.head();
    while(head)
    {
        cnt++;
        out << head->get_fd() << ",";
        head = head->_next;
    }
    LOG(INFO,"List totle len : %d",cnt);
    LOG(INFO,"list fd : %s",out.str().c_str());
}

}

