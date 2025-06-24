#include "tcp_server.h"

TcpServer::TcpServer(const string& ip, unsigned short port)
    : acceptor_(ip, port)
    , loop_(acceptor_) {}

TcpServer::~TcpServer() {}

// 服务器的启动与停止
void TcpServer::start() {
    acceptor_.ready(); // 让服务处于监听状态
    loop_.loop();      // 也就是执行epoll_wait
}

void TcpServer::stop() {
    loop_.unloop();
}

// 设置三个回调
void TcpServer::setAllCallback(TcpConnectionCallback&& cb1,
                               TcpConnectionCallback&& cb2,
                               TcpConnectionCallback&& cb3) {
    loop_.setNewConnectionCallback(std::move(cb1));
    loop_.setMessageCallback(std::move(cb2));
    loop_.setCloseCallback(std::move(cb3));
}
