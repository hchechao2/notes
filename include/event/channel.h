#ifndef CHANNEL_H
#define CHANNEL_H

#include "utils/common.h"
#include "event/event_loop.h"
#include "tcp/buffer.h"

#define EVENT_TIMEOUT    0x01
/** Wait for a socket or FD to become readable */
#define EVENT_READ        0x02
/** Wait for a socket or FD to become writeable */
#define EVENT_WRITE    0x04
/** Wait for a POSIX signal to be raised*/
#define EVENT_SIGNAL    0x08


typedef int (*event_read_callback)(void *data);

typedef int (*event_write_callback)(void *data);

class Channel {
public:
    Channel(int fd, int events,void *data,event_read_callback eventReadCallback, event_write_callback eventWriteCallback);

    int write_event_is_enabled();

    int write_event_enable();

    int write_event_disable();
    
private:
    int fd;
    int events;   //表示event类型

    event_read_callback eventReadCallback;
    event_write_callback eventWriteCallback;
    void *data; // 可能是event_loop，也可能是tcp_server或者tcp_connection 分别代表唤醒套接字，监听套接字，连接套接字
};

#endif