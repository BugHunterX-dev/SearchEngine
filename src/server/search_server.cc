#include "search_engine_server.h"
#include <cstdlib>
#include <iostream>
#include <signal.h>

using std::cout;
using std::endl;
using std::string;

// 全局服务器实例
SearchEngineServer* server = nullptr;

// 信号处理函数
void signalHandler(int signal) {
    cout << "\n收到信号 " << signal << "，正在优雅关闭服务器..." << endl;
    if (server) {
        server->stop();
        delete server;
        server = nullptr;
    }
    exit(0);
}

void showUsage(const char* programName) {
    cout << "使用方法:" << endl;
    cout << "  " << programName << " [端口号]" << endl;
    cout << "  " << programName << " [IP地址] [端口号]" << endl;
    cout << endl;
    cout << "参数说明:" << endl;
    cout << "  端口号    - 服务器监听端口 (默认: 8080)" << endl;
    cout << "  IP地址    - 服务器绑定地址 (默认: 0.0.0.0)" << endl;
    cout << "  数据目录  - 固定为 ./data" << endl;
    cout << endl;
    cout << "示例:" << endl;
    cout << "  " << programName
         << "                # 使用默认配置 (0.0.0.0:8080)" << endl;
    cout << "  " << programName << " 9000           # 指定端口 (0.0.0.0:9000)"
         << endl;
    cout << "  " << programName << " 127.0.0.1 8080 # 指定IP和端口" << endl;
}

void showBanner() {
    cout << "\n" << string(80, '=') << endl;
    cout << string(18, ' ');
    cout << "           搜索引擎服务器           " << endl;
    cout << string(80, '=') << endl;
}

int main(int argc, char* argv[]) {
    // 显示启动横幅
    showBanner();

    // 解析命令行参数
    string ip = "0.0.0.0";
    int port = 8080;
    string dataDir = "./data"; // 数据目录固定

    if (argc == 2) {
        if (string(argv[1]) == "--help" || string(argv[1]) == "-h") {
            showUsage(argv[0]);
            return 0;
        }
        // 一个参数：端口号
        port = std::atoi(argv[1]);
    } else if (argc == 3) {
        // 两个参数：IP地址 + 端口号
        ip = argv[1];
        port = std::atoi(argv[2]);
    } else if (argc > 3) {
        cout << "参数过多" << endl;
        showUsage(argv[0]);
        return 1;
    }

    // 验证端口号
    if (port <= 0 || port > 65535) {
        cout << "无效的端口号: " << port << endl;
        return 1;
    }

    cout << "服务器配置:" << endl;
    cout << "  监听地址: " << ip << ":" << port << endl;
    cout << "  数据目录: " << dataDir << endl;
    cout << "  线程数量: 4" << endl;
    cout << "  队列大小: 100" << endl;
    cout << endl;

    // 注册信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    try {
        // 创建搜索引擎服务器
        server = new SearchEngineServer(4, 100, ip,
                                          static_cast<unsigned short>(port));
        server->setDataDirectory(dataDir);

        // 启动服务器
        server->start();

        cout << "服务器运行中，按 Ctrl+C 优雅退出..." << endl;
        cout << "客户端连接命令: ./search_client " << ip << " " << port
             << endl;
        cout << string(80, '-') << endl;

        // 主线程进入事件循环
        while (true) {
            sleep(1);
        }

    } catch (const std::exception& e) {
        cout << "服务器启动失败: " << e.what() << endl;
        if (server) {
            delete server;
            server = nullptr;
        }
        return 1;
    }

    return 0;
}