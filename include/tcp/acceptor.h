#ifndef ACCEPTOR_H
#define ACCEPTOR_H

class Acceptor {
public:
    Acceptor(int port);
private:
    int listen_port;
    int listen_fd;
};

#endif