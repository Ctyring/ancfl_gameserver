#!/bin/bash

# 游戏服务器编译脚本
# 适用于Linux系统
# 功能：使用CMake编译所有服务器

echo "========================================"
echo "游戏服务器编译脚本"
echo "========================================"

# 脚本目录
SCRIPT_DIR=$(dirname "$0")
BUILD_DIR="$SCRIPT_DIR/build"

# 检查CMake是否安装
if ! command -v cmake &> /dev/null; then
    echo "错误：CMake未安装，请先安装CMake"
    exit 1
fi

# 检查g++是否安装
if ! command -v g++ &> /dev/null; then
    echo "错误：g++未安装，请先安装g++"
    exit 1
fi

echo "开始编译游戏服务器..."

# 创建构建目录
if [ ! -d "$BUILD_DIR" ]; then
    echo "创建构建目录..."
    mkdir -p "$BUILD_DIR"
fi

# 进入构建目录
cd "$BUILD_DIR"

# 运行CMake
echo "运行CMake生成Makefile..."
cmake ..
if [ $? -ne 0 ]; then
    echo "错误：CMake执行失败"
    exit 1
fi

# 编译
echo "开始编译..."
make -j4
if [ $? -ne 0 ]; then
    echo "错误：编译失败"
    exit 1
fi

echo "编译成功！"
echo "可执行文件位置：$BUILD_DIR"
echo "========================================"
echo "编译完成！"
echo "========================================"
