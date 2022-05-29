#ifndef EVENT_DISPATCHER_H
#define EVENT_DISPATCHER_H

#include <string>
#include <memory>
#include "util/utils.h"
#include "util/c_head.h"
#include "event/channel.h"
class EventLoop;
// class Channel;

class EventDispatcher {
public:
    std::string name;
    EventDispatcher(const char * str):name(str){}
    
    /** 通知dispatcher新增一个channel事件*/
    virtual int add(std::shared_ptr<Channel> & channel) =0 ;

    /** 通知dispatcher删除一个channel事件*/
    virtual int del(std::shared_ptr<Channel> & channel) =0 ;

    /** 通知dispatcher更新channel对应的事件*/
    virtual int update(std::shared_ptr<Channel> & channel)=0;

    /** 实现事件分发，然后调用event_loop的event_activate方法执行callback*/
    virtual int dispatch(EventLoop & eventLoop, struct timeval *)=0;

    /** 清除数据 */
    // virtual int clear(struct event_loop * eventLoop);

    virtual ~EventDispatcher(){}
};

#define INIT_POLL_SIZE 1024

class PollDispatcherData {
public:
    int event_count;
    int nfds;
    int realloc_copy;
    struct pollfd *event_set;
    // struct pollfd *event_set_copy;
    PollDispatcherData():event_count(0),nfds(0),realloc_copy(0){
        event_set = static_cast<pollfd *> (malloc(sizeof(struct pollfd) * INIT_POLL_SIZE));
        for (int i = 0; i < INIT_POLL_SIZE; i++)
            event_set[i].fd = -1;
    }

    ~PollDispatcherData(){
        free(event_set);
        event_set=nullptr;
    }

};

class PollDispatcher:public EventDispatcher{
public:

    PollDispatcher():EventDispatcher("POLL"),poll_dispatcher_data(){};
    int add(std::shared_ptr<Channel> & channel) override;

    int del(std::shared_ptr<Channel> & channel) override;

    int update(std::shared_ptr<Channel> & channel) override;

    int dispatch(EventLoop & eventLoop, struct timeval *) override;

private:
    PollDispatcherData poll_dispatcher_data;
};


#define MAXEVENTS 128

class EpollDispatcherData{
public:
    int event_count;
    int nfds;
    int realloc_copy;
    int efd;
    struct epoll_event *events;
    EpollDispatcherData():event_count(0),nfds(0),realloc_copy(0){
        efd = epoll_create1(0);
        if (efd == -1) {
            error(1, errno, (char *)"epoll create failed ");
        }
        events = static_cast< epoll_event * > (calloc(MAXEVENTS, sizeof(struct epoll_event)) );
    }

    ~EpollDispatcherData(){
        free(events);
        close(efd);
        events=nullptr;
    }
    
};

class EpollDispatcher:public EventDispatcher{
public:

    EpollDispatcher():EventDispatcher("EPOLL"),epoll_dispatcher_data(){};
    int add(std::shared_ptr<Channel> & channel) override;

    int del(std::shared_ptr<Channel> & channel) override;

    int update(std::shared_ptr<Channel> & channel) override;

    int dispatch(EventLoop & eventLoop, struct timeval *) override;

private:
    EpollDispatcherData epoll_dispatcher_data;
};


#endif