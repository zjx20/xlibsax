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
    if( !(_mutex = g_mutex_init()) ) return false;
    return true;
}

bool connection_mgr::add_connection(connection_base *conn) {
    //auto_mutex auto_mutex(_mutex);

    _use_list.push_back(conn);
    _use_len++;
    print_list(_use_list);
    return true;
}

bool connection_mgr::del_connection(connection_base *conn) {
    //auto_mutex auto_mutex(_mutex);

    _use_list.erase(conn);
    _use_len--;
    print_list(_use_list);
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

bool connection_mgr::add_connection_map(connection_base *conn,
                             const char *ip, const int &port) {
    int64_t key = generate_id(ip, port);
    if(key == -1) return false;
    connection_map::iterator it = _conn_map.find(key);
    if(it != _conn_map.end()) return false;
    _conn_map.insert(connection_map::value_type(key, conn));
    return true;
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

