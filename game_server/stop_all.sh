#!/bin/bash

# 游戏服务器停止脚本
# 适用于Linux系统
# 功能：停止所有服务器进程

echo "========================================"
echo "游戏服务器停止脚本"
echo "========================================"

# 服务器进程名称
SERVER_PROCESSES=(
    "center_server"
    "login_server"
    "account_server"
    "logic_server"
    "db_server"
    "log_server"
    "proxy_server"
    "monitor_server"
)

echo "开始停止服务器..."

for process in "${SERVER_PROCESSES[@]}"; do
    # 查找进程
    PIDS=$(ps aux | grep "$process" | grep -v grep | awk '{print $2}')
    
    if [ -n "$PIDS" ]; then
        echo "停止 $process..."
        for PID in $PIDS; do
            kill -9 $PID
            echo "  停止进程 $PID"
        done
    else
        echo "$process 未运行"
    fi
done

echo "========================================"
echo "所有服务器停止完成！"
echo "========================================"
