#include "tcp/buffer.h"

Buffer::Buffer():readIndex (0),writeIndex(0),total_size(65536) {
    m_data=new char[total_size];
}

Buffer::~Buffer() {
    delete[] m_data;
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
    if (writeable_size() >= size) {
        return;
    }
    //如果front_spare和writeable的大小加起来可以容纳数据，则把可读数据往前面拷贝
    if (front_spare_size() + writeable_size() >= size) {
        int readable = readable_size();
        
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

void Buffer::append(char *data, int size) {
    if (data != nullptr) {
        make_room(size);
        
        //std::copy  
        //memcpy
        //realloc

        std::copy(data, data+size, m_data + writeIndex);
        // memcpy(data + writeIndex, data, size);
        writeIndex += size;
    }
}

void Buffer::append_char(char data) {
    make_room(1);
    //拷贝数据到可写空间中
    m_data[writeIndex++] = data;
}

void Buffer::append_string(char *data) {
    if (data != NULL) {
        int size = strlen(data);
        append(data, size);
    }
}

void Buffer::append_string(std::string data) {
    int size=data.size();
    make_room(size);
    std::copy(data.begin(), data.end(), m_data + writeIndex);
    writeIndex += size;
}

int Buffer::socket_read(int fd) {
    char additional_buffer[INIT_BUFFER_SIZE];
    struct iovec vec[2];
    int max_writable = writeable_size();
    vec[0].iov_base = m_data + writeIndex;
    vec[0].iov_len = max_writable;
    vec[1].iov_base = additional_buffer;
    vec[1].iov_len = sizeof(additional_buffer);
    int result = readv(fd, vec, 2);
    if (result < 0) {
        return -1;
    } else if (result <= max_writable) {
        writeIndex += result;
    } else {
        writeIndex = total_size;
        append(additional_buffer, result - max_writable);
    }
    return result;
}

char Buffer::read_char() {
    char c = m_data[readIndex++];
    return c;
}

const char *CRLF = "\r\n";

char * Buffer::find_CRLF() {
    char *crlf = reinterpret_cast<char *> (memmem(m_data + readIndex, readable_size(), CRLF, 2));
    return crlf;
}