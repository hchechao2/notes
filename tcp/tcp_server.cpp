#include "tcp_server.h"

TcpServer::TcpServer(EventLoop& event_loop,size_t thread_num,int port)
    :event_loop(event_loop),acceptor(port),thread_pool(thread_num),port(port),thread_num(thread_num),data(nullptr) {}

//主线程监听listen_fd，有新连接时执行的函数
int handle_connection_established(void *data) {
    struct TCPserver *tcpServer = (struct TCPserver *) data;
    struct acceptor *acceptor = tcpServer->acceptor;
    int listenfd = acceptor->listen_fd;

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int connected_fd = accept(listenfd, (struct sockaddr *) &client_addr, &client_len);
    make_nonblocking(connected_fd);

    yolanda_msgx("new connection established, socket == %d", connected_fd);

    // 按照顺序从线程池中选择一个eventloop来唤醒
    EventLoop eventLoop = thread_pool.get_loop();
    
    //只有连接套接字才会创建，主线程的监听eventloop不会
    //为这个新建立套接字创建一个tcp_connection对象，并把应用程序的callback函数设置给这个tcp_connection对象
    //其中调用do_channel_event，执行wakeup唤醒eventloop
    
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


void TcpServer::start() {

    //初始化并让子线程进入loop等待本地besocket信号
    thread_pool.start();
    
    //acceptor主线程， 同时把tcpServer作为参数传给channel对象
    //acceptor 对象表示的是服务器端监听器，acceptor 对象作为一个 channel 对象，注册到 event_loop
    struct channel *channel = channel_new(acceptor.listen_fd, EVENT_READ, tcpServer,handle_connection_established, 
                                            NULL);
    
    event_loop.add_channel_event(channel->fd, channel);
}


void TcpServer::set_data(void *data) {
    if (data != nullptr) {
        m_data = data;
    }
}