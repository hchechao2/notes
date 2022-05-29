#ifndef EVENT_LOOP_THREAD_H
#define EVENT_LOOP_THREAD_H


#include <assert.h>
#include <string>
#include <memory>
#include "util/c_head.h"
#include "event/event_loop.h"

class EventLoopThread {
public:
    EventLoopThread(int i);
    void start();
    std::shared_ptr<EventLoop> event_loop;
    pthread_t thread_tid;        /* thread ID */
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    std::string thread_name;
};

#endif