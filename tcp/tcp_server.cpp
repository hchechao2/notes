#include "tcp_server.h"

int handle_connection_established(void *data) {
    struct TCPserver *tcpServer = (struct TCPserver *) data;
    struct acceptor *acceptor = tcpServer->acceptor;
    int listenfd = acceptor->listen_fd;

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int connected_fd = accept(listenfd, (struct sockaddr *) &client_addr, &client_len);
    make_nonblocking(connected_fd);

    yolanda_msgx("new connection established, socket == %d", connected_fd);

    // choose event loop from the thread pool
    struct event_loop *eventLoop = thread_pool_get_loop(tcpServer->threadPool);

    // create a new tcp connection
    struct tcp_connection *tcpConnection = tcp_connection_new(connected_fd, eventLoop,
                                                              tcpServer->connectionCompletedCallBack,
                                                              tcpServer->connectionClosedCallBack,
                                                              tcpServer->messageCallBack,
                                                              tcpServer->writeCompletedCallBack);
    //for callback use
    if (tcpServer->data != NULL) {
        tcpConnection->data = tcpServer->data;
    }
    return 0;
}


void TcpServer::tcp_server_start() {

    //开启多个线程
    thread_pool.start();
    // thread_pool_start(tcpServer->threadPool);

    //acceptor主线程， 同时把tcpServer作为参数传给channel对象
    //acceptor 对象表示的是服务器端监听器，acceptor 对象作为一个 channel 对象，注册到 event_loop
    struct channel *channel = channel_new(acceptor.listen_fd, EVENT_READ, handle_connection_established, 
                                            NULL,tcpServer);
    
    event_loop_add_channel_event(eventLoop, channel->fd, channel);
}


void TcpServer::tcp_server_set_data(void *data) {
    if (data != nullptr) {
        m_data = data;
    }
}