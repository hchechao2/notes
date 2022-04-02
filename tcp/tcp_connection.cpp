#include "tcp_connection.h"

TcpConnection::TcpConnection (int connected_fd, EventLoop *eventLoop)
 :eventLoop(event_loop),name("connection-"+std:to_string(connected_fd)),
    input_buffer(),output_buffer(),channel()
{
    //connectionCompletedCallBack callback
    if (tcpConnection->connectionCompletedCallBack != NULL) {
        tcpConnection->connectionCompletedCallBack(tcpConnection);
    }
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

int TcpConnection::send_buffer(Buffer &buffer) {
    int size = buffer.readable_size();
    int result = send_data(buffer.data + buffer.readIndex, size);
    buffer.readIndex += size;
    return result;
}

void TcpConnection::shutdown() {
    if (shutdown(channel->fd, SHUT_WR) < 0) {
        yolanda_msgx("tcp_connection_shutdown failed, socket == %d", channel->fd);
    }
}