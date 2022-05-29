#include "http/http_request.h"

const std::string HttpRequest::HTTP10{"HTTP/1.0"};
const std::string HttpRequest::HTTP11{"HTTP/1.1"};
const std::string HttpRequest::KEEP_ALIVE{"Keep-Alive"};
const std::string HttpRequest::CLOSE{"close"};

HttpRequest::HttpRequest():current_state(REQUEST_STATUS){
    request_headers.reserve(INIT_REQUEST_HEADER_SIZE);
}

//重置一个request对象
void HttpRequest::reset() {
    method.clear();
    current_state = REQUEST_STATUS;
    version.clear();
    url.clear();
    request_headers.clear();
}

//给request增加header
void HttpRequest::add_header(std::string key,std::string value) {request_headers.emplace_back(key,value);}

//根据key值获取header熟悉
std::string HttpRequest::get_header(std::string key) {
    for (size_t i = 0; i < request_headers.size(); i++) {
        if (request_headers[i].key==key) {return request_headers[i].value;}
    }
    return std::string();
}

//获得request解析的当前状态
// http_request_state HttpRequest::current_state() {return current_state;}

//根据request请求判断是否需要关闭服务器-->客户端单向连接
int HttpRequest::close_connection() {
    std::string connection = get_header("Connection");
    if (connection == CLOSE ) {return 1;}
    if (version==HTTP10 && connection!=KEEP_ALIVE) {return 1;}
    return 0;
}
