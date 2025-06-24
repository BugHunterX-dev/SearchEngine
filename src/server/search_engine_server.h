#pragma once

#include "../data_reader/data_reader.h"
#include "../network/tcp_connection.h"
#include "../network/tcp_server.h"
#include "../network/thread_pool.h"
#include "../network/tlv_protocol.h"
#include "../recommendation/keyword_recommender.h"
#include "../web_search/web_search_engine.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

using std::shared_ptr;
using std::string;
using std::unique_ptr;

// 前向声明
using TcpConnectionPtr = shared_ptr<TcpConnection>;

// 搜索引擎业务任务类
class SearchEngineTask {
public:
    SearchEngineTask(const string& message, const TcpConnectionPtr& connection,
                     DataReaderManager* dataManager);
    ~SearchEngineTask() = default;

    // 处理业务逻辑
    void process();

private:
    // 处理关键字推荐请求
    void handleKeywordRecommendRequest(const TLVMessage& request);

    // 处理网页搜索请求
    void handleSearchRequest(const TLVMessage& request);

    // 发送响应
    void sendResponse(const TLVMessage& response);

    // 发送错误响应
    void sendErrorResponse(const string& errorMessage, int errorCode = -1);

private:
    string message_;                 // 原始消息
    TcpConnectionPtr connection_;    // TCP连接
    DataReaderManager* dataManager_; // 数据管理器

    // 业务组件（延迟初始化）
    static unique_ptr<WebSearchEngine> webSearchEngine_;
    static unique_ptr<KeywordRecommender> keywordRecommender_;
    static bool initialized_;

    // 初始化业务组件
    void initializeBusinessComponents();
};

// 搜索引擎服务器类
class SearchEngineServer {
public:
    SearchEngineServer(size_t threadNum, size_t queueSize, const string& ip,
                       unsigned short port);
    ~SearchEngineServer();

    // 启动和停止服务器
    void start();
    void stop();

    // 设置数据目录
    void setDataDirectory(const string& dataDir);

private:
    // 回调函数
    void onNewConnection(const TcpConnectionPtr& connection);
    void onMessage(const TcpConnectionPtr& connection);
    void onClose(const TcpConnectionPtr& connection);

private:
    ThreadPool threadPool_;                     // 线程池
    TcpServer tcpServer_;                       // TCP服务器
    unique_ptr<DataReaderManager> dataManager_; // 数据管理器
    string dataDirectory_;                      // 数据目录
    bool running_;                              // 运行状态
};