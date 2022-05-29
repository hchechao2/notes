#include "util/utils.h"
#include "event/event_loop.h"

void assertInSameThread(EventLoop & eventLoop) {
    if (eventLoop.owner_thread_id != pthread_self()) {
        LOG_ERR("not in the same thread");
        exit(-1);
    }
}

//1： same thread: 0： not the same thread
int isInSameThread(EventLoop & eventLoop){
    return eventLoop.owner_thread_id == pthread_self();
}