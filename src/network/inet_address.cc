#include "inet_address.h"
#include <strings.h>

InetAddress::InetAddress(const string &ip, unsigned short port)
{
    ::bzero(&addr_, sizeof(struct sockaddr_in));
    addr_.sin_family = AF_INET;
    //shift + k
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());//host->network
}

InetAddress::InetAddress(const struct sockaddr_in &addr)
: addr_(addr)
{

}

InetAddress::~InetAddress()
{

}

string InetAddress::ip() const
{
    //从网络字节序转换为本机字节序
    return string(inet_ntoa(addr_.sin_addr));
}

unsigned short InetAddress::port() const
{
    //从网络字节序转换为本机字节序
    return ntohs(addr_.sin_port);
}

const struct sockaddr_in *InetAddress::getInetAddrPtr() const
{
    return &addr_;
}
