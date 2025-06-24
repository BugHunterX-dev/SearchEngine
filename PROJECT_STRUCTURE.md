# 搜索引擎项目结构说明

## 项目概述
这是一个基于C++的全文搜索引擎项目，支持中英文混合搜索、关键字推荐、网页搜索等功能。

## 核心目录结构

```
search_engine/
├── src/                        # 源代码目录
│   ├── cache/                  # 缓存系统
│   ├── client/                 # 客户端程序
│   ├── data_reader/            # 数据读取器
│   ├── keywords/               # 分词与文本处理
│   ├── network/                # 网络通信模块
│   ├── recommendation/         # 关键字推荐
│   ├── server/                 # 服务器程序
│   ├── web_search/             # 网页搜索引擎核心
│   └── webpages/               # 网页处理
├── corpus/                     # 语料库
│   ├── CN/                     # 中文语料
│   ├── EN/                     # 英文语料
│   ├── stopwords/              # 停用词表
│   └── webpages/               # 网页数据
├── data/                       # 处理后的数据文件
│   ├── dict_cn.dat             # 中文词典库
│   ├── dict_en.dat             # 英文词典库
│   ├── index_cn.dat            # 中文索引库
│   ├── index_en.dat            # 英文索引库
│   ├── inverted_index.dat      # 倒排索引库
│   ├── offsets.dat             # 偏移库
│   └── webpages.dat            # 网页库
└── build/                      # 构建目录
```

## 核心程序文件

### 服务器程序
- `src/server/search_server.cc` - 搜索引擎服务器主程序
- `src/server/search_engine_server.cc` - 服务器核心实现
- `search_server` - 编译后的服务器可执行文件 (16MB)

### 客户端程序
- `src/client/search_client.cc` - 搜索引擎客户端主程序  
- `search_client` - 编译后的客户端可执行文件 (3.3MB)

### 核心模块
- `src/web_search/` - 网页搜索引擎模块 (WebSearchEngine类)
- `src/recommendation/` - 关键字推荐模块 (KeywordRecommender类)
- `src/network/` - 网络通信模块 (Reactor模式 + TLV协议)
- `src/cache/` - 高性能LRU缓存模块

### 构建系统
- `CMakeLists.txt` - CMake配置文件 (支持并行构建)
- `build.sh` - 智能构建脚本 (Release模式优化)
- `rebuild.sh` - 完全重新构建脚本 (清理+重构建)

## 功能特性

### 🚀 核心功能
- **全文搜索**: 基于TF-IDF算法的向量空间模型
- **关键字推荐**: 基于编辑距离的智能推荐
- **中英文支持**: 统一的分词处理框架
- **高性能缓存**: LRU缓存系统，显著提升查询速度

### 🌐 网络架构
- **Reactor模型**: 高并发事件驱动架构
- **线程池**: 异步任务处理
- **TLV协议**: 自定义的可靠通信协议
- **JSON数据**: 结构化的请求/响应格式

### 📊 性能特色
- **缓存优化**: 重复查询近乎瞬时响应
- **内存管理**: 高效的数据结构设计
- **并发处理**: 支持多客户端同时访问

## 使用方法

### 构建项目
```bash
# 首次构建或增量构建
./build.sh

# 完全重新构建 (推荐)
./rebuild.sh
```

### 启动服务器
```bash
./search_server [端口号]
# 默认端口: 8080
```

### 启动客户端
```bash
./search_client [服务器地址] [端口号]
# 默认连接: 127.0.0.1:8080
```

### 客户端命令
- `recommend <查询词> [数量]` - 关键字推荐
- `search <查询词> [数量]` - 网页搜索
- `help` - 显示帮助信息
- `status` - 显示连接状态
- `quit` - 退出程序

## 数据统计
- **中文词汇**: 17,315个
- **英文词汇**: 34,705个  
- **网页数量**: 3,763个
- **倒排索引**: 92,676个词汇条目

## 技术栈
- **编程语言**: C++17
- **JSON处理**: nlohmann/json
- **网络编程**: Linux socket API
- **并发模型**: Reactor + 线程池
- **数据格式**: 自定义二进制格式 + JSON

## 编译要求
- C++17 兼容编译器 (g++ 7.0+)
- nlohmann/json 库
- Linux 环境 