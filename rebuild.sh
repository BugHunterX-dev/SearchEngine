#!/bin/bash

echo "🔄 开始重新构建搜索引擎项目..."

# 检查CMake是否可用
if ! command -v cmake &> /dev/null; then
    echo "❌ 未找到 CMake"
    echo "💡 安装方法: sudo apt-get install cmake"
    exit 1
fi

echo "🧹 清理旧的构建文件..."

# 清理构建目录
if [ -d "build" ]; then
rm -rf build
    echo "  ✅ 删除 build/ 目录"
fi

# 清理可执行文件
if [ -f "search_server" ]; then
    rm -f search_server
    echo "  ✅ 删除 search_server"
fi

if [ -f "search_client" ]; then
    rm -f search_client
    echo "  ✅ 删除 search_client"
fi

echo ""
echo "📦 重新配置项目..."

# 创建并进入构建目录
mkdir -p build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release

if [ $? -ne 0 ]; then
    echo "❌ CMake 配置失败！"
    exit 1
fi

echo ""
echo "🔨 重新编译项目..."
echo "构建目标: search_server, search_client"
echo "编译器优化: Release模式"

make clean
make -j$(nproc)

if [ $? -eq 0 ]; then
    cd ..
    
    echo ""
    echo "✅ 重新构建成功！"
    echo "🎯 生成的可执行文件:"
    
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
    echo "  🔧 构建模式: Release (重新构建)"
    echo "  📂 构建目录: ./build/"
    echo "  💾 总体积: $(du -sh . | awk '{print $1}')"
    echo "  🕒 完成时间: $(date '+%Y-%m-%d %H:%M:%S')"
    
else
    echo "❌ 重新构建失败！"
    exit 1
fi

