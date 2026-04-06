#!/bin/bash

# 多进程网络测试脚本 - 简化版（仅测试代理服务器）
TEST_DIR="/root/ancfl/ancfl_gameserver/game_server/tests"
BIN_DIR="/root/ancfl/ancfl_gameserver/game_server/bin"
LOG_DIR="${TEST_DIR}/logs"
CLIENT_TEST="${TEST_DIR}/bin/game_client_tests"

# 颜色定义
GREEN="\033[0;32m"
RED="\033[0;31m"
YELLOW="\033[1;33m"
NC="\033[0m" # No Color

# 创建日志目录
mkdir -p "${LOG_DIR}"

# 清理之前的进程
echo -e "${YELLOW}清理之前的进程...${NC}"
pkill -f "center_server" 2>/dev/null
pkill -f "proxy_server" 2>/dev/null
pkill -f "logic_server" 2>/dev/null
sleep 2

# 启动中心服务器
echo -e "${GREEN}启动中心服务器...${NC}"
${BIN_DIR}/center_server > "${LOG_DIR}/center_server.log" 2>&1 &
CENTER_PID=$!
sleep 2

# 启动逻辑服务器
echo -e "${GREEN}启动逻辑服务器...${NC}"
${BIN_DIR}/logic_server > "${LOG_DIR}/logic_server.log" 2>&1 &
LOGIC_PID=$!
sleep 2

# 启动代理服务器
echo -e "${GREEN}启动代理服务器...${NC}"
${BIN_DIR}/proxy_server > "${LOG_DIR}/proxy_server.log" 2>&1 &
PROXY_PID=$!
sleep 10

# 检查所有服务器是否启动成功
echo -e "${YELLOW}检查服务器状态...${NC}"
ALL_OK=1
CENTER_OK=1
PROXY_OK=1

# 检查中心服务器
if ! ss -tulpn | grep -q ':8000 '; then
    echo -e "${RED}中心服务器启动失败${NC}"
    cat "${LOG_DIR}/center_server.log"
    ALL_OK=0
    CENTER_OK=0
else
    echo -e "${GREEN}中心服务器启动成功 (端口: 8000)${NC}"
fi

# 检查代理服务器
if ! ss -tulpn | grep -q ':8005 '; then
    echo -e "${RED}代理服务器启动失败${NC}"
    cat "${LOG_DIR}/proxy_server.log"
    ALL_OK=0
    PROXY_OK=0
else
    echo -e "${GREEN}代理服务器启动成功 (端口: 8005)${NC}"
fi

# 检查逻辑服务器
if ! ss -tulpn | grep -q ':8004 '; then
    echo -e "${RED}逻辑服务器启动失败${NC}"
    cat "${LOG_DIR}/logic_server.log"
    ALL_OK=0
else
    echo -e "${GREEN}逻辑服务器启动成功 (端口: 8004)${NC}"
fi

# 运行客户端测试
if [ "${CENTER_OK}" -eq 1 ] && [ "${PROXY_OK}" -eq 1 ]; then
    echo -e "${GREEN}中心服务器和代理服务器启动成功，运行客户端测试...${NC}"
    if [ -f "${CLIENT_TEST}" ]; then
        ${CLIENT_TEST} > "${LOG_DIR}/client_test.log" 2>&1
        TEST_RESULT=$?
        if [ "${TEST_RESULT}" -eq 0 ]; then
            echo -e "${GREEN}客户端测试成功${NC}"
        else
            echo -e "${RED}客户端测试失败${NC}"
            cat "${LOG_DIR}/client_test.log"
        fi
    else
        echo -e "${RED}客户端测试程序不存在: ${CLIENT_TEST}${NC}"
    fi
    
    # 运行代理服务器消息转发测试
    echo -e "${GREEN}运行代理服务器消息转发测试...${NC}"
    if [ -f "${TEST_DIR}/bin/game_client_tests" ]; then
        ${TEST_DIR}/bin/game_client_tests > "${LOG_DIR}/proxy_message_forwarding.log" 2>&1
        TEST_RESULT=$?
        if [ "${TEST_RESULT}" -eq 0 ]; then
            echo -e "${GREEN}代理服务器消息转发测试成功${NC}"
        else
            echo -e "${RED}代理服务器消息转发测试失败${NC}"
            cat "${LOG_DIR}/proxy_message_forwarding.log"
        fi
    else
        echo -e "${RED}客户端测试程序不存在: ${TEST_DIR}/bin/game_client_tests${NC}"
    fi
else
    echo -e "${RED}中心服务器或代理服务器启动失败，跳过客户端测试${NC}"
fi

# 停止所有服务器
echo -e "${YELLOW}停止所有服务器...${NC}"
if ps -p "${CENTER_PID}" > /dev/null; then
    kill "${CENTER_PID}"
fi
if ps -p "${PROXY_PID}" > /dev/null; then
    kill "${PROXY_PID}"
fi
if ps -p "${LOGIC_PID}" > /dev/null; then
    kill "${LOGIC_PID}"
fi

echo -e "${GREEN}测试完成${NC}"

