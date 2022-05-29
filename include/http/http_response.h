#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <vector>
#include <string>
#include "util/c_head.h"
#include "tcp/buffer.h"
#define INIT_RESPONSE_HEADER_SIZE 128

struct response_header {
    std::string key;
    std::string value;
    response_header(std::string key,std::string value):key(key),value(value){}
};

enum HttpStatusCode {
    Unknown,
    OK = 200,
    MovedPermanently = 301,
    BadRequest = 400,
    NotFound = 404,
};

class HttpResponse {
public:
    HttpResponse();
    void encode_buffer(Buffer &buffer);

    HttpStatusCode statusCode;
    std::string statusMessage;
    std::string contentType;
    std::string body;
    std::vector<response_header> response_headers;
    int keep_connected;
};

#endif