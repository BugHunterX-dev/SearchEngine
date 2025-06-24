#ifndef __ACCEPTOR_H__
#define __ACCEPTOR_H__

#include "socket.h"
#include "inet_address.h"
#include <string>

using std::string;

class Acceptor
{
public:
    Acceptor(const string &ip, unsigned short port);
    ~Acceptor();
    void ready();
private:
    void setReuseAddr();
    void setReusePort();
    void bind();
    void listen();
public:
    int accept();
    int fd() const;

private:
    Socket sock_;
    InetAddress addr_;
};

#endif
