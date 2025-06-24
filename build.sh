#!/bin/bash

echo "🚀 开始构建搜索引擎项目..."

# 检查CMake是否可用
if ! command -v cmake &> /dev/null; then
    echo "❌ 未找到 CMake"
    echo "💡 安装方法: sudo apt-get install cmake"
    exit 1
fi

# 检查nlohmann/json头文件
if [ ! -f "/usr/include/nlohmann/json.hpp" ] && [ ! -f "/usr/local/include/nlohmann/json.hpp" ]; then
    echo "❌ 未找到 nlohmann/json 头文件"
    echo "💡 请确保已安装 nlohmann-json3-dev 包"
    exit 1
fi

# 创建并进入构建目录
mkdir -p build
cd build

echo "📦 配置项目..."
cmake .. -DCMAKE_BUILD_TYPE=Release

if [ $? -ne 0 ]; then
    echo "❌ CMake 配置失败！"
    exit 1
fi

echo ""
echo "🔨 编译项目..."
echo "构建目标: search_server, search_client"
echo "编译器优化: Release模式"

make -j$(nproc)

if [ $? -eq 0 ]; then
    cd ..

    echo ""
    echo "✅ 构建成功！"
    echo "🎯 可执行文件:"
    
    if [ -f "search_server" ]; then
        size=$(ls -lh search_server | awk '{print $5}')
        echo "  📄 服务器程序: ./search_server (大小: $size)"
        echo "     启动命令: ./search_server [端口号]"
    fi
    
    if [ -f "search_client" ]; then
        size=$(ls -lh search_client | awk '{print $5}')
        echo "  📄 客户端程序: ./search_client (大小: $size)"
        echo "     启动命令: ./search_client [服务器地址] [端口号]"
    fi
    
    echo ""
    echo "📊 项目统计:"
    echo "  🔧 构建模式: Release"
    echo "  📂 构建目录: ./build/"
    echo "  💾 总体积: $(du -sh . | awk '{print $1}')"
    
else
    echo "❌ 构建失败！"
    exit 1
fi

