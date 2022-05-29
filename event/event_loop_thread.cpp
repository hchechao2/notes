#include "event/event_loop_thread.h"

//event_loop_thread 是 从reactor 的线程实现，连接套接字的 read/write 事件检测都是在这个线程里完成的。
EventLoopThread::EventLoopThread(int i)
    :thread_tid(0) ,thread_name("Thread-"+ std::to_string(i+1)){
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
}


void* run(void * arg) {
    EventLoopThread * eventLoopThread = static_cast<EventLoopThread *>(arg);
    pthread_mutex_lock(&(eventLoopThread->mutex));
    // 初始化子线程event loop，之后通知主线程
    eventLoopThread->event_loop = std::make_shared<EventLoop> (eventLoopThread->thread_name);
    yolanda_msgx("event loop thread init and signal, %s", eventLoopThread->thread_name.c_str());

    pthread_cond_signal(&(eventLoopThread->cond));

    pthread_mutex_unlock(&(eventLoopThread->mutex));

    //子线程event loop run
    eventLoopThread->event_loop->run();
    return 0;

}

void EventLoopThread::start() {
    //主线程开始创建子线程
    pthread_create(&thread_tid, NULL, run, this);

    //使用mutex和conditional variable来实现线程间通信
    //使得本函数在子reactor中eventloop初始化完成并且进入run循环后再返回
    assert(pthread_mutex_lock(&mutex) == 0);

    //主线程wait会立即进入睡眠，并释放mutex
    //被signal唤醒后再次持有锁
    while (event_loop == nullptr) {
        assert(pthread_cond_wait(&cond, &mutex) == 0);
    }
    assert(pthread_mutex_unlock(&mutex) == 0);

    yolanda_msgx("event loop thread started, %s", thread_name.c_str());
    
}