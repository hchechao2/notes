#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include <pthread.h>
#include <string>
#include <unordered_map>
#include "tcp/channel.h"
#include "event/event_dispatcher.h"
#include "utils/common.h"

extern  struct event_dispatcher poll_dispatcher;
extern  struct event_dispatcher epoll_dispatcher;

class ChannelElement {
private:
    int type; //1: add  2: delete 3:update
    Channel *channel;
    ChannelElement *next;
};

class EventLoop {
public:
    
    EventLoop(string name="main thread");
    
    int run();
    void wakeup();
    
    //通过调用do_channel_event
    int add_channel_event(int fd, struct channel *channel1);
    int remove_channel_event(int fd, struct channel *channel1);
    int update_channel_event(int fd, struct channel *channel1);

    //每次循环结束后 修改当前监听的事件列表
    int handle_pending_channel();
    
    //具体实现
    int handle_pending_add(int fd, struct channel *channel);
    int handle_pending_remove(int fd, struct channel *channel);
    int handle_pending_update(int fd, struct channel *channel);

    // dispather派发完事件之后，调用该方法通知event_loop执行对应事件的相关callback方法
    // res: EVENT_READ | EVENT_WRITE等
    //完成channel对象中eventReadcallback 和 eventWritecallback 的调用
    int channel_event_activate(int fd, int res);

private:
    int do_channel_event(int fd, struct channel *channel1, int type);
    void channel_buffer_nolock(int fd, struct channel *channel1, int type); 
    
    int quit;
    
    EventDispatcher *event_dispatcher;

    /** 对应的event_dispatcher的数据.设置为*/
    void *event_dispatcher_data;
    //ChannelMap *channel_map;
    unordered_map<int,Channel*>  channel_map;

    //socketPair 是父线程用来通知子线程有新的事件需要处理。
    //pending_head 和 pending_tail 是保留在子线程内的需要处理的新事件
    int is_handle_pending;
    ChannelElement *pending_head;
    ChannelElement *pending_tail;

    pthread_t owner_thread_id;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    //socketPair 是父线程用来通知子线程有新的事件需要处理。子reactor初始化就会监听其变化
    int socketPair[2];
    string thread_name;
};



#endif