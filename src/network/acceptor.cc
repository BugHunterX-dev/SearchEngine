#include "acceptor.h"
#include <stdio.h>

Acceptor::Acceptor(const string& ip, unsigned short port)
    : sock_()
    , addr_(ip, port) {}

Acceptor::~Acceptor() {}

void Acceptor::ready() {
    setReuseAddr();
    setReusePort();
    bind();
    listen();
}

void Acceptor::setReuseAddr() {
    int on = 1;
    int ret = setsockopt(sock_.fd(), SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (ret) {
        perror("setsockopt");
        return;
    }
}

void Acceptor::setReusePort() {
    int on = 1;
    int ret = setsockopt(sock_.fd(), SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
    if (-1 == ret) {
        perror("setsockopt");
        return;
    }
}

void Acceptor::bind() {
    int ret = ::bind(sock_.fd(), (struct sockaddr*)addr_.getInetAddrPtr(),
                     sizeof(struct sockaddr));
    if (-1 == ret) {
        perror("bind");
        return;
    }
}

void Acceptor::listen() {
    int ret = ::listen(sock_.fd(), 128);
    if (-1 == ret) {
        perror("listen");
        return;
    }
}

int Acceptor::accept() {
    int connfd = ::accept(sock_.fd(), nullptr, nullptr);
    if (-1 == connfd) {
        perror("accept");
        return -1;
    }
    return connfd;
}

int Acceptor::fd() const {
    return sock_.fd();
}
