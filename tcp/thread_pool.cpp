#include "thread_pool.h"

// thread_pool 维护了一个 sub-reactor 的线程列表，它可以提供给主 reactor 线程使用，每次当有新的连接建立时，
//可以从 thread_pool 里获取一个线程，以便用它来完成对新连接套接字的 read/write 事件注册，将 I/O 线程和主 reactor 线程分离。
void ThreadPool::start() {
    assert(!started);
    assertInSameThread(mainLoop);
    if (thread_number <= 0) {
        return;
    }
    started = 1;
    for (int i = 0; i < thread_number; ++i) {
        event_loop_threads.emplace_back(i);
    }
    for (int i = 0; i < thread_number; ++i) {
        event_loop_threads[i].start();
    }
}

//一定是main thread中选择
EventLoop & 
ThreadPool::get_loop() {
    assert(started);
    assertInSameThread(mainLoop);

    //优先选择当前主线程
    EventLoop & selected = mainLoop;

    //从线程池中按照顺序挑选出一个线程
    if (thread_number > 0) {
        selected = eventLoopThreads[position].eventLoop;
        if (++position >= thread_number) {
            position = 0;
        }
    }
    return selected;
}
