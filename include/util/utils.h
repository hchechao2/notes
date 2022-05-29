#ifndef UTILS_H
#define UTILS_H

class EventLoop;

void assertInSameThread(EventLoop & eventLoop);

//1： same thread: 0： not the same thread
int isInSameThread(EventLoop & eventLoop);
#endif