#!/bin/bash

# 简单的代理服务器客户端测试脚本

# 服务器地址和端口
SERVER_IP="127.0.0.1"
SERVER_PORT="8002"

# 测试消息
TEST_MSG='{"msg_id": 1001, "username": "test", "password": "test123"}'

echo "测试连接到代理服务器: ${SERVER_IP}:${SERVER_PORT}"
echo "发送测试消息: ${TEST_MSG}"

# 使用 nc 命令发送测试消息
echo -n "${TEST_MSG}" | nc ${SERVER_IP} ${SERVER_PORT}

if [ $? -eq 0 ]; then
    echo "\n测试成功：成功连接到代理服务器并发送消息"
else
    echo "\n测试失败：无法连接到代理服务器"
fi
