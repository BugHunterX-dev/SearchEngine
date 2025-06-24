#ifndef __TCPSERVER_H__
#define __TCPSERVER_H__

#include "acceptor.h"
#include "event_loop.h"

class TcpServer
{
public:
    TcpServer(const string &ip, unsigned short port);
    ~TcpServer();

    //服务器的启动与停止
    void start();
    void stop();
    
    //设置三个回调
    void setAllCallback(TcpConnectionCallback &&cb1
                        , TcpConnectionCallback &&cb2
                        , TcpConnectionCallback &&cb3);

private:
    Acceptor acceptor_;
    EventLoop loop_;

};

#endif
