#include <assert.h>
#include "event_loop.h"
#include "utils/log.h"
#include "utils/utils.h"

//主要步骤 实例化该循环对象，具体的底层poll/epoll由dispatcher实现，dispatcher反过来修改eventloop对象


//ctor
EventLoop(string name)
    :thread_name(name),quit(0),owner_thread_id( pthread_self() ),is_handle_pending(0),pending_head(NULL), pending_tail(NULL) {
    
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);


//实例化event_dispatcher成员对象
#ifdef EPOLL_ENABLE
    yolanda_msgx("set epoll as dispatcher, %s", thread_name);
    event_dispatcher = &epoll_dispatcher;
#else
    yolanda_msgx("set poll as dispatcher, %s", thread_name);
    event_dispatcher = &poll_dispatcher;
#endif
    event_dispatcher_data = event_dispatcher->init(eventLoop);

    //add the socketfd to event
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, socketPair) < 0) {
        LOG_ERR("socketpair set fialed");
    }
    
    //监听与主线程的本地套接字，以便从dispatch 的阻塞中唤醒，唤醒后调用handleWakeup函数
    struct channel *channel = channel_new(eventLoop->socketPair[1], EVENT_READ, this,handleWakeup,NULL);
    add_channel_event(socketPair[1], channel);

}

//在构造函数中传给channel对象，被主线程唤醒后执行
int EventLoop::handleWakeup(void *data) {
    struct event_loop *eventLoop = (struct event_loop *) data;
    char one;
    ssize_t n = read(eventLoop->socketPair[1], &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERR("handleWakeup  failed");
    }
    yolanda_msgx("wakeup, %s", eventLoop->thread_name);
}

//遍历处理当前 event loop 里 pending 的 channel event 
int EventLoop::handle_pending_channel() {
    //get the lock
    pthread_mutex_lock(&mutex);
    is_handle_pending = 1;

    struct channel_element *channelElement = eventLoop->pending_head;
    //channel链表代表每次被唤醒待处理的事件合集，依次处理
    while (channelElement != NULL) {
        struct channel *channel = channelElement->channel;
        int fd = channel->fd;
        if (channelElement->type == 1) {
            handle_pending_add(fd, channel);
        } else if (channelElement->type == 2) {
            handle_pending_remove(fd, channel);
        } else if (channelElement->type == 3) {
            handle_pending_update(fd, channel);
        }
        channelElement = channelElement->next;
    }

    pending_head = pending_tail = NULL;
    is_handle_pending = 0;

    //release the lock
    pthread_mutex_unlock(&mutex);

    return 0;
}

//往channel链表里添加
void EventLoop::channel_buffer_nolock(int fd, struct channel *channel1, int type) {
    //外面获取过锁了，如果想避免可以使用recursive lock
    //为子eventloop注册连接套接字
    struct channel_element *channelElement = malloc(sizeof(struct channel_element));
    channelElement->channel = channel1;
    channelElement->type = type;
    channelElement->next = NULL;
    

    if (eventLoop->pending_head == NULL) {
        eventLoop->pending_head = eventLoop->pending_tail = channelElement;
    } else {
        eventLoop->pending_tail->next = channelElement;
        eventLoop->pending_tail = channelElement;
    }
}


int EventLoop::do_channel_event(int fd, struct channel *channel1, int type) {

    
    pthread_mutex_lock(&eventLoop->mutex);
    assert(eventLoop->is_handle_pending == 0);
    //往该线程的channel链表里增加新的channel
    event_loop_channel_buffer_nolock(eventLoop, fd, channel1, type);
    //release the lock
    pthread_mutex_unlock(&eventLoop->mutex);
    //主线程中操作就唤醒该从线程

    //子线程会通过channel对象的write回调间接执行这个函数
    //否则子线程自己执行handle_pending_channel处理新增加的 channel event 事件列表
    if (!isInSameThread(eventLoop)) {
        event_loop_wakeup(eventLoop);
    } else {
        event_loop_handle_pending_channel(eventLoop);
    }

    return 0;

}
int EventLoop::add_channel_event(int fd, struct channel *channel1) {
    return do_channel_event(fd, channel1, 1);
}

int EventLoop::remove_channel_event( int fd, struct channel *channel1) {
    return do_channel_event(fd, channel1, 2);
}

int EventLoop::update_channel_event(int fd, struct channel *channel1) {
    return do_channel_event(fd, channel1, 3);
}

// in the i/o thread
int EventLoop::handle_pending_add(int fd, Channel *channel) {
    yolanda_msgx("add channel fd == %d, %s", fd, thread_name);
    
    if (fd < 0)
        return 0;

    //第一次创建，增加
    if (channel_map.find(fd)==channel_map.end()){
        channel_map[fd] = channel;
        event_dispatcher->add(eventLoop, channel);
        return 1;
    }

    return 0;
}

int EventLoop::handle_pending_remove(int fd, Channel *channel1) {
    
    assert(fd == channel1->fd);

    if (fd < 0)
        return 0;

    if (channel_map.find(fd)==channel_map.end())
        return -1;

    Channel *channel2 = channel_map[fd];

    
    
    int retval = 0;
    if (event_dispatcher->del(eventLoop, channel2) == -1) {
        retval = -1;
    } else {
        retval = 1;
    }

    channel_map.erase(fd);
    return retval;
}


//channel 对象主要和 event_dispatcher 交互
int EventLoop::handle_pending_update(int fd, Channel *channel) {
    yolanda_msgx("update channel fd == %d, %s", fd, thread_name);
    
    if (fd < 0)
        return 0;

    if (channel_map.find(fd)==channel_map.end())
        return -1;

    event_dispatcher->update(eventLoop, channel);
}

int EventLoop::channel_event_activate(int fd, int revents) {
    //channel_map 保存了描述符到 channel 的映射，
    //event_dispatcher 在获得活动事件列表之后，需要通过文件描述符找到对应的 channel，
    //从而回调 channel 上的事件处理函数 event_read_callback 和 event_write_callback
    yolanda_msgx("activate channel fd == %d, revents=%d, %s", fd, revents, eventLoop->thread_name);
    if (fd < 0)
        return 0;

    if (channel_map.find(fd)==channel_map.end())
        return -1;

    Channel *channel = channel_map[fd];
    assert(fd == channel->fd);

    if (revents & (EVENT_READ)) {
        if (channel->eventReadCallback) channel->eventReadCallback(channel->data);
    }
    if (revents & (EVENT_WRITE)) {
        if (channel->eventWriteCallback) channel->eventWriteCallback(channel->data);
    }

    return 0;

}

void EventLoop::wakeup() {
    char one = 'a';
    //往这个套接字的一端写时，另外一端就可以感知到读的事件，写是在主线程中，通过操纵从eventloop对象
    ssize_t n = write(eventLoop->socketPair[0], &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERR("wakeup event loop thread failed");
    }
}

/**
 *
 * 1.参数验证
 * 2.调用dispatcher来进行事件分发,分发完回调事件处理函数
 */
int EventLoop::run() {
    
    if (owner_thread_id != pthread_self()) {
        exit(1);
    }

    yolanda_msgx("event loop run, %s", thread_name);
    struct timeval timeval;
    timeval.tv_sec = 1;

    //主循环
    while (!quit) {
        //block here to wait I/O event, and get active channels
        event_dispatcher->dispatch(eventLoop, &timeval);
        //dispatch内通过执行channel_event_activate来调用的read|write回调函数对事件进行处理

        //这样像onMessage等应用程序代码也会在 event loop 线程执行，如果这里的业务逻辑过于复杂，
        //就会导致 event_loop_handle_pending_channel 执行的时间偏后，从而影响 I/O 的检测。
        
        //如果是子线程被唤醒，这个部分也会立即执行
        //实际上是消化pending链表，这个链表是对（需要监听事件集合）的操作的集合
        handle_pending_channel();
    }

    yolanda_msgx("event loop end, %s", thread_name);
    return 0;
}