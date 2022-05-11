#include "tcp_connection.h"

TcpConnection::TcpConnection (int connected_fd, EventLoop *eventLoop)
 :event_loop(eventLoop),name("connection-"+std:to_string(connected_fd)),
    input_buffer(),output_buffer(),channel(connected_fd, EVENT_READ, handle_read, handle_write, this)
{
    //connectionCompletedCallBack callback
    if (tcpConnection->connectionCompletedCallBack != NULL) {
        tcpConnection->connectionCompletedCallBack(tcpConnection);
    }
    //调用do_channel_event将连接套接字加入eventloop
    event_loop_add_channel_event(tcpConnection->eventLoop, connected_fd, tcpConnection->channel);
}

int TcpConnection::send_data(void *data, int size) {
    size_t nwrited = 0;
    size_t nleft = size;
    int fault = 0;

    //先往套接字尝试发送数据
    if (!channel.write_event_is_enabled() && output_buffer.readable_size() == 0) {
        nwrited = write(channel.fd, data, size);
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

    if (!fault && nleft > 0) {
        //拷贝到Buffer中，Buffer的数据由框架接管
        output_buffer.append(data + nwrited, nleft);
        if ( !channel.write_event_is_enabled() ) channel.write_event_enable();
    }

    return nwrited;
}

//在应用层server进行调用
//往 buffer 对象里写入 encode 以后的数据，调用 tcp_connection_send_buffer，将 buffer 里的数据通过套接字缓冲区发送出去
int TcpConnection::send_buffer(Buffer &buffer) {
    int size = buffer.readable_size();
    int result = send_data(buffer.data + buffer.readIndex, size);
    buffer.readIndex += size;
    return result;
}

//在应用层server进行调用
void TcpConnection::shutdown() {
    if (shutdown(channel->fd, SHUT_WR) < 0) {
        yolanda_msgx("tcp_connection_shutdown failed, socket == %d", channel->fd);
    }
}

int handle_connection_closed(struct tcp_connection *tcpConnection) {
    struct event_loop *eventLoop = tcpConnection->eventLoop;
    struct channel *channel = tcpConnection->channel;
    event_loop_remove_channel_event(eventLoop, channel->fd, channel);
    if (tcpConnection->connectionClosedCallBack != NULL) {
        tcpConnection->connectionClosedCallBack(tcpConnection);
    }
}

// channel对象的callback
// 通过调用 buffer_socket_read 函数接收来自套接字的数据流，并将其缓冲到 buffer 对象中。
// 将 buffer 对象和 tcp_connection 对象传递给应用程序真正的处理函数 messageCallBack 来进行报文的解析工作
int handle_read(void *data) {
    struct tcp_connection *tcpConnection = (struct tcp_connection *) data;
    struct buffer *input_buffer = tcpConnection->input_buffer;
    struct channel *channel = tcpConnection->channel;

    if (buffer_socket_read(input_buffer, channel->fd) > 0) {
        //应用程序真正读取Buffer里的数据
        if (tcpConnection->messageCallBack != NULL) {
            tcpConnection->messageCallBack(input_buffer, tcpConnection);
        }
    } else {
        handle_connection_closed(tcpConnection);
    }
}
//channel对象的callback
//发送缓冲区可以往外写
//把channel对应的output_buffer不断往外发送
int handle_write(void *data) {
    struct tcp_connection *tcpConnection = (struct tcp_connection *) data;
    struct event_loop *eventLoop = tcpConnection->eventLoop;
    assertInSameThread(eventLoop);

    struct buffer *output_buffer = tcpConnection->output_buffer;
    struct channel *channel = tcpConnection->channel;

    ssize_t nwrited = write(channel->fd, output_buffer->data + output_buffer->readIndex,
                            buffer_readable_size(output_buffer));
    if (nwrited > 0) {
        //已读nwrited字节
        output_buffer->readIndex += nwrited;
        //如果数据完全发送出去，就不需要继续了
        if (buffer_readable_size(output_buffer) == 0) {
            channel_write_event_disable(channel);
        }
        //回调writeCompletedCallBack
        if (tcpConnection->writeCompletedCallBack != NULL) {
            tcpConnection->writeCompletedCallBack(tcpConnection);
        }
    } else {
        yolanda_msgx("handle_write for tcp connection %s", tcpConnection->name);
    }

}