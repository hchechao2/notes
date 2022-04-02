#include "channel.h"

int Channel::write_event_is_enabled() {
    return events & EVENT_WRITE;
}

int Channel::write_event_enable() {
    struct event_loop *eventLoop = (struct event_loop *) channel->data;
    events = events | EVENT_WRITE;
    event_loop_update_channel_event(eventLoop, channel->fd, channel);
}

int Channel::write_event_disable() {
    struct event_loop *eventLoop = (struct event_loop *) channel->data;
    events = events & ~EVENT_WRITE;
    event_loop_update_channel_event(eventLoop, channel->fd, channel);
}