#include "tcp/tcp_server.h"
#include "http/http_server.h"
#include "event/channel.h"

TcpServer::TcpServer(size_t thread_num,int port, HttpServer& http_server)
    :event_loop(),acceptor(port),thread_pool(event_loop,thread_num),http_server(http_server){}

void TcpServer::start() {
    //初始化并让子线程进入loop等待本地besocket信号
    thread_pool.start();
    //acceptor主线程， 同时把tcpServer作为参数传给channel对象
    //acceptor 对象表示的是服务器端监听器，acceptor 对象作为一个 channel 对象，注册到 event_loop
    std::shared_ptr<Channel> c = std::make_shared<ListenChannel> (acceptor.listen_fd, EVENT_READ, event_loop,*this);

    event_loop.add_channel_event(c);
}

