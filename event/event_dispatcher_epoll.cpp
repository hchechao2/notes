#include "event/event_dispatcher.h"
// #include "event/channel.h"
#include "event/event_loop.h"

int EpollDispatcher::add(std::shared_ptr<Channel> & channel) {
    
    int fd = channel->fd;
    int events = 0;
    if (channel->events & EVENT_READ) {
        events = events | EPOLLIN;
    }
    if (channel->events & EVENT_WRITE) {
        events = events | EPOLLOUT;
    }

    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;
//    event.events = events | EPOLLET;
    if (epoll_ctl(epoll_dispatcher_data.efd, EPOLL_CTL_ADD, fd, &event) == -1) {
        error(1, errno, (char *)"epoll_ctl add  fd failed");
    }

    return 0;
}

int EpollDispatcher::del(std::shared_ptr<Channel> & channel) {
    int fd = channel->fd;
    int events = 0;
    if (channel->events & EVENT_READ) {
        events = events | EPOLLIN;
    }

    if (channel->events & EVENT_WRITE) {
        events = events | EPOLLOUT;
    }

    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;
//    event.events = events | EPOLLET;
    if (epoll_ctl(epoll_dispatcher_data.efd, EPOLL_CTL_DEL, fd, &event) == -1) {
        error(1, errno, (char *)"epoll_ctl delete fd failed");
    }

    return 0;
}

int EpollDispatcher::update(std::shared_ptr<Channel> & channel) {
    int fd = channel->fd;

    int events = 0;
    if (channel->events & EVENT_READ) {
        events = events | EPOLLIN;
    }

    if (channel->events & EVENT_WRITE) {
        events = events | EPOLLOUT;
    }

    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;
//    event.events = events | EPOLLET;
    if (epoll_ctl(epoll_dispatcher_data.efd, EPOLL_CTL_MOD, fd, &event) == -1) {
        error(1, errno, (char *)"epoll_ctl modify fd failed");
    }

    return 0;
}

int EpollDispatcher::dispatch(EventLoop & eventLoop, struct timeval *timeval) {
    
    int i, n;

    n = epoll_wait(epoll_dispatcher_data.efd, epoll_dispatcher_data.events, MAXEVENTS, -1);
    yolanda_msgx("epoll_wait wakeup, %s", eventLoop.thread_name.c_str());
    for (i = 0; i < n; i++) {
        if ((epoll_dispatcher_data.events[i].events & EPOLLERR) || (epoll_dispatcher_data.events[i].events & EPOLLHUP)) {
            fprintf(stderr, "epoll error\n");
            close(epoll_dispatcher_data.events[i].data.fd);
            continue;
        }

        if (epoll_dispatcher_data.events[i].events & EPOLLIN) {
            yolanda_msgx("get message channel fd==%d for read, %s", epoll_dispatcher_data.events[i].data.fd, eventLoop.thread_name.c_str());
            eventLoop.channel_event_activate(epoll_dispatcher_data.events[i].data.fd, EVENT_READ);
        }

        if (epoll_dispatcher_data.events[i].events & EPOLLOUT) {
            yolanda_msgx("get message channel fd==%d for write, %s", epoll_dispatcher_data.events[i].data.fd,eventLoop.thread_name.c_str());
            eventLoop.channel_event_activate(epoll_dispatcher_data.events[i].data.fd, EVENT_WRITE);
        }
    }

    return 0;
}
