#include "reactor.h"
#include "../web_search/web_search_engine.h"
#include "../recommendation/keyword_recommender.h"
#include "../data_reader/data_reader.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <iostream>
#include <nlohmann/json.hpp>

using std::cout;
using std::endl;
using std::cerr;
using std::string;
using std::vector;

// ============ BusinessHandler 实现 ============
BusinessHandler::BusinessHandler(DataReaderManager* dataManager) {
    cout << "初始化业务处理器..." << endl;
    
    // 初始化搜索引擎和关键字推荐器
            searchEngine_ = std::make_unique<WebSearchEngine>(dataManager);
    keywordRecommender_ = std::make_unique<KeywordRecommender>(dataManager);
    
    cout << "业务处理器初始化完成" << endl;
}

TLVMessage BusinessHandler::processMessage(const TLVMessage& request) {
    try {
        switch (request.getType()) {
            case MessageType::KEYWORD_RECOMMEND_REQUEST:
                return handleKeywordRecommendRequest(request);
                
            case MessageType::SEARCH_REQUEST:
                return handleSearchRequest(request);
                
            default:
                return TLVMessageBuilder::buildErrorResponse(
                    "不支持的消息类型", static_cast<int>(request.getType()));
        }
    } catch (const std::exception& e) {
        return TLVMessageBuilder::buildErrorResponse(
            string("处理请求时发生错误: ") + e.what(), -1);
    }
}

TLVMessage BusinessHandler::handleKeywordRecommendRequest(const TLVMessage& request) {
    // 解析JSON请求
    nlohmann::json requestJson = nlohmann::json::parse(request.getJsonData());
    
    string query = requestJson["query"];
    int k = requestJson.value("k", 10);  // 默认返回10个推荐
    
    cout << "处理关键字推荐请求: " << query << ", k=" << k << endl;
    
    // 调用关键字推荐器
    string responseJson = keywordRecommender_->recommendToJson(query, k).dump();
    
    return TLVMessageBuilder::buildKeywordRecommendResponse(responseJson);
}

TLVMessage BusinessHandler::handleSearchRequest(const TLVMessage& request) {
    // 解析JSON请求
    nlohmann::json requestJson = nlohmann::json::parse(request.getJsonData());
    
    string query = requestJson["query"];
    int topN = requestJson.value("topN", 5);  // 默认返回5个结果
    
    cout << "处理搜索请求: " << query << ", topN=" << topN << endl;
    
    // 调用搜索引擎
    string responseJson = searchEngine_->searchToJson(query, topN).dump();
    
    return TLVMessageBuilder::buildSearchResponse(responseJson);
}

// ============ Reactor 实现 ============
Reactor::Reactor(int port, DataReaderManager* dataManager, size_t threadPoolSize)
    : serverFd_(-1)
    , epollFd_(-1)
    , port_(port)
    , running_(false) {
    
    cout << "初始化Reactor服务器，端口: " << port_ 
         << ", 线程池大小: " << threadPoolSize << endl;
    
    // 创建线程池和业务处理器
    threadPool_ = std::make_unique<ThreadPool>(threadPoolSize);
    businessHandler_ = std::make_unique<BusinessHandler>(dataManager);
    
    // 创建epoll实例
    epollFd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epollFd_ == -1) {
        throw std::runtime_error("创建epoll失败: " + string(strerror(errno)));
    }
    
    // 创建服务器socket
    createServerSocket();
    
    cout << "Reactor服务器初始化完成" << endl;
}

Reactor::~Reactor() {
    stop();
    
    if (serverFd_ != -1) {
        close(serverFd_);
    }
    
    if (epollFd_ != -1) {
        close(epollFd_);
    }
}

void Reactor::start() {
    if (running_.load()) {
        cout << "服务器已经在运行中" << endl;
        return;
    }
    
    running_.store(true);
    cout << "启动Reactor服务器，监听端口: " << port_ << endl;
    
    const int MAX_EVENTS = 1024;
    struct epoll_event events[MAX_EVENTS];
    
    while (running_.load()) {
        // 等待事件
        int nfds = epoll_wait(epollFd_, events, MAX_EVENTS, 1000); // 1秒超时
        
        if (nfds == -1) {
            if (errno == EINTR) {
                continue; // 被信号中断，继续循环
            }
            cerr << "epoll_wait失败: " << strerror(errno) << endl;
            break;
        }
        
        // 处理就绪的事件
        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;
            uint32_t event = events[i].events;
            
            if (fd == serverFd_) {
                // 新连接事件
                if (event & EPOLLIN) {
                    acceptNewConnection();
                }
            } else {
                // 客户端连接事件
                auto conn = getConnection(fd);
                if (!conn) {
                    continue;
                }
                
                if (event & (EPOLLHUP | EPOLLERR)) {
                    // 连接错误或关闭
                    handleError(conn);
                } else if (event & EPOLLIN) {
                    // 可读事件
                    handleRead(conn);
                } else if (event & EPOLLOUT) {
                    // 可写事件
                    handleWrite(conn);
                }
            }
        }
    }
    
    cout << "Reactor事件循环结束" << endl;
}

void Reactor::stop() {
    if (!running_.load()) {
        return;
    }
    
    cout << "停止Reactor服务器..." << endl;
    running_.store(false);
    
    // 关闭所有连接
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    for (auto& [fd, conn] : connections_) {
        close(fd);
    }
    connections_.clear();
    
    cout << "Reactor服务器已停止" << endl;
}

void Reactor::handleRead(std::shared_ptr<Connection> conn) {
    if (conn->state != ConnectionState::READING) {
        return;
    }
    
    const size_t BUFFER_SIZE = 4096;
    uint8_t buffer[BUFFER_SIZE];
    
    ssize_t bytesRead = recv(conn->fd, buffer, BUFFER_SIZE, 0);
    
    if (bytesRead > 0) {
        // 将数据添加到读缓冲区
        conn->readBuffer.insert(conn->readBuffer.end(), buffer, buffer + bytesRead);
        conn->bytesRead += bytesRead;
        
        // 尝试处理完整的消息
        processReadData(conn);
        
    } else if (bytesRead == 0) {
        // 连接关闭
        handleClose(conn);
    } else {
        // 读取错误
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            handleError(conn);
        }
    }
}

void Reactor::handleWrite(std::shared_ptr<Connection> conn) {
    if (conn->state != ConnectionState::WRITING || conn->writeBuffer.empty()) {
        return;
    }
    
    size_t remaining = conn->writeBuffer.size() - conn->bytesWritten;
    ssize_t bytesWritten = send(conn->fd, 
                               conn->writeBuffer.data() + conn->bytesWritten,
                               remaining, 0);
    
    if (bytesWritten > 0) {
        conn->bytesWritten += bytesWritten;
        
        if (conn->bytesWritten >= conn->writeBuffer.size()) {
            // 写入完成，切换到读取状态
            conn->state = ConnectionState::READING;
            conn->writeBuffer.clear();
            conn->bytesWritten = 0;
            
            // 修改epoll监听事件为读取
            modifyEpollEvents(conn->fd, EPOLLIN | EPOLLET);
        }
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            handleError(conn);
        }
    }
}

void Reactor::handleError(std::shared_ptr<Connection> conn) {
    cout << "连接 " << conn->fd << " 发生错误，关闭连接" << endl;
    handleClose(conn);
}

void Reactor::handleClose(std::shared_ptr<Connection> conn) {
    cout << "关闭连接 " << conn->fd << endl;
    
    conn->state = ConnectionState::CLOSED;
    removeFromEpoll(conn->fd);
    close(conn->fd);
    removeConnection(conn->fd);
}

void Reactor::createServerSocket() {
    // 创建socket
    serverFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd_ == -1) {
        throw std::runtime_error("创建socket失败: " + string(strerror(errno)));
    }
    
    // 设置SO_REUSEADDR选项
    int opt = 1;
    if (setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        close(serverFd_);
        throw std::runtime_error("设置socket选项失败: " + string(strerror(errno)));
    }
    
    // 设置非阻塞
    makeNonBlocking(serverFd_);
    
    // 绑定地址
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);
    
    if (bind(serverFd_, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        close(serverFd_);
        throw std::runtime_error("绑定地址失败: " + string(strerror(errno)));
    }
    
    // 开始监听
    if (listen(serverFd_, SOMAXCONN) == -1) {
        close(serverFd_);
        throw std::runtime_error("监听失败: " + string(strerror(errno)));
    }
    
    // 添加到epoll
    addToEpoll(serverFd_, EPOLLIN | EPOLLET);
    
    cout << "服务器socket创建成功，监听端口: " << port_ << endl;
}

void Reactor::makeNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        throw std::runtime_error("获取文件描述符标志失败");
    }
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error("设置非阻塞模式失败");
    }
}

void Reactor::addToEpoll(int fd, uint32_t events) {
    struct epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    
    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
        throw std::runtime_error("添加到epoll失败: " + string(strerror(errno)));
    }
}

void Reactor::modifyEpollEvents(int fd, uint32_t events) {
    struct epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    
    if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev) == -1) {
        cerr << "修改epoll事件失败: " << strerror(errno) << endl;
    }
}

void Reactor::removeFromEpoll(int fd) {
    if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        cerr << "从epoll删除失败: " << strerror(errno) << endl;
    }
}

void Reactor::acceptNewConnection() {
    struct sockaddr_in clientAddr{};
    socklen_t clientLen = sizeof(clientAddr);
    
    while (true) {
        int clientFd = accept(serverFd_, (struct sockaddr*)&clientAddr, &clientLen);
        
        if (clientFd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 没有更多连接
                break;
            }
            cerr << "接受连接失败: " << strerror(errno) << endl;
            break;
        }
        
        cout << "接受新连接: " << inet_ntoa(clientAddr.sin_addr) 
             << ":" << ntohs(clientAddr.sin_port) 
             << ", fd=" << clientFd << endl;
        
        try {
            // 设置非阻塞
            makeNonBlocking(clientFd);
            
            // 创建连接对象
            auto conn = std::make_shared<Connection>(clientFd);
            
            // 添加到连接管理
            {
                std::lock_guard<std::mutex> lock(connectionsMutex_);
                connections_[clientFd] = conn;
            }
            
            // 添加到epoll监听读事件
            addToEpoll(clientFd, EPOLLIN | EPOLLET);
            
        } catch (const std::exception& e) {
            cerr << "处理新连接失败: " << e.what() << endl;
            close(clientFd);
        }
    }
}

void Reactor::removeConnection(int fd) {
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    connections_.erase(fd);
}

std::shared_ptr<Connection> Reactor::getConnection(int fd) {
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    auto it = connections_.find(fd);
    return (it != connections_.end()) ? it->second : nullptr;
}

void Reactor::processReadData(std::shared_ptr<Connection> conn) {
    while (TLVCodec::hasCompleteMessage(conn->readBuffer)) {
        size_t parsedBytes = 0;
        auto messages = TLVCodec::decode(conn->readBuffer, parsedBytes);
        
        // 处理解析出的消息
        for (const auto& message : messages) {
            processCompleteMessage(conn, message);
        }
        
        // 从缓冲区移除已处理的数据
        if (parsedBytes > 0) {
            conn->readBuffer.erase(conn->readBuffer.begin(), 
                                  conn->readBuffer.begin() + parsedBytes);
        }
    }
}

void Reactor::processCompleteMessage(std::shared_ptr<Connection> conn, 
                                   const TLVMessage& message) {
    // 切换到处理状态
    conn->state = ConnectionState::PROCESSING;
    
    // 提交到线程池处理
    threadPool_->enqueueTask([this, conn, message]() {
        try {
            // 处理业务逻辑
            TLVMessage response = businessHandler_->processMessage(message);
            
            // 编码响应
            auto encodedResponse = TLVCodec::encode(response);
            
            // 发送响应
            if (sendData(conn, encodedResponse)) {
                conn->state = ConnectionState::WRITING;
                modifyEpollEvents(conn->fd, EPOLLOUT | EPOLLET);
            } else {
                handleError(conn);
            }
            
        } catch (const std::exception& e) {
            cerr << "处理消息失败: " << e.what() << endl;
            
            // 发送错误响应
            auto errorResponse = TLVMessageBuilder::buildErrorResponse(
                string("服务器内部错误: ") + e.what(), 500);
            auto encodedError = TLVCodec::encode(errorResponse);
            
            if (sendData(conn, encodedError)) {
                conn->state = ConnectionState::WRITING;
                modifyEpollEvents(conn->fd, EPOLLOUT | EPOLLET);
            } else {
                handleError(conn);
            }
        }
    });
}

bool Reactor::sendData(std::shared_ptr<Connection> conn, 
                      const vector<uint8_t>& data) {
    if (conn->state == ConnectionState::CLOSED) {
        return false;
    }
    
    // 将数据添加到写缓冲区
    conn->writeBuffer.insert(conn->writeBuffer.end(), data.begin(), data.end());
    conn->bytesWritten = 0;
    
    return true;
}
