#pragma once

#include "thread_pool.h"
#include "tlv_protocol.h"
#include <atomic>
#include <functional>
#include <memory>
#include <sys/epoll.h>
#include <unordered_map>
#include <vector>

// 前向声明
class WebSearchEngine;
class KeywordRecommender;
class DataReaderManager;

// 连接状态
enum class ConnectionState {
    READING,    // 正在读取数据
    PROCESSING, // 正在处理业务逻辑
    WRITING,    // 正在写入响应
    CLOSED      // 连接已关闭
};

// 客户端连接
struct Connection {
    int fd;                           // 文件描述符
    ConnectionState state;            // 连接状态
    std::vector<uint8_t> readBuffer;  // 读缓冲区
    std::vector<uint8_t> writeBuffer; // 写缓冲区
    size_t bytesRead;                 // 已读取字节数
    size_t bytesWritten;              // 已写入字节数

    Connection(int socket_fd)
        : fd(socket_fd)
        , state(ConnectionState::READING)
        , bytesRead(0)
        , bytesWritten(0) {
        readBuffer.reserve(8192);  // 预分配8KB读缓冲区
        writeBuffer.reserve(8192); // 预分配8KB写缓冲区
    }
};

// 移除了EventHandler接口，直接在Reactor中实现具体的事件处理逻辑

// 业务逻辑处理器
class BusinessHandler {
public:
    BusinessHandler(DataReaderManager* dataManager);
    ~BusinessHandler() = default;

    // 处理TLV消息，返回响应消息
    TLVMessage processMessage(const TLVMessage& request);

private:
    std::unique_ptr<WebSearchEngine> searchEngine_;
    std::unique_ptr<KeywordRecommender> keywordRecommender_;

    // 处理关键字推荐请求
    TLVMessage handleKeywordRecommendRequest(const TLVMessage& request);

    // 处理搜索请求
    TLVMessage handleSearchRequest(const TLVMessage& request);
};

// Reactor事件循环
class Reactor {
public:
    explicit Reactor(
        int port, DataReaderManager* dataManager,
        size_t threadPoolSize = std::thread::hardware_concurrency());
    ~Reactor();

    // 启动服务器
    void start();

    // 停止服务器
    void stop();

private:
    // 事件处理方法（内部使用）
    void handleRead(std::shared_ptr<Connection> conn);
    void handleWrite(std::shared_ptr<Connection> conn);
    void handleError(std::shared_ptr<Connection> conn);
    void handleClose(std::shared_ptr<Connection> conn);
    // 连接管理
    std::unordered_map<int, std::shared_ptr<Connection>> connections_;
    std::mutex connectionsMutex_;

    // 线程池和业务处理器
    std::unique_ptr<ThreadPool> threadPool_;
    std::unique_ptr<BusinessHandler> businessHandler_;

    // 网络相关私有方法
    void createServerSocket();
    void makeNonBlocking(int fd);
    void addToEpoll(int fd, uint32_t events);
    void modifyEpollEvents(int fd, uint32_t events);
    void removeFromEpoll(int fd);

    // 连接管理
    void acceptNewConnection();
    void removeConnection(int fd);
    std::shared_ptr<Connection> getConnection(int fd);

    // 数据处理
    void processReadData(std::shared_ptr<Connection> conn);
    void processCompleteMessage(std::shared_ptr<Connection> conn,
                                const TLVMessage& message);

    // 工具方法
    bool sendData(std::shared_ptr<Connection> conn,
                  const std::vector<uint8_t>& data);

    int serverFd_;              // 服务器socket
    int epollFd_;               // epoll文件描述符
    int port_;                  // 监听端口
    std::atomic<bool> running_; // 运行状态
};