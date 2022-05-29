#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <memory>

#include "tcp/acceptor.h"
#include "event/event_loop.h"
#include "tcp/thread_pool.h"

class HttpServer;

class TcpServer {
public:
    TcpServer(size_t thread_num,int port, HttpServer& http_server);
    virtual ~TcpServer(){}
    void start(); //开始监听
    //composition
    EventLoop event_loop;
    Acceptor acceptor;
    ThreadPool thread_pool;
    //aggregation
    HttpServer& http_server; //for callback http_server 
};


#endif
