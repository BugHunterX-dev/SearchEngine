#ifndef __TCPCONNECTION_H__
#define __TCPCONNECTION_H__

#include "inet_address.h"
#include "socket.h"
#include "socket_io.h"

#include <functional>
#include <memory>

using std::function;
using std::shared_ptr;

class EventLoop; // 前向声明

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
    using TcpConnectionPtr = shared_ptr<TcpConnection>;
    using TcpConnectionCallback = function<void(const TcpConnectionPtr&)>;

public:
    explicit TcpConnection(int fd, EventLoop* loop);
    ~TcpConnection();
    void send(const string& msg);
    void sendInLoop(const string& msg);
    string receive();
    // 判断连接是否断开（被动断开）
    bool isClosed() const;

    // 为了方便调试的函数
    string toString();

private:
    // 获取本端地址与对端地址
    InetAddress getLocalAddr();
    InetAddress getPeerAddr();

public:
    // 注册三个回调
    void setNewConnectionCallback(const TcpConnectionCallback& cb);
    void setMessageCallback(const TcpConnectionCallback& cb);
    void setCloseCallback(const TcpConnectionCallback& cb);

    // 执行三个回调
    void handleNewConnectionCallback();
    void handleMessageCallback();
    void handleCloseCallback();

private:
    EventLoop* loop_; // 要知道EventLoop的存在才能将数据发给EventLoop
    SocketIO sockIO_;

    // 为了调试而加入的三个数据成员
    Socket sock_;
    InetAddress localAddr_;
    InetAddress peerAddr_;

    TcpConnectionCallback onNewConnection_; // 连接建立
    TcpConnectionCallback onMessage_;       // 消息到达
    TcpConnectionCallback onClose_;         // 连接断开
};

#endif
