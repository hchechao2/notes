#ifndef BUFFER_H
#define BUFFER_H

#include <cstring>
#include <string>
#include <algorithm>
#include "util/c_head.h"

#define INIT_BUFFER_SIZE 65536
//数据缓冲区
class Buffer {
public:
    Buffer();

    ~Buffer();
    void make_room(int size);

    int writeable_size() const;

    int readable_size() const;

    int front_spare_size() const;

    void append(char *data, int size);

    void append_char(char data);
    void append_string(std::string data);
    void append_string(char * data);

    int socket_read(int fd);

    char read_char();

    char * find_CRLF();

    char * m_data;          //实际缓冲
    int readIndex;       //缓冲读取位置
    int writeIndex;      //缓冲写入位置
    int total_size;      //总大小

};


#endif