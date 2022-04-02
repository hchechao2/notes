#ifndef TCP_SERVER_H
#define TCP_SERVER_H

class TcpServer {
public:
    TcpServer(size_t thread_num):event_loop(),acceptor(),thread_pool(),thread_num(thread_num),data(nullptr) {}
    virtual int connection_completed_call_back(TcpConnection &tcpConnection);
    virtual int message_call_back(Buffer &buffer,TcpConnection &tcpConnection);
    virtual int rite_completed_call_back(TcpConnection &tcpConnection);
    virtual int connection_closed_call_back(TcpConnection &tcpConnection);
    virtual ~TcpServer(){
    void tcp_server_start(); //开始监听
    void tcp_server_set_data(); // 设置callback 数据

    }
private:
    size_t port;
    size_t thread_num;
    EventLoop* event_loop;
    Acceptor acceptor;
    ThreadPool thread_pool;
    void * m_data; //for callback http_server 
};

#endif
