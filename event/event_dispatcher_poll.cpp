#include "event/event_dispatcher.h"
// #include "event/channel.h"
#include "event/event_loop.h"

int PollDispatcher::add(std::shared_ptr<Channel> & channel) {

    // struct poll_dispatcher_data *pollDispatcherData = (struct poll_dispatcher_data *) eventLoop->event_dispatcher_data;

    int events = 0;
    if (channel->events & EVENT_READ) {
        events = events | POLLRDNORM;
    }

    if (channel->events & EVENT_WRITE) {
        events = events | POLLWRNORM;
    }

    //找到一个可以记录该连接套接字的位置
    int i = 0;
    for (i = 0; i < INIT_POLL_SIZE; i++) {
        if (poll_dispatcher_data.event_set[i].fd < 0) {
            poll_dispatcher_data.event_set[i].fd = channel->fd;
            poll_dispatcher_data.event_set[i].events = events;
            break;
        }
    }

    if (i >= INIT_POLL_SIZE) {
        LOG_ERR("too many clients, just abort it");
    }
    return 0;
}

int PollDispatcher::del(std::shared_ptr<Channel> & channel) {

    //找到fd对应的记录
    int i = 0;
    for (i = 0; i < INIT_POLL_SIZE; i++) {
        if (poll_dispatcher_data.event_set[i].fd == channel->fd) {
            poll_dispatcher_data.event_set[i].fd = -1;
            break;
        }
    }

    if (i >= INIT_POLL_SIZE) {
        LOG_ERR("can not find fd, poll delete error");
    }

    return 0;
}

int PollDispatcher::update(std::shared_ptr<Channel> & channel) {
    int events = 0;
    if (channel->events & EVENT_READ) {
        events = events | POLLRDNORM;
    }

    if (channel->events & EVENT_WRITE) {
        events = events | POLLWRNORM;
    }

    //找到fd对应的记录
    int i = 0;
    for (i = 0; i < INIT_POLL_SIZE; i++) {
        if (poll_dispatcher_data.event_set[i].fd == channel->fd) {
            poll_dispatcher_data.event_set[i].events = events;
            break;
        }
    }
    if (i >= INIT_POLL_SIZE) {
        LOG_ERR("can not find fd, poll updated error");
    }

    return 0;
}

int PollDispatcher::dispatch(EventLoop & eventLoop, struct timeval *timeval) {
    

    int ready_number = 0;
    int timewait = timeval->tv_sec * 1000;
    if ((ready_number = poll(poll_dispatcher_data.event_set, INIT_POLL_SIZE, timewait)) < 0) {
        error(1, errno, (char *)"poll failed ");
    }

    if (ready_number == 0) {
        return 0;
    }

    int i;
    for (i = 0; i <= INIT_POLL_SIZE; i++) {
        int socket_fd;
        struct pollfd pollfd = poll_dispatcher_data.event_set[i];
        if ((socket_fd = pollfd.fd) < 0)
            continue;

        //有事件发生
        if (pollfd.revents > 0) {
            yolanda_msgx("get message channel i==%d, fd==%d, %s", i, socket_fd, eventLoop.thread_name.c_str());

            if (pollfd.revents & POLLRDNORM) {
                eventLoop.channel_event_activate(socket_fd, EVENT_READ);
            }

            if (pollfd.revents & POLLWRNORM) {
                eventLoop.channel_event_activate(socket_fd, EVENT_WRITE);
            }

            if (--ready_number <= 0)
                break;
        }
    }

    return 0;
}

