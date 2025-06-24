#include "src/network/tlv_protocol.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iomanip>
#include <nlohmann/json.hpp>

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::vector;

class SearchEngineClient {
public:
    SearchEngineClient(const string& serverHost, int serverPort)
        : host_(serverHost), port_(serverPort), sockfd_(-1) {}
    
    ~SearchEngineClient() {
        disconnect();
    }
    
    // 连接到服务器
    bool connect() {
        sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_ == -1) {
            std::cerr << "创建socket失败" << endl;
            return false;
        }
        
        struct sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port_);
        
        if (inet_aton(host_.c_str(), &serverAddr.sin_addr) == 0) {
            std::cerr << "无效的服务器地址: " << host_ << endl;
            close(sockfd_);
            sockfd_ = -1;
            return false;
        }
        
        if (::connect(sockfd_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
            std::cerr << "连接服务器失败 " << host_ << ":" << port_ << endl;
            close(sockfd_);
            sockfd_ = -1;
            return false;
        }
        
        cout << "成功连接到服务器 " << host_ << ":" << port_ << endl;
        return true;
    }
    
    // 断开连接
    void disconnect() {
        if (sockfd_ != -1) {
            close(sockfd_);
            sockfd_ = -1;
            cout << "已断开与服务器的连接" << endl;
        }
    }
    
    // 关键字推荐
    bool recommendKeywords(const string& query, int k = 10) {
        if (sockfd_ == -1) {
            std::cerr << "未连接到服务器" << endl;
            return false;
        }
        
        cout << "\n正在获取关键字推荐..." << endl;
        cout << "查询词: " << query << endl;
        cout << "推荐数量: " << k << endl;
        cout << "----------------------------------------" << endl;
        
        try {
            // 构建请求
            auto request = TLVMessageBuilder::buildKeywordRecommendRequest(query, k);
            auto response = sendRequest(request);
            
            if (response.getType() == MessageType::KEYWORD_RECOMMEND_RESPONSE) {
                displayKeywordRecommendations(response);
                return true;
            } else if (response.getType() == MessageType::ERROR_RESPONSE) {
                displayError(response);
                return false;
            } else {
                std::cerr << "收到未知类型的响应" << endl;
                return false;
            }
        } catch (const std::exception& e) {
            std::cerr << "关键字推荐失败: " << e.what() << endl;
            return false;
        }
    }
    
    // 网页搜索
    bool searchWebPages(const string& query, int topN = 5) {
        if (sockfd_ == -1) {
            std::cerr << "未连接到服务器" << endl;
            return false;
        }
        
        cout << "\n正在搜索网页..." << endl;
        cout << "搜索词: " << query << endl;
        cout << "返回数量: " << topN << endl;
        cout << "----------------------------------------" << endl;
        
        try {
            // 构建请求
            auto request = TLVMessageBuilder::buildSearchRequest(query, topN);
            auto response = sendRequest(request);
            
            if (response.getType() == MessageType::SEARCH_RESPONSE) {
                displaySearchResults(response);
                return true;
            } else if (response.getType() == MessageType::ERROR_RESPONSE) {
                displayError(response);
                return false;
            } else {
                std::cerr << "收到未知类型的响应" << endl;
                return false;
            }
        } catch (const std::exception& e) {
            std::cerr << "网页搜索失败: " << e.what() << endl;
            return false;
        }
    }
    
    // 运行交互式客户端
    void runInteractive() {
        cout << "\n" << std::string(60, '=') << endl;
        cout << "欢迎使用搜索引擎客户端" << endl;
        cout << std::string(60, '=') << endl;
        cout << "可用命令:" << endl;
        cout << "  1. recommend <查询词> [数量]  - 关键字推荐" << endl;
        cout << "  2. search <查询词> [数量]     - 网页搜索" << endl;
        cout << "  3. help                      - 显示帮助信息" << endl;
        cout << "  4. status                    - 显示连接状态" << endl;
        cout << "  5. clear                     - 清空屏幕" << endl;
        cout << "  6. quit                      - 退出程序" << endl;
        cout << std::string(60, '=') << endl;
        
        string line;
        while (true) {
            cout << "\n搜索引擎> ";
            if (!std::getline(cin, line)) {
                break;
            }
            
            // 去除首尾空格
            line = trim(line);
            if (line.empty()) {
                continue;
            }
            
            // 解析命令
            std::istringstream iss(line);
            string command;
            iss >> command;
            
            if (command == "quit" || command == "exit" || command == "q") {
                cout << "感谢使用搜索引擎客户端，再见！" << endl;
                break;
            } else if (command == "help" || command == "h") {
                showHelp();
            } else if (command == "status") {
                showStatus();
            } else if (command == "clear") {
                int result = system("clear");
                (void)result;  // 明确忽略返回值
            } else if (command == "recommend" || command == "r") {
                handleRecommendCommand(iss);
            } else if (command == "search" || command == "s") {
                handleSearchCommand(iss);
            } else {
                cout << "未知命令: " << command << endl;
                cout << "输入 'help' 查看可用命令" << endl;
            }
        }
    }

private:
    string host_;
    int port_;
    int sockfd_;
    
    // 发送请求并接收响应
    TLVMessage sendRequest(const TLVMessage& request) {
        if (sockfd_ == -1) {
            throw std::runtime_error("未连接到服务器");
        }
        
        // 编码请求
        auto encodedRequest = TLVCodec::encode(request);
        
        // 发送请求
        if (send(sockfd_, encodedRequest.data(), encodedRequest.size(), 0) == -1) {
            throw std::runtime_error("发送请求失败");
        }
        
        // 接收响应
        vector<uint8_t> buffer;
        buffer.resize(8192);  // 预分配缓冲区
        
        size_t totalReceived = 0;
        while (true) {
            ssize_t received = recv(sockfd_, buffer.data() + totalReceived, 
                                  buffer.size() - totalReceived, 0);
            
            if (received <= 0) {
                throw std::runtime_error("接收响应失败");
            }
            
            totalReceived += received;
            
            // 检查是否有完整的消息
            vector<uint8_t> responseBuffer(buffer.begin(), buffer.begin() + totalReceived);
            if (TLVCodec::hasCompleteMessage(responseBuffer)) {
                size_t parsedBytes = 0;
                auto messages = TLVCodec::decode(responseBuffer, parsedBytes);
                
                if (!messages.empty()) {
                    return messages[0];  // 返回第一个消息
                }
            }
            
            // 如果缓冲区满了，扩容
            if (totalReceived >= buffer.size()) {
                buffer.resize(buffer.size() * 2);
            }
        }
    }
    
    // 显示关键字推荐结果
    void displayKeywordRecommendations(const TLVMessage& response) {
        try {
            auto responseJson = nlohmann::json::parse(response.getJsonData());
            
            string query = responseJson["query"];
            auto candidates = responseJson["candidates"];
            int timestamp = responseJson["timestamp"];
            
            cout << "推荐结果 (查询: " << query << ")" << endl;
            cout << "时间戳: " << timestamp << endl;
            cout << "推荐数量: " << candidates.size() << endl;
            cout << std::string(50, '-') << endl;
            
            if (candidates.empty()) {
                cout << "未找到相关推荐" << endl;
                return;
            }
            
            int index = 1;
            for (const auto& candidate : candidates) {
                string word = candidate["word"];
                int editDistance = candidate["editDistance"];
                int frequency = candidate["frequency"];
                
                cout << std::setw(2) << index << ". ";
                cout << std::setw(15) << std::left << word;
                cout << " [距离:" << std::setw(2) << editDistance 
                     << " 频次:" << std::setw(6) << frequency << "]" << endl;
                index++;
            }
        } catch (const std::exception& e) {
            std::cerr << "解析推荐结果失败: " << e.what() << endl;
        }
    }
    
    // 显示搜索结果
    void displaySearchResults(const TLVMessage& response) {
        try {
            auto responseJson = nlohmann::json::parse(response.getJsonData());
            
            string query = responseJson["query"];
            auto results = responseJson["results"];
            int total = responseJson["total"];
            int timestamp = responseJson["timestamp"];
            
            cout << "搜索结果 (查询: " << query << ")" << endl;
            cout << "时间戳: " << timestamp << endl;
            cout << "结果数量: " << total << endl;
            cout << std::string(80, '-') << endl;
            
            if (results.empty()) {
                cout << "未找到相关网页" << endl;
                return;
            }
            
            int index = 1;
            for (const auto& result : results) {
                int docid = result["docid"];
                string title = result["title"];
                string url = result["url"];
                string summary = result["summary"];
                double score = result["score"];
                
                cout << index << ". " << title << endl;
                cout << url << endl;
                cout << "文档ID: " << docid 
                     << " | 相关度: " << std::fixed << std::setprecision(4) << score << endl;
                cout << summary << endl;
                cout << std::string(80, '-') << endl;
                index++;
            }
        } catch (const std::exception& e) {
            std::cerr << "解析搜索结果失败: " << e.what() << endl;
        }
    }
    
    // 显示错误信息
    void displayError(const TLVMessage& response) {
        try {
            auto errorJson = nlohmann::json::parse(response.getJsonData());
            string message = errorJson.value("error", errorJson.value("message", "未知错误"));
            int code = errorJson.value("code", -1);
            
            cout << "服务器错误 [" << code << "]: " << message << endl;
        } catch (const std::exception& e) {
            cout << "解析错误响应失败: " << e.what() << endl;
        }
    }
    
    // 处理推荐命令
    void handleRecommendCommand(std::istringstream& iss) {
        string query;
        int k = 10;  // 默认推荐10个
        
        iss >> query;
        if (query.empty()) {
            cout << "请输入查询词" << endl;
            cout << "用法: recommend <查询词> [数量]" << endl;
            return;
        }
        
        // 读取剩余的查询词（支持多个词组成的查询）
        string word;
        while (iss >> word) {
            query += " " + word;
        }
        
        // 如果最后一个词是数字，则作为推荐数量
        std::istringstream lastWordCheck(word);
        int num;
        if (lastWordCheck >> num && lastWordCheck.eof() && num > 0 && num <= 50) {
            // 移除最后一个数字词
            size_t lastSpace = query.find_last_of(' ');
            if (lastSpace != string::npos) {
                query = query.substr(0, lastSpace);
                k = num;
            }
        }
        
        recommendKeywords(query, k);
    }
    
    // 处理搜索命令
    void handleSearchCommand(std::istringstream& iss) {
        string query;
        int topN = 5;  // 默认返回5个结果
        
        iss >> query;
        if (query.empty()) {
            cout << "请输入搜索词" << endl;
            cout << "用法: search <查询词> [数量]" << endl;
            return;
        }
        
        // 读取剩余的查询词（支持多个词组成的查询）
        string word;
        while (iss >> word) {
            query += " " + word;
        }
        
        // 如果最后一个词是数字，则作为返回数量
        std::istringstream lastWordCheck(word);
        int num;
        if (lastWordCheck >> num && lastWordCheck.eof() && num > 0 && num <= 20) {
            // 移除最后一个数字词
            size_t lastSpace = query.find_last_of(' ');
            if (lastSpace != string::npos) {
                query = query.substr(0, lastSpace);
                topN = num;
            }
        }
        
        searchWebPages(query, topN);
    }
    
    // 显示帮助信息
    void showHelp() {
        cout << "\n" << std::string(60, '=') << endl;
        cout << "搜索引擎客户端帮助" << endl;
        cout << std::string(60, '=') << endl;
        cout << "命令格式:" << endl;
        cout << "  recommend <查询词> [数量]" << endl;
        cout << "    - 获取关键字推荐" << endl;
        cout << "    - 示例: recommend 中国 5" << endl;
        cout << "    - 示例: recommend hello" << endl;
        cout << endl;
        cout << "  search <查询词> [数量]" << endl;
        cout << "    - 搜索相关网页" << endl;
        cout << "    - 示例: search 北京 天气 3" << endl;
        cout << "    - 示例: search 人工智能" << endl;
        cout << endl;
        cout << "  其他命令:" << endl;
        cout << "    help   - 显示此帮助信息" << endl;
        cout << "    status - 显示连接状态" << endl;
        cout << "    clear  - 清空屏幕" << endl;
        cout << "    quit   - 退出程序" << endl;
        cout << std::string(60, '=') << endl;
    }
    
    // 显示连接状态
    void showStatus() {
        cout << "\n客户端状态信息" << endl;
        cout << "服务器地址: " << host_ << ":" << port_ << endl;
        cout << "连接状态: " << (sockfd_ != -1 ? "已连接" : "未连接") << endl;
        if (sockfd_ != -1) {
            cout << "Socket FD: " << sockfd_ << endl;
        }
    }
    
    // 去除首尾空格
    string trim(const string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == string::npos) return "";
        size_t end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }
};

// 显示程序标题
void showBanner() {
    cout << string(18, ' ');
    cout << "           搜索引擎客户端           " << endl;
    cout << "功能特性:" << endl;
    cout << "  智能关键字推荐 - 基于编辑距离算法" << endl;
    cout << "  全文网页搜索   - 基于TF-IDF相关性排序" << endl;
    cout << "  交互式界面     - 支持多种命令" << endl;
    cout << "  实时通信       - TLV协议保证可靠传输" << endl;
    cout << std::string(80, '=') << endl;
}

int main(int argc, char* argv[]) {
    // 显示标题
    showBanner();
    
    // 解析命令行参数
    string serverHost = "127.0.0.1";
    int serverPort = 8080;
    
    if (argc >= 2) {
        serverHost = argv[1];
    }
    if (argc >= 3) {
        serverPort = std::atoi(argv[2]);
        if (serverPort <= 0 || serverPort > 65535) {
            std::cerr << "无效的端口号: " << argv[2] << endl;
            return 1;
        }
    }
    
    cout << "准备连接到服务器: " << serverHost << ":" << serverPort << endl;
    
    // 创建客户端
    SearchEngineClient client(serverHost, serverPort);
    
    // 连接到服务器
    if (!client.connect()) {
        cout << "请确保搜索引擎服务器正在运行" << endl;
        cout << "启动命令: ./search_server " << serverPort << endl;
        return 1;
    }
    
    // 运行交互式客户端
    client.runInteractive();
    
    return 0;
}