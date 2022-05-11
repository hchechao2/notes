#include "channel.h"

//ctor
Channel::Channel(int fd, int events,void *data,event_read_callback eventReadCallback, event_write_callback eventWriteCallback)
    :fd(fd), events(events),data(data){}

int Channel::write_event_is_enabled() {
    return events & EVENT_WRITE;
}

int Channel::write_event_enable() {
    EventLoop *event_loop = (EventLoop *) channel->data;
    events = events | EVENT_WRITE;
     event_loop ->update_channel_event(channel->fd, channel);
}

int Channel::write_event_disable() {
    EventLoop *event_loop = (EventLoop *) channel->data;
    events = events & ~EVENT_WRITE;
    event_loop ->update_channel_event(channel->fd, channel);
}