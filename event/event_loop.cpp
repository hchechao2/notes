#include "event/event_loop.h"
#include "event/channel.h"
#include "event/event_dispatcher.h"

EventLoop::~EventLoop(){}

EventLoop::EventLoop(std::string name,IOmultiplexing t)
    :thread_name(name),quit(0),owner_thread_id(pthread_self()) {
    
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    //实例化event_dispatcher成员对象
    if (t==POLL){
        yolanda_msgx("set poll as dispatcher, %s", thread_name.c_str());

        event_dispatcher = std::make_unique<PollDispatcher> ();
    }
    else {
        yolanda_msgx("set epoll as dispatcher, %s", thread_name.c_str());
        event_dispatcher = std::make_unique<EpollDispatcher> ();
    }
    // event_dispatcher_data = event_dispatcher->init(eventLoop);

    //add the socketfd to event
    //创建并监听本地套接字，以便从dispatch 的阻塞中唤醒，唤醒后调用handleWakeup函数

    if (name != "main thread"){
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, socketPair) < 0) {
            LOG_ERR("socketpair set fialed");
        }
        std::shared_ptr<Channel> c=std::make_shared<WakeChannel> (socketPair[1], EVENT_READ, *this);
        add_channel_event(c);
    }
}


int do_channel_event(std::shared_ptr<Channel> & c, int type) {
    auto & event_loop=c->event_loop;
    pthread_mutex_lock(&(event_loop.mutex));
    // assert(eventLoop->is_handle_pending == 0);
    //往该线程的channel链表里增加新的channel
    // channel_buffer_nolock(fd, channel1, type);

    c->set_type(type);
    event_loop.pending.push_front(c);

    //release the lock
    pthread_mutex_unlock(&event_loop.mutex);
    //主线程中操作就唤醒该eventloop线程

    //子线程会通过channel对象的write回调间接执行这个函数，调用handle_pending_channel处理新增加的 channel event 事件列表
    if (!isInSameThread(event_loop)) {
        event_loop.wakeup();
    } else {
        event_loop.handle_pending_channel();
    }

    return 0;
}

void EventLoop::wakeup() {
    char one = 'a';
    //在主线程中对子线程eventloop发送消息进行通信
    ssize_t n = write(socketPair[0], &one, sizeof one);
    yolanda_msgx("wakeup, %s",thread_name.c_str());
    if (n != sizeof one) {
        LOG_ERR("wakeup event loop thread failed");
    }
}

int EventLoop::add_channel_event(std::shared_ptr<Channel> & c) {
    return do_channel_event(c, 1);
}

int EventLoop::remove_channel_event(std::shared_ptr<Channel> & c) {
    return do_channel_event(c, 2);
}

int EventLoop::update_channel_event(std::shared_ptr<Channel> & c) {
    return do_channel_event(c, 3);
}


//遍历处理当前 event loop 里 pending 的 channel event 
int EventLoop::handle_pending_channel() {
    //get the lock
    pthread_mutex_lock(&mutex);
    // is_handle_pending = 1;
    auto channel_iterator=pending.begin();
    //channel链表代表每次被唤醒待处理的事件合集，依次处理
    while (channel_iterator != pending.end()) {
        // struct channel *channel = channelElement->channel;
        if ((*channel_iterator)->type == 1) {
            handle_pending_add(*channel_iterator);
        } else if ((*channel_iterator)->type == 2) {
            handle_pending_remove(*channel_iterator);
        } else if ((*channel_iterator)->type == 3) {
            handle_pending_update(*channel_iterator);
        }
        ++channel_iterator;
    }

    pending.clear();
    // pending_head = pending_tail = NULL;
    // is_handle_pending = 0;

    //release the lock
    pthread_mutex_unlock(&mutex);
    return 0;
}


// in the i/o thread
int EventLoop::handle_pending_add(std::shared_ptr<Channel> & c) {
    int fd=c->fd;
    yolanda_msgx("add channel fd == %d, %s", fd, thread_name.c_str());
    
    if (fd < 0)
        return 0;

    //第一次创建，增加
    if (channel_map.find(fd)==channel_map.end()){
        channel_map[fd] = c;
        event_dispatcher->add(channel_map[fd]);
        return 1;
    }

    return 0;
}

int EventLoop::handle_pending_remove(std::shared_ptr<Channel> & c) {
    int fd=c->fd;
    yolanda_msgx("remove channel fd == %d, %s", fd, thread_name.c_str());

    if (fd < 0)
        return 0;

    if (channel_map.find(fd)==channel_map.end())
        return -1;

    auto & channel2 = channel_map[fd];

    int retval = 0;
    if (event_dispatcher->del(channel2) == -1) {
        retval = -1;
    } else {
        retval = 1;
    }

    channel_map.erase(fd);
    return retval;
}

//channel 对象主要和 event_dispatcher 交互
int EventLoop::handle_pending_update(std::shared_ptr<Channel> & c) {
    int fd=c->fd;
    yolanda_msgx("update channel fd == %d, %s", fd, thread_name.c_str());
    
    if (fd < 0)
        return 0;

    if (channel_map.find(fd)==channel_map.end())
        return -1;

    event_dispatcher->update(channel_map[fd]);

    return 0;
}

int EventLoop::channel_event_activate(int fd, int revents) {
    //channel_map 保存了描述符到 channel 的映射，
    //event_dispatcher 在获得活动事件列表之后，需要通过文件描述符找到对应的 channel，
    //从而回调 channel 上的事件处理函数 event_read_callback 和 event_write_callback
    yolanda_msgx("activate channel fd == %d, revents=%d, %s", fd, revents, thread_name.c_str());
    if (fd < 0)
        return 0;

    if (channel_map.find(fd)==channel_map.end())
        return -1;

    auto & channel = channel_map[fd];
    assert(fd == channel->fd);

    if (revents & EVENT_READ) {
        channel->run_read_callback();
        // if (channel->eventReadCallback) channel->eventReadCallback(channel->data);
    }
    if (revents & EVENT_WRITE) {
        channel->run_write_callback();
        // if (channel->eventWriteCallback) channel->eventWriteCallback(channel->data);
    }

    return 0;
}


//  调用dispatcher来进行事件分发,分发完回调事件处理函数
int EventLoop::run() {
    
    if (owner_thread_id != pthread_self()) {
        exit(1);
    }

    yolanda_msgx("event loop run, %s", thread_name.c_str());
    struct timeval timeval;
    timeval.tv_sec = 1;

    //主循环
    while (!quit) {
        //block here to wait I/O event, and get active channels
        event_dispatcher->dispatch(*this, &timeval);
        //dispatch内通过执行channel_event_activate来调用的read|write回调函数对事件进行处理

        //这样像onMessage等应用程序代码也会在 event loop 线程执行，如果这里的业务逻辑过于复杂，
        //就会导致 event_loop_handle_pending_channel 执行的时间偏后，从而影响 I/O 的检测。
        
        //如果是子线程被唤醒，这个部分也会立即执行
        //实际上是消化pending链表，这个链表是对（需要监听事件集合）的操作的集合
        handle_pending_channel();
    }
    yolanda_msgx("event loop end, %s", thread_name.c_str());
    return 0;
}