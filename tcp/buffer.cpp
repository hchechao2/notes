#include "Buffer.h"

Buffer::Buffer():readIndex (0),writeIndex(0),total_size(65536) {
    m_data=new char[total_size];
}

Buffer::~Buffer() {
    delete m_data;
}

int Buffer::writeable_size() const{
    return total_size -writeIndex;
}

int Buffer::readable_size() const{
    return writeIndex - readIndex;
}

int Buffer::front_spare_size() const{
    return readIndex;
}

void Buffer::make_room(int size) {
    if (writeable_size(buffer) >= size) {
        return;
    }
    //如果front_spare和writeable的大小加起来可以容纳数据，则把可读数据往前面拷贝
    if (front_spare_size(buffer) + writeable_size(buffer) >= size) {
        int readable = readable_size(buffer);
        
        //有重叠区域，避免内存错误使用循环
        for (int i = 0; i < readable; i++) {
            memcpy(m_data + i, m_data + readIndex + i, 1);
        }

        readIndex = 0;
        writeIndex = readable;
    } else {
        char * tmp = new char [total_size + size];
        std::copy(m_data, m_data+total_size, tmp);
        //void *tmp = realloc(buffer->data, buffer->total_size + size);
        delete m_data;
        total_size += size;
        m_data = tmp;
    }
}

int Buffer::append(void *data, int size) {
    if (data != NULL) {
        make_room(size);
        //拷贝数据到可写空间中
        std::copy(data, data+size, m_data);
        // memcpy(data + writeIndex, data, size);
        writeIndex += size;
    }
}

int Buffer::append_char(char data) {
    make_room(1);
    //拷贝数据到可写空间中
    data[writeIndex++] = data;
}

int Buffer::append_string(char *data) {
    if (data != NULL) {
        int size = strlen(data);
        append(data, size);
    }
}


int Buffer::socket_read(int fd) {
    char additional_buffer[INIT_BUFFER_SIZE];
    struct iovec vec[2];
    int max_writable = buffer_writeable_size(buffer);
    vec[0].iov_base = buffer->data + buffer->writeIndex;
    vec[0].iov_len = max_writable;
    vec[1].iov_base = additional_buffer;
    vec[1].iov_len = sizeof(additional_buffer);
    int result = readv(fd, vec, 2);
    if (result < 0) {
        return -1;
    } else if (result <= max_writable) {
        buffer->writeIndex += result;
    } else {
        buffer->writeIndex = buffer->total_size;
        append(buffer, additional_buffer, result - max_writable);
    }
    return result;
}

char Buffer::read_char() {
    char c = data[readIndex++];
    return c;
}

const char *CRLF = "\r\n";

char * Buffer::find_CRLF() {
    char *crlf = memmem(buffer->data + buffer->readIndex, buffer_readable_size(buffer), CRLF, 2);
    return crlf;
}