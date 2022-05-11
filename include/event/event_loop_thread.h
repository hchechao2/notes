#ifndef EVENT_LOOP_THREAD_H
#define EVENT_LOOP_THREAD_H

#include <pthread.h>
#include <string>
class EventLoopThread {
public:
    EventLoopThread(int i);
    EventLoop & start();
    
private:
    void run(void *arg) ;
    EventLoop *eventLoop;
    pthread_t thread_tid;        /* thread ID */
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    string thread_name;
    long thread_count;    /* # connections handled */
};

#endif