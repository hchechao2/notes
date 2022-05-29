// #define _GNU_SOURCE
#include "http/http_server.h"
#include "http/http_request.h"
#include "http/http_response.h"

HttpServer::HttpServer(int port,request_callback requestCallback,int threadNum)
    :requestCallback(requestCallback) ,server(threadNum,port,* this){}

void HttpServer::start() {
    server.start();
    server.event_loop.run();
}
