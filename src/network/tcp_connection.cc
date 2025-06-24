#include "tcp_connection.h"
#include "event_loop.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>
#include <arpa/inet.h>

using std::cout;
using std::endl;
using std::ostringstream;
using std::vector;

TcpConnection::TcpConnection(int fd, EventLoop* loop)
    : loop_(loop)
    , sockIO_(fd)
    , sock_(fd)
    , localAddr_(getLocalAddr())
    , peerAddr_(getPeerAddr()) {}

TcpConnection::~TcpConnection() {}

void TcpConnection::send(const string& msg) {
    sockIO_.writen(msg.c_str(), msg.size());
}

// 该函数到底该怎么实现，需要思考一下？
// 我们知道需要将数据msg传递给EventLoop
// 需要将线程池处理好之后的数据_msg通过TcpConnection的对象发送
// 给Reactor，但是Reactor本身没有发送数据的能力,其次接受数据
// 与发送数据应该是同一个连接对象,也就是调用sendInLoop函数的
// 对象，就是这个连接，这个对象不就是this,除此之外，有了连接
// 有了消息，还要有发送数据的函数
void TcpConnection::sendInLoop(const string& msg) {
    if (loop_) {
        loop_->runInLoop(bind(&TcpConnection::send, this, msg));
    }
#if 0 
    function<void()> f = bind(&TcpConnection::send, this, msg);
    if(loop_)
    {
        //void(function<void()> &&)
        loop_->runInLoop(std::move(f));
    }
#endif
}
string TcpConnection::receive() {
    // 先读取TLV头部(6字节)
    char headerBuf[6] = {0};
    int headerRead = sockIO_.readn(headerBuf, 6);
    if (headerRead != 6) {
        return "";  // 读取头部失败
    }
    
    // 解析消息长度
    uint32_t dataLength;
    memcpy(&dataLength, headerBuf + 2, 4);
    dataLength = ntohl(dataLength);  // 网络字节序转主机字节序
    
    // 读取完整消息(头部 + 数据)
    vector<char> fullMessage(6 + dataLength);
    memcpy(fullMessage.data(), headerBuf, 6);  // 复制头部
    
    if (dataLength > 0) {
        int dataRead = sockIO_.readn(fullMessage.data() + 6, dataLength);
        if (dataRead != static_cast<int>(dataLength)) {
            return "";  // 读取数据部分失败
        }
    }
    
    return string(fullMessage.begin(), fullMessage.end());
}

// 判断连接是否断开（被动断开）
bool TcpConnection::isClosed() const {
    char buff[10] = {0};
    int ret = recv(sock_.fd(), buff, sizeof(buff), MSG_PEEK);

    return 0 == ret;
}
string TcpConnection::toString() {
    ostringstream oss;
    oss << localAddr_.ip() << ":" << localAddr_.port() << "---->"
        << peerAddr_.ip() << ":" << peerAddr_.port();

    return oss.str();
}

// 获取本端的网络地址信息
InetAddress TcpConnection::getLocalAddr() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(struct sockaddr);
    // 获取本端地址的函数getsockname
    int ret = getsockname(sock_.fd(), (struct sockaddr*)&addr, &len);
    if (-1 == ret) {
        perror("getsockname");
    }

    return InetAddress(addr);
}

// 获取对端的网络地址信息
InetAddress TcpConnection::getPeerAddr() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(struct sockaddr);
    // 获取对端地址的函数getpeername
    int ret = getpeername(sock_.fd(), (struct sockaddr*)&addr, &len);
    if (-1 == ret) {
        perror("getpeername");
    }

    return InetAddress(addr);
}

// 注册三个回调
void TcpConnection::setNewConnectionCallback(const TcpConnectionCallback& cb) {
    onNewConnection_ = cb;
}

void TcpConnection::setMessageCallback(const TcpConnectionCallback& cb) {
    onMessage_ = cb;
}

void TcpConnection::setCloseCallback(const TcpConnectionCallback& cb) {
    onClose_ = cb;
}

// 执行三个回调
void TcpConnection::handleNewConnectionCallback() {
    if (onNewConnection_) {
        // 为了防止智能指针的误用，就不能使用不用的智能指针
        // 去托管同一个裸指针
        /* onNewConnection_(shared_ptr<TcpConnection>(this)); */
        onNewConnection_(shared_from_this());
    } else {
        cout << "onNewConnection_ == nullptr" << endl;
    }
}

void TcpConnection::handleMessageCallback() {
    if (onMessage_) {
        onMessage_(shared_from_this());
    } else {
        cout << "onMessage_ == nullptr" << endl;
    }
}

void TcpConnection::handleCloseCallback() {
    if (onClose_) {
        onClose_(shared_from_this());
    } else {
        cout << "onClose_ == nullptr" << endl;
    }
}
