#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "event/event_loop.h"
#include "event/event_loop_thread.h"

class ThreadPool {
public:
    ThreadPool(EventLoop *mainLoop, int threadNumber)
        : mainloop(mainLoop),thread_number(threadNumber),
        started(0),position(0),event_loop_thread(nullptr){}

    ~ThreadPool(){
        //是否需要回收？
    }
private:
    //创建thread_pool的主线程
    EventLoop *mainLoop;
    //是否已经启动
    int started;
    //线程数目
    int thread_number;
    //数组指针，指向创建的event_loop_thread数组
    EventLoopThread *event_loop_threads;

    //表示在数组里的位置，用来决定选择哪个event_loop_thread服务
    int position;

};



#endif