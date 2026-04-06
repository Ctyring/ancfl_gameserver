#!/bin/bash

# 测试逻辑服务器启动
TEST_DIR="/root/ancfl/ancfl_gameserver/game_server/tests"
BIN_DIR="/root/ancfl/ancfl_gameserver/game_server/bin"
LOG_DIR="${TEST_DIR}/logs"
mkdir -p "${LOG_DIR}"

# 清理之前的进程
pkill -f "logic_server" 2>/dev/null
sleep 2

# 启动逻辑服务器
echo "启动逻辑服务器..."
${BIN_DIR}/logic_server > "${LOG_DIR}/logic_server.log" 2>&1 &
LOGIC_PID=$!
sleep 5

# 检查进程状态
if ps -p "${LOGIC_PID}" > /dev/null; then
    echo "逻辑服务器启动成功 (PID: ${LOGIC_PID})"
    # 检查端口是否绑定
    PORT=8003
    if lsof -i :${PORT} > /dev/null; then
        echo "逻辑服务器成功绑定端口 ${PORT}"
    else
        echo "逻辑服务器未绑定端口 ${PORT}"
    fi
    # 停止服务器
    kill "${LOGIC_PID}"
    echo "逻辑服务器已停止"
else
    echo "逻辑服务器启动失败"
    cat "${LOG_DIR}/logic_server.log"
fi
