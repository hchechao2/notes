#ifndef TCP_CONNECTION
#define TCP_CONNECTION
#include <string>
#include "event/event_loop.h"
#include "tcp/channel.h"
#include "tcp/buffer.h"
//#include "tcp/tcp_server.h"



//tcp_connection 是大部分应用程序和框架直接打交道的数据结构
//避免把 channel 对象暴露给应用程序
class TcpConnection {
public:
    TcpConnection(int connected_fd, EventLoop *eventLoop);

    EventLoop *event_loop;
    Channel channel;
    string name;
    Buffer input_buffer;   //接收缓冲区
    Buffer output_buffer;  //发送缓冲区

    connection_completed_call_back connectionCompletedCallBack;
    message_call_back messageCallBack;
    write_completed_call_back writeCompletedCallBack;
    connection_closed_call_back connectionClosedCallBack;
    int send_data();
    int send_buffer();
    void shutdown();
private:
    void * data; //for callback use: http_server
    void * request; // for callback use
    void * response; // for callback use
};

#endif