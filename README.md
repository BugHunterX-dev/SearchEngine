# 🔍 搜索引擎 (Search Engine)

一个基于C++17开发的高性能全文搜索引擎，支持中英文混合搜索、关键字推荐和网页搜索功能。

## ✨ 核心特性

- 🌐 **全文搜索**: 基于TF-IDF算法的向量空间模型，支持精准的文档检索
- 🔤 **中英文支持**: 统一的分词处理框架，无缝处理中英文混合内容
- 💡 **智能推荐**: 基于编辑距离算法的关键字推荐系统
- ⚡ **高性能缓存**: LRU缓存机制，重复查询近乎瞬时响应
- 🏗️ **Reactor架构**: 高并发事件驱动网络架构
- 📡 **TLV协议**: 自定义的可靠通信协议
- 🎯 **多客户端支持**: 支持并发连接和查询处理

## 📊 数据规模

- **中文词汇**: 17,315个
- **英文词汇**: 34,705个  
- **网页数量**: 3,763个
- **倒排索引**: 92,676个词汇条目

## 🛠️ 技术栈

- **编程语言**: C++17
- **JSON处理**: nlohmann/json
- **网络编程**: Linux socket API
- **并发模型**: Reactor + 线程池
- **数据格式**: 自定义二进制格式 + JSON
- **构建系统**: CMake

## 📋 系统要求

- Linux 操作系统
- C++17 兼容编译器 (g++ 7.0+)
- CMake 3.10+
- nlohmann/json 库

## 🚀 快速开始

### 1. 克隆仓库

```bash
# GitHub
git clone https://github.com/BugHunterX-dev/SearchEngine.git
cd SearchEngine

# 或者 Gitee
git clone https://gitee.com/BugHunterX/search-engine.git
cd search-engine
```

### 2. 构建项目

```bash
# 首次构建或增量构建
./build.sh

# 或者完全重新构建 (推荐)
./rebuild.sh
```

### 3. 启动服务器

```bash
# 使用默认端口 8080
./search_server

# 或指定端口
./search_server 9999
```

### 4. 启动客户端

```bash
# 连接到本地服务器
./search_client

# 或连接到远程服务器
./search_client 192.168.1.100 9999
```

## 💻 使用指南

### 客户端命令

- `search <查询词> [结果数量]` - 执行网页搜索
- `recommend <查询词> [推荐数量]` - 获取关键字推荐
- `help` - 显示帮助信息
- `status` - 显示连接状态
- `quit` - 退出程序

### 使用示例

```bash
# 搜索相关网页 (默认返回5个结果)
search "人工智能"

# 搜索并指定返回10个结果
search "machine learning" 10

# 获取关键字推荐 (默认10个推荐)
recommend "搜索引擎"

# 获取5个关键字推荐
recommend "algorithm" 5
```

## 🏗️ 项目架构

```
search_engine/
├── src/                        # 源代码目录
│   ├── cache/                  # LRU缓存系统
│   ├── client/                 # 客户端程序
│   ├── config/                 # 配置文件读取模块
│   ├── data_reader/            # 数据读取器
│   ├── keywords/               # 分词与文本处理
│   ├── network/                # 网络通信模块
│   ├── recommendation/         # 关键字推荐
│   ├── server/                 # 服务器程序
│   ├── web_search/             # 网页搜索引擎核心
│   └── webpages/               # 网页处理
├── corpus/                     # 语料库
├── data/                       # 索引和字典文件
├── config/                     # 配置文件
└── tools/                      # 构建工具
```

## 🔧 配置说明

主要配置文件位于 `config/search_engine.conf`：

```ini
# 服务器配置
server_ip=0.0.0.0
server_port=8080
thread_num=4

# 算法参数
max_edit_distance=2
default_recommend_k=10
default_search_top_n=5

# 缓存配置
recommend_cache_size=500
search_cache_size=200
```

## 🔨 工具程序

项目包含以下构建工具：

- `build_dictionaries` - 构建中英文词典和索引库
- `build_webpages` - 处理网页数据并构建倒排索引
- `config_test` - 配置文件测试工具

## 🚦 开发状态

- ✅ 核心搜索引擎
- ✅ 关键字推荐系统  
- ✅ 网络通信模块
- ✅ 缓存系统
- ✅ 中英文分词
- ✅ 客户端程序

## 📝 许可证

本项目采用 MIT 许可证。详情请参阅 [LICENSE](LICENSE) 文件。

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 打开 Pull Request

## 📧 联系方式

- GitHub: [@BugHunterX-dev](https://github.com/BugHunterX-dev)
- Gitee: [@BugHunterX](https://gitee.com/BugHunterX)

## 🙏 致谢

感谢所有为这个项目做出贡献的开发者和测试者！ 