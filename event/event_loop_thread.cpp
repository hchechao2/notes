#include "event_loop_thread.h"
//event_loop_thread 是 reactor 的线程实现，连接套接字的 read/write 事件检测都是在这个线程里完成的。
EventLoopThread::EventLoopThread(int i)
    :eventLoop(nullptr),thread_count(0),thread_tid(0) 
{
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    thread_name=new char[16];
    //去掉了\0
    sprintf(thread_name, "Thread-%d", i + 1);
}

void EventLoopThread::run(void *arg) ;{
    struct event_loop_thread *eventLoopThread = (struct event_loop_thread *) arg;

    pthread_mutex_lock(&eventLoopThread->mutex);

    // 初始化化event loop，之后通知主线程
    eventLoopThread->eventLoop = event_loop_init_with_name(eventLoopThread->thread_name);
    yolanda_msgx("event loop thread init and signal, %s", eventLoopThread->thread_name);
    pthread_cond_signal(&eventLoopThread->cond);

    pthread_mutex_unlock(&eventLoopThread->mutex);

    //子线程event loop run
    event_loop_run(eventLoopThread->eventLoop);
}

EventLoop & EventLoopThread::start() {
    pthread_create(&thread_tid, NULL, &run, this);

    assert(pthread_mutex_lock(&eventLoopThread->mutex) == 0);

    while (eventLoopThread->eventLoop == NULL) {
        assert(pthread_cond_wait(&eventLoopThread->cond, &eventLoopThread->mutex) == 0);
    }
    assert(pthread_mutex_unlock(&eventLoopThread->mutex) == 0);

    yolanda_msgx("event loop thread started, %s", eventLoopThread->thread_name);
    return eventLoopThread->eventLoop;
}