#include "socket.h"
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

Socket::Socket() {
    fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0) {
        perror("socket");
        return;
    }
}

Socket::Socket(int fd)
    : fd_(fd) {}

Socket::~Socket() {
    close(fd_);
}

int Socket::fd() const {
    return fd_;
}

void Socket::shutDownWrite() {
    // 关闭写端
    int ret = shutdown(fd_, SHUT_WR);
    if (ret) {
        perror("shutdown");
        return;
    }
}
