#ifndef TCP_SERVER_H
#define TCP_SERVER_H

class TcpServer {
public:
    TcpServer(size_t thread_num);
    virtual int connection_completed_call_back(TcpConnection &tcpConnection);
    virtual int message_call_back(Buffer &buffer,TcpConnection &tcpConnection);
    virtual int write_completed_call_back(TcpConnection &tcpConnection);
    virtual int connection_closed_call_back(TcpConnection &tcpConnection);
    virtual ~TcpServer(){}
    void start(); //开始监听
    void set_data(); // 设置callback 数据

    
private:
    size_t port;
    size_t thread_num;
    //主reactor
    EventLoop& event_loop;
    Acceptor acceptor;
    //从reactor
    ThreadPool thread_pool;
    void * m_data; //for callback http_server 
};



#endif
