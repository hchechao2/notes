#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <functional>
#include "util/c_head.h"
#include "tcp/tcp_server.h"
#include "http/http_request.h"
#include "http/http_response.h"


using request_callback=std::function<int(HttpRequest &http_request, HttpResponse &http_response)>;

class HttpServer{
public:
    request_callback requestCallback;
    HttpServer(int port,request_callback requestCallback,int threadNum);
    void start();
private:
    TcpServer server;
};

#endif