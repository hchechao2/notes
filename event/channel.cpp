#include "event/channel.h"

#include "event/event_loop.h"
#include "tcp/tcp_server.h"
#include "http/http_server.h"

#include "http/http_response.h"

int Channel::write_event_enable() {
    events = events | EVENT_WRITE;
    event_loop.update_channel_event(event_loop.channel_map[fd]);
    return 0;
}

int Channel::write_event_disable() {
    events = events & ~EVENT_WRITE;
    event_loop.update_channel_event(event_loop.channel_map[fd]);
    return 0;
}

int WakeChannel::run_read_callback(){
    char one;
    ssize_t n = read(event_loop.socketPair[1], &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERR("handleWakeup  failed");
    }
    yolanda_msgx("wakeup, %s", event_loop.thread_name.c_str());
    return 0;
}

int ListenChannel::run_read_callback(){
    
    int listenfd = data.acceptor.listen_fd;

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int connected_fd = accept(listenfd, (struct sockaddr *) &client_addr, &client_len);
    make_nonblocking(connected_fd);

    yolanda_msgx("new connection established, socket == %d", connected_fd);

    // 按照顺序从线程池中选择一个eventloop来唤醒
    EventLoop & sub_loop = data.thread_pool.get_loop();
    
    //只有连接套接字才会创建，主线程的监听eventloop不会
    //为这个新建立套接字创建一个tcp_connection对象，并把应用程序的callback函数设置给这个tcp_connection对象
    //其中调用do_channel_event，执行wakeup唤醒eventloop
    
    std::shared_ptr<Channel> c= std::make_shared<ConnectChannel> (connected_fd, sub_loop ,data.http_server);
    
    sub_loop.add_channel_event(c);
    
    return 0;
}


int ConnectChannel::run_read_callback(){
     if (input_buffer.socket_read(fd) > 0) {
        //应用程序真正读取Buffer里的数据
        messageCallBack();
    } else {
        event_loop.remove_channel_event(event_loop.channel_map[fd]);
        connectionClosedCallBack();
    }
    return 0;
}


int ConnectChannel::run_write_callback(){
    assertInSameThread(event_loop);
    ssize_t nwrited = write(fd, output_buffer.m_data + output_buffer.readIndex,
                            output_buffer.readable_size());
    if (nwrited > 0) {
        //已读nwrited字节
        output_buffer.readIndex += nwrited;
        //如果数据完全发送出去，就不需要继续了
        if (output_buffer.readable_size() == 0) {
            write_event_disable();
        }
        //回调writeCompletedCallBack
        writeCompletedCallBack();
        
    } else {
        yolanda_msgx("handle_write for tcp connection %d", fd);
    }
    return 0;
}


int ConnectChannel::process_status_line(char *start, char *end) {
    int size = end - start;
    //method
    char *space = reinterpret_cast<char *> (memmem(start, end - start, " ", 1));
    assert(space != NULL);
    int method_size = space - start;
    // char * tmp = reinterpret_cast<char *> (malloc(method_size + 1));
    // strncpy(tmp, start, space - start);
    // tmp[method_size + 1] = '\0';
    // request->method=tmp;
    // free(tmp);
    std::string method (start, method_size);
    request->method = std::move(method);

    //url
    start = space + 1;
    space = reinterpret_cast<char *> (memmem(start, end - start, " ", 1));
    assert(space != NULL);
    int url_size = space - start;
    // char * tmp = reinterpret_cast<char *> (malloc(url_size + 1));
    // strncpy(tmp, start, space - start);
    // tmp[url_size + 1] = '\0';
    // request->url = tmp;
    // free(tmp);
    std::string url (start, url_size);
    request->url = std::move(url);

    //version
    start = space + 1;
    assert(space != NULL);
    // char * tmp = reinterpret_cast<char *> (malloc(end - start + 1));
    // strncpy(tmp, start, end - start);
    // tmp [end - start + 1] = '\0';
    // request->version = tmp;
    // free(tmp);
    std::string version (start,end - start);
    request->version = std::move(version);
    return size;
}


int ConnectChannel::parse_http_request() {
    int ok = 1;
    while (request->current_state != REQUEST_DONE) {
        if (request->current_state == REQUEST_STATUS) {
            char *crlf = input_buffer.find_CRLF();
            if (crlf) {
                int request_line_size = process_status_line(input_buffer.m_data + input_buffer.readIndex, crlf);
                if (request_line_size) {
                    input_buffer.readIndex += request_line_size;  // request line size
                    input_buffer.readIndex += 2;  //CRLF size
                    request->current_state = REQUEST_HEADERS;
                }
            }
            else ok=0;
        } else if (request->current_state == REQUEST_HEADERS) {
            char *crlf = input_buffer.find_CRLF();
            if (crlf) {
                /**
                 *    <start>-------<colon>:-------<crlf>
                 */
                char *start = input_buffer.m_data + input_buffer.readIndex;
                int request_line_size = crlf - start;
                char *colon = reinterpret_cast<char *> (memmem(start, request_line_size ,": " , 2));
                if (colon != NULL) {
                    // char *key = reinterpret_cast<char *> (malloc(colon - start + 1));
                    // strncpy(key, start, colon - start);
                    // key[colon - start] = '\0';
                    std::string key (start, colon - start) ;
                    // char *value = reinterpret_cast<char *> (malloc(crlf - colon - 2 + 1));
                    // strncpy(value, colon + 2, crlf - colon - 2);
                    // value[crlf - colon - 2] = '\0';
                    std::string value (colon + 2, crlf - colon - 2) ;

                    request->add_header(key, value);

                    input_buffer.readIndex += request_line_size;  //request line size
                    input_buffer.readIndex += 2;  //CRLF size
                } else {
                    //读到这里说明:没找到，就说明这个是最后一行
                    input_buffer.readIndex += 2;  //CRLF size
                    request->current_state = REQUEST_DONE;
                }
            }
        }
    }
    return ok;
}

// buffer是框架构建好的，并且已经收到部分数据的情况下
// 注意这里可能没有收到全部数据，所以要处理数据不够的情形
int ConnectChannel::messageCallBack() {
    yolanda_msgx("get message from tcp connection %d", fd);

    if (parse_http_request() == 0) {
        char error_response[] =  "HTTP/1.1 400 Bad Request\r\n\r\n";
        
        send_data(error_response, strlen(error_response));

        shutdownfd();
    }

    //处理完了所有的request数据，接下来进行编码和发送
    if (request->current_state == REQUEST_DONE) {
        HttpResponse http_response{};

        //httpServer暴露的requestCallback回调
        if (server.requestCallback) {
            server.requestCallback(*request, http_response);
        }
        Buffer buffer{};
        http_response.encode_buffer(buffer);
        send_buffer(buffer);

        if (request->close_connection()) {
            shutdownfd();
        }
	    request->reset();
    }
    return 0;
}

//数据通过buffer写完之后的callback
int ConnectChannel::writeCompletedCallBack() {
    yolanda_msgx("write completed");
    return 0;
}

//连接关闭之后的callback
int ConnectChannel::connectionClosedCallBack() {
    yolanda_msgx("connection closed");
    request.reset();
    return 0;
}


int ConnectChannel::send_data(void *data, int size) {
    size_t nwrited = 0;
    size_t nleft = size;
    int fault = 0;

    //先往绕过Buffer套接字尝试发送数据
    if (!write_event_is_enabled() && output_buffer.readable_size() == 0) {
        nwrited = write(fd, data, size);
        if (nwrited >= 0) {
            nleft = nleft - nwrited;
        } else {
            nwrited = 0;
            if (errno != EWOULDBLOCK) {
                if (errno == EPIPE || errno == ECONNRESET) {
                    fault = 1;
                }
            }
        }
    }

    //拷贝到Buffer中，Buffer的数据由框架接管
    if (!fault && nleft > 0) {
        output_buffer.append( reinterpret_cast <char*> (data) + nwrited, nleft);
        if ( !write_event_is_enabled() ) write_event_enable();
    }
    return nwrited;
}

//在应用层server进行调用
//往 buffer 对象里写入 encode 以后的数据，调用 tcp_connection_send_buffer，将 buffer 里的数据通过套接字缓冲区发送出去
int ConnectChannel::send_buffer(Buffer & buffer) {
    int size = buffer.readable_size();
    int result = send_data(buffer.m_data + buffer.readIndex, size);
    buffer.readIndex += size;
    return result;
}

//在应用层server进行调用
void ConnectChannel::shutdownfd() {
    if ( shutdown(fd, SHUT_WR) < 0) {
        yolanda_msgx("tcp_connection_shutdown failed, socket == %d", fd);
    }
}