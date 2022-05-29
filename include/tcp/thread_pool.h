#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include "util/utils.h"
#include "event/event_loop.h"
#include "event/event_loop_thread.h"

class ThreadPool {
public:
    ThreadPool(EventLoop & mainLoop, int threadNumber)
        : mainLoop(mainLoop),started(0),thread_number(threadNumber),position(0){}

    void start();
    EventLoop & get_loop();
private:
    //创建thread_pool的主线程，即tcpserver的eventloop
    EventLoop & mainLoop;
    //是否已经启动
    int started;
    //线程数目
    int thread_number;
    //数组指针，指向创建的event_loop_thread数组
    std::vector<EventLoopThread> event_loop_threads;
    //表示在数组里的位置，用来决定选择哪个event_loop_thread服务
    int position;

};
//目的：mainloop是检测监听socket，监听到新连接时，将其相关的 I/O 事件交给 sub-reactor 子线程负责检测
// 我们知道，sub-reactor 线程是一个无限循环的 event loop 执行体，在没有已注册事件发生的情况下，
// 这个线程阻塞在 event_dispatcher 的 dispatch 上。你可以简单地认为阻塞在 poll 调用或者 epoll_wait 上，
// 这种情况下，主线程如何能把已连接套接字交给 sub-reactor 子线程呢？

// 如果我们能让 sub-reactor 线程从 event_dispatcher 的 dispatch 上返回，
// 再让 sub-reactor 线程返回之后能够把新的已连接套接字事件注册上，这件事情就算完成了。
// 那如何让 sub-reactor 线程从 event_dispatcher 的 dispatch 上返回呢？
// 答案是构建一个类似管道一样的描述字，让 event_dispatcher 注册该管道描述字，
// 当我们想让 sub-reactor 线程苏醒时，往管道上发送一个字符就可以了。在 event_loop_init 函数里，
// 调用了 socketpair 函数创建了套接字对，这个套接字对的作用就是我刚刚说过的，
// 往这个套接字的一端写时，另外一端就可以感知到读的事件。其实，这里也可以直接使用 UNIX 上的 pipe 管道，作用是一样的。

#endif