#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H


#include <assert.h>
#include <unordered_map>
#include <forward_list>
#include <memory>
#include "util/c_head.h"
#include "util/utils.h"

enum IOmultiplexing {POLL ,EPOLL};

class Channel;
class EventDispatcher;


class EventLoop {
public:
    EventLoop(std::string name="main thread",IOmultiplexing t=POLL);
    ~EventLoop();
    int run();
    void wakeup();
    
    // int do_channel_event(std::unique_ptr<Channel> && c, int type);
    int add_channel_event(std::shared_ptr<Channel> & c);
    int remove_channel_event(std::shared_ptr<Channel> & c);
    int update_channel_event(std::shared_ptr<Channel> & c);

    //每次循环结束后 修改当前监听的事件列表
    int handle_pending_channel();
    
    //具体实现
    int handle_pending_add(std::shared_ptr<Channel> & c);
    int handle_pending_remove(std::shared_ptr<Channel> & c);
    int handle_pending_update(std::shared_ptr<Channel> & c);

    // dispather派发完事件之后，调用该方法通知event_loop执行对应事件的相关callback方法
    // res: EVENT_READ | EVENT_WRITE等
    //完成channel对象中eventReadcallback 和 eventWritecallback 的调用
    int channel_event_activate(int fd, int res);


    
    // void channel_buffer_nolock(int fd, struct channel *channel1, int type); 
    std::string thread_name;
    int quit;

    std::unique_ptr<EventDispatcher>  event_dispatcher;
    // void* event_dispatcher_data;

    //ChannelMap *channel_map;
    std::unordered_map<int,std::shared_ptr<Channel>>  channel_map;

    //socketPair 是父线程用来通知子线程有新的事件需要处理。
    //pending_head 和 pending_tail 是保留在子线程内的需要处理的新事件
    // int is_handle_pending;
    // ChannelElement *pending_head;
    // ChannelElement *pending_tail;
    std::forward_list<std::shared_ptr<Channel>> pending;

    pthread_t owner_thread_id;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    //socketPair 是父线程用来通知子线程有新的事件需要处理。子reactor初始化就会监听其变化
    int socketPair[2];
    
};

#endif