#!/bin/bash

# 游戏服务器启动脚本
# 适用于Linux系统
# 功能：启动所有服务器进程

echo "========================================"
echo "游戏服务器启动脚本"
echo "========================================"

# 脚本目录
SCRIPT_DIR=$(dirname "$0")
BUILD_DIR="$SCRIPT_DIR/build"

# 检查构建目录是否存在
if [ ! -d "$BUILD_DIR" ]; then
    echo "错误：构建目录不存在，请先运行 build.sh 编译服务器"
    exit 1
fi

# 服务器列表
SERVERS=(
    "center_server"
    "login_server"
    "account_server"
    "logic_server"
    "db_server"
    "log_server"
    "proxy_server"
    "monitor_server"
)

# 启动所有服务器
echo "开始启动服务器..."

for server in "${SERVERS[@]}"; do
    SERVER_BIN="$BUILD_DIR/$server"
    
    # 检查可执行文件是否存在
    if [ ! -f "$SERVER_BIN" ]; then
        echo "警告：$server 可执行文件不存在，跳过启动"
        continue
    fi
    
    # 启动服务器
    echo "启动 $server..."
    "$SERVER_BIN" &
    
    # 记录进程ID
    PID=$!
    echo "$server 启动成功，PID: $PID"
    
    # 短暂延迟，确保服务器启动
    sleep 1
done

echo "========================================"
echo "所有服务器启动完成！"
echo "========================================"
echo "提示：使用 'ps aux | grep server' 查看服务器进程"
echo "      使用 'kill -9 PID' 停止指定服务器"
echo "      使用 './stop_all.sh' 停止所有服务器"
