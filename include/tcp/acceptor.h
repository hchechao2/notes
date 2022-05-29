#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include "util/c_head.h"

class Acceptor {
public:
    Acceptor(int port);
    int listen_port;
    int listen_fd;
};

#endif