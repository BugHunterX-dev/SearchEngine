#include "search_engine_server.h"
#include "../network/tcp_server.h"
#include "../network/thread_pool.h"
#include <ctime>
#include <iostream>
#include <vector>

using std::cout;
using std::endl;
using std::vector;

// 静态成员定义
unique_ptr<WebSearchEngine> SearchEngineTask::webSearchEngine_ = nullptr;
unique_ptr<KeywordRecommender> SearchEngineTask::keywordRecommender_ = nullptr;
bool SearchEngineTask::initialized_ = false;

// ============ SearchEngineTask 实现 ============

SearchEngineTask::SearchEngineTask(const string& message,
                                   const TcpConnectionPtr& connection,
                                   DataReaderManager* dataManager)
    : message_(message)
    , connection_(connection)
    , dataManager_(dataManager) {
    initializeBusinessComponents();
}

void SearchEngineTask::initializeBusinessComponents() {
    if (!initialized_ && dataManager_) {
        webSearchEngine_ = std::make_unique<WebSearchEngine>(dataManager_);
        keywordRecommender_ =
            std::make_unique<KeywordRecommender>(dataManager_);
        initialized_ = true;
        cout << "业务组件初始化完成" << endl;
    }
}

void SearchEngineTask::process() {
    try {
        // 解析TLV消息
        vector<uint8_t> buffer(message_.begin(), message_.end());
        size_t parsedBytes = 0;
        auto messages = TLVCodec::decode(buffer, parsedBytes);

        if (messages.empty()) {
            sendErrorResponse("无效的消息格式");
            return;
        }

        const auto& request = messages[0];

        // 根据消息类型分发处理
        switch (request.getType()) {
        case MessageType::KEYWORD_RECOMMEND_REQUEST:
            handleKeywordRecommendRequest(request);
            break;
        case MessageType::SEARCH_REQUEST:
            handleSearchRequest(request);
            break;
        default:
            sendErrorResponse("不支持的消息类型");
            break;
        }

    } catch (const std::exception& e) {
        sendErrorResponse(string("处理请求时发生错误: ") + e.what());
        cout << "处理请求异常: " << e.what() << endl;
    }
}

void SearchEngineTask::handleKeywordRecommendRequest(
    const TLVMessage& request) {
    try {
        auto requestJson = nlohmann::json::parse(request.getJsonData());
        string query = requestJson["query"];
        int k = requestJson.value("k", 10);

        cout << "处理关键字推荐请求: " << query << ", k=" << k << endl;

        // 调用推荐系统
        auto responseJson = keywordRecommender_->recommendToJson(query, k);

        // 构建响应
        auto response = TLVMessageBuilder::buildKeywordRecommendResponse(
            responseJson.dump());
        sendResponse(response);

    } catch (const std::exception& e) {
        sendErrorResponse(string("关键字推荐失败: ") + e.what());
    }
}

void SearchEngineTask::handleSearchRequest(const TLVMessage& request) {
    try {
        auto requestJson = nlohmann::json::parse(request.getJsonData());
        string query = requestJson["query"];
        int topN = requestJson.value("topN", 5);

        cout << "处理搜索请求: " << query << ", topN=" << topN << endl;

        // 调用搜索引擎
        auto responseJson = webSearchEngine_->searchToJson(query, topN);

        // 构建响应
        auto response =
            TLVMessageBuilder::buildSearchResponse(responseJson.dump());
        sendResponse(response);

    } catch (const std::exception& e) {
        sendErrorResponse(string("网页搜索失败: ") + e.what());
    }
}

void SearchEngineTask::sendResponse(const TLVMessage& response) {
    try {
        auto encodedResponse = TLVCodec::encode(response);
        string responseStr(encodedResponse.begin(), encodedResponse.end());
        connection_->send(responseStr);
    } catch (const std::exception& e) {
        cout << "发送响应失败: " << e.what() << endl;
    }
}

void SearchEngineTask::sendErrorResponse(const string& errorMessage,
                                         int errorCode) {
    try {
        auto errorResponse =
            TLVMessageBuilder::buildErrorResponse(errorMessage, errorCode);
        sendResponse(errorResponse);
    } catch (const std::exception& e) {
        cout << "发送错误响应失败: " << e.what() << endl;
    }
}

// ============ SearchEngineServer 实现 ============

SearchEngineServer::SearchEngineServer(size_t threadNum, size_t queueSize,
                                       const string& ip, unsigned short port)
    : threadPool_(threadNum, queueSize)
    , tcpServer_(ip, port)
    , dataDirectory_("./data")
    , running_(false) {

    // 设置TCP服务器回调
    tcpServer_.setAllCallback(
        [this](const TcpConnectionPtr& conn) { onNewConnection(conn); },
        [this](const TcpConnectionPtr& conn) { onMessage(conn); },
        [this](const TcpConnectionPtr& conn) { onClose(conn); });
}

SearchEngineServer::~SearchEngineServer() {
    stop();
}

void SearchEngineServer::setDataDirectory(const string& dataDir) {
    dataDirectory_ = dataDir;
}

void SearchEngineServer::start() {
    if (running_) {
        cout << "服务器已在运行中" << endl;
        return;
    }

    cout << "启动搜索引擎服务器..." << endl;

    // 初始化数据管理器
    try {
        dataManager_ = std::make_unique<DataReaderManager>();
        dataManager_->initialize(dataDirectory_);
        cout << "数据文件加载完成" << endl;
    } catch (const std::exception& e) {
        cout << "数据文件加载失败: " << e.what() << endl;
        return;
    }

    // 启动线程池
    threadPool_.start();
    cout << "线程池启动完成" << endl;

    // 启动TCP服务器
    running_ = true;
    tcpServer_.start();
    cout << "搜索引擎服务器启动成功" << endl;
}

void SearchEngineServer::stop() {
    if (!running_) {
        return;
    }

    cout << "正在停止搜索引擎服务器..." << endl;

    running_ = false;
    tcpServer_.stop();
    threadPool_.stop();

    cout << "搜索引擎服务器已停止" << endl;
}

void SearchEngineServer::onNewConnection(const TcpConnectionPtr& connection) {
    cout << "新客户端连接: " << connection->toString() << endl;
}

void SearchEngineServer::onMessage(const TcpConnectionPtr& connection) {
    // 接收消息
    string message = connection->receive();
    if (message.empty()) {
        return;
    }

    cout << "收到消息，长度: " << message.length() << " 字节" << endl;

    // 创建任务并提交到线程池
    auto task = std::make_shared<SearchEngineTask>(message, connection,
                                                   dataManager_.get());
    threadPool_.addTask([task]() { task->process(); });
}

void SearchEngineServer::onClose(const TcpConnectionPtr& connection) {
    cout << "客户端断开连接: " << connection->toString() << endl;
}