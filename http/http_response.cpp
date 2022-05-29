#include "http/http_response.h"
#include <algorithm>
HttpResponse::HttpResponse():keep_connected(0){
    response_headers.reserve(INIT_RESPONSE_HEADER_SIZE);
}

void HttpResponse::encode_buffer(Buffer & output) {
    char buf[32];
    snprintf(buf, sizeof buf, "HTTP/1.1 %d ", statusCode);
    output.append_string(buf);
    output.append_string(statusMessage);
    output.append_string((char *)"\r\n");



    if (keep_connected) {
        output.append_string((char *)"Connection: close\r\n");
    } else {
        snprintf(buf, sizeof buf, "Content-Length: %zd\r\n",body.size());
        output.append_string(buf);
        output.append_string((char *)"Connection: Keep-Alive\r\n");
    }

    
    for (size_t i = 0; i < response_headers.size(); i++) {
        output.append_string(response_headers[i].key);
        output.append_string((char *)": ");
        output.append_string(response_headers[i].value);
        output.append_string((char *)"\r\n");
    }
    

    output.append_string((char *)"\r\n");
    output.append_string(body);
}