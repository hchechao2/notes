#ifndef CHANNEL_H
#define CHANNEL_H

#include <iostream>
#include <memory>
#include "util/c_head.h"
#include "util/utils.h"
#include "tcp/buffer.h"
#include "http/http_request.h"

#define EVENT_TIMEOUT    0x01
/** Wait for a socket or FD to become readable */
#define EVENT_READ        0x02
/** Wait for a socket or FD to become writeable */
#define EVENT_WRITE    0x04
/** Wait for a POSIX signal to be raised*/
#define EVENT_SIGNAL    0x08

class TcpServer;
class EventLoop;
class HttpServer;

class Channel {
public:
    Channel(int fd, int events,EventLoop & event_loop)
        :event_loop(event_loop),fd(fd),events(events){}

    int write_event_is_enabled(){return events & EVENT_WRITE;}
    void set_type(int t){type=t;}

    int write_event_enable();
    int write_event_disable();
    virtual int run_read_callback(){ return 0;}
    virtual int run_write_callback(){ return 0;}
    virtual  ~Channel(){std::cout<<"channel destoryed"<<std::endl;}
    EventLoop & event_loop;
    int fd;
    int type;
    int events;   //表示event类型
};




class WakeChannel:public Channel{
public:
    WakeChannel(int fd, int events,EventLoop & event_loop)
        :Channel(fd, events,event_loop){}

    int run_read_callback() override;
};


class ListenChannel:public Channel{
public:
    
    ListenChannel(int fd, int events,EventLoop & event_loop,TcpServer & data)
        :Channel(fd, events,event_loop),data(data){}

    int run_read_callback() override;

private:
    TcpServer & data;
};


class ConnectChannel:public Channel{
public:
    ConnectChannel(int fd,EventLoop & data ,HttpServer & http_server)
        :Channel(fd, EVENT_READ,data),server(http_server),request(std::make_shared<HttpRequest> ()){
        yolanda_msgx("connectchannel ctor");
    }

    int run_read_callback() override;
    int run_write_callback() override;

     
    Buffer input_buffer;   //接收缓冲区
    Buffer output_buffer;  //发送缓冲区

    int send_data(void *data, int size);
    int send_buffer(Buffer & buffer);
    void shutdownfd();
    HttpServer & server;
    std::shared_ptr<HttpRequest> request; 
private:
    int process_status_line(char *start, char *end);
    int parse_http_request();
    int messageCallBack();
    int writeCompletedCallBack();
    int connectionClosedCallBack();
};

#endif