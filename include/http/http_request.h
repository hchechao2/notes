#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <vector>
#include "util/c_head.h"

#define INIT_REQUEST_HEADER_SIZE 128

struct request_header {
    std::string key;
    std::string value;
    request_header(std::string key,std::string value):key(key),value(value){}
};

enum http_request_state {
    REQUEST_STATUS,    //等待解析状态行
    REQUEST_HEADERS,   //等待解析headers
    REQUEST_BODY,      //等待解析请求body
    REQUEST_DONE       //解析完成
};

class HttpRequest {
public:
    HttpRequest();
    void reset();
    void add_header(std::string key,std::string value);
    std::string get_header(std::string key);
    int close_connection();
    std::string version;
    std::string method;
    std::string url;
    http_request_state current_state;
    std::vector<request_header> request_headers;

    const static std::string HTTP10;
    const static std::string HTTP11;
    const static std::string KEEP_ALIVE;
    const static std::string CLOSE;
};


#endif