#ifndef BUFFER_H
#define BUFFER_H


//数据缓冲区
class Buffer {
public:
    Buffer();

    ~Buffer();

    int writeable_size() const;

    int readable_size() const;

    int front_spare_size() const;

    int append( void *data, int size);

    int append_char(char data);

    int append_string(char * data);

    int socket_read(int fd);

    char read_char();

    char * find_CRLF();

private:
    char * m_data;          //实际缓冲
    int readIndex;       //缓冲读取位置
    int writeIndex;      //缓冲写入位置
    int total_size;      //总大小

    friend class TcpConnection;
};




#endif