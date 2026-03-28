#!/bin/bash

# 游戏服务器集成测试脚本
# 用于测试各个服务器的基本功能

echo "========================================"
echo "游戏服务器集成测试"
echo "========================================"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 测试结果统计
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# 测试函数
test_service() {
    local service_name=$1
    local service_path=$2
    local port=$3
    
    echo ""
    echo "----------------------------------------"
    echo "测试服务: $service_name"
    echo "----------------------------------------"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    # 检查可执行文件是否存在
    if [ ! -f "$service_path" ]; then
        echo -e "${RED}[失败]${NC} 可执行文件不存在: $service_path"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi
    
    # 检查端口是否被占用
    if [ ! -z "$port" ]; then
        if netstat -tuln 2>/dev/null | grep -q ":$port "; then
            echo -e "${YELLOW}[警告]${NC} 端口 $port 已被占用"
        fi
    fi
    
    # 尝试启动服务（后台运行，3秒后自动停止）
    echo "启动服务..."
    timeout 3s $service_path &
    local pid=$!
    
    # 等待服务启动
    sleep 2
    
    # 检查进程是否运行
    if ps -p $pid > /dev/null 2>&1; then
        echo -e "${GREEN}[成功]${NC} $service_name 启动成功 (PID: $pid)"
        
        # 等待 timeout 结束
        wait $pid 2>/dev/null
        
        PASSED_TESTS=$((PASSED_TESTS + 1))
        return 0
    else
        # 检查退出状态
        wait $pid 2>/dev/null
        local exit_code=$?
        
        # exit_code 124 表示 timeout，说明服务正常运行被强制终止
        if [ $exit_code -eq 124 ] || [ $exit_code -eq 0 ]; then
            echo -e "${GREEN}[成功]${NC} $service_name 启动成功"
            PASSED_TESTS=$((PASSED_TESTS + 1))
            return 0
        else
            echo -e "${RED}[失败]${NC} $service_name 启动失败 (退出码: $exit_code)"
            FAILED_TESTS=$((FAILED_TESTS + 1))
            return 1
        fi
    fi
}

# 测试数据库连接
test_database() {
    echo ""
    echo "----------------------------------------"
    echo "测试数据库连接"
    echo "----------------------------------------"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    # 检查 MySQL 是否运行
    if pgrep -x mysqld > /dev/null 2>&1 || systemctl is-active --quiet mysqld 2>/dev/null || systemctl is-active --quiet mysql 2>/dev/null; then
        echo -e "${GREEN}[成功]${NC} MySQL 服务正在运行"
        
        # 测试数据库连接
        if mysql -u root -p12345678 -e "SHOW DATABASES;" &>/dev/null; then
            echo -e "${GREEN}[成功]${NC} 数据库连接成功"
            
            # 检查游戏数据库是否存在
            if mysql -u root -p12345678 -e "USE game_server;" &>/dev/null; then
                echo -e "${GREEN}[成功]${NC} game_server 数据库存在"
                PASSED_TESTS=$((PASSED_TESTS + 1))
                return 0
            else
                echo -e "${YELLOW}[警告]${NC} game_server 数据库不存在，请运行 init_db.sh"
                FAILED_TESTS=$((FAILED_TESTS + 1))
                return 1
            fi
        else
            echo -e "${RED}[失败]${NC} 数据库连接失败，请检查密码"
            FAILED_TESTS=$((FAILED_TESTS + 1))
            return 1
        fi
    else
        echo -e "${RED}[失败]${NC} MySQL 服务未运行"
        echo "请先启动 MySQL: systemctl start mysqld"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi
}

# 测试配置文件
test_config() {
    echo ""
    echo "----------------------------------------"
    echo "测试配置文件"
    echo "----------------------------------------"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    # 检查配置目录
    if [ -d "./conf" ]; then
        echo -e "${GREEN}[成功]${NC} 配置目录存在"
        
        # 检查关键配置文件
        local config_files=("server.yml" "db_server.yml" "login_server.yml")
        local missing_count=0
        
        for config in "${config_files[@]}"; do
            if [ -f "./conf/$config" ]; then
                echo -e "${GREEN}[成功]${NC} 配置文件存在: $config"
            else
                echo -e "${YELLOW}[警告]${NC} 配置文件不存在: $config"
                missing_count=$((missing_count + 1))
            fi
        done
        
        if [ $missing_count -eq 0 ]; then
            PASSED_TESTS=$((PASSED_TESTS + 1))
            return 0
        else
            echo -e "${RED}[失败]${NC} 缺少 $missing_count 个配置文件"
            FAILED_TESTS=$((FAILED_TESTS + 1))
            return 1
        fi
    else
        echo -e "${RED}[失败]${NC} 配置目录不存在"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi
}

# 主测试流程
echo ""
echo "开始执行测试..."
echo ""

# 切换到游戏服务器目录
cd /root/ancfl/ancfl_gameserver/game_server

# 测试数据库
test_database

# 测试配置文件
test_config

# 测试各个服务器
test_service "Center Server" "bin/center_server" 9999
test_service "Login Server" "bin/login_server" 8200
test_service "DB Server" "bin/db_server" 8300
test_service "Account Server" "bin/account_server" 8100
test_service "Logic Server" "bin/logic_server" 8001
test_service "Proxy Server" "bin/proxy_server" 9001
test_service "Monitor Server" "bin/monitor_server" 8400
test_service "Log Server" "bin/log_server" 8500

# 输出测试结果
echo ""
echo "========================================"
echo "测试结果统计"
echo "========================================"
echo -e "总测试数: $TOTAL_TESTS"
echo -e "${GREEN}通过: $PASSED_TESTS${NC}"
echo -e "${RED}失败: $FAILED_TESTS${NC}"
echo ""

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}所有测试通过！${NC}"
    exit 0
else
    echo -e "${RED}部分测试失败，请检查日志${NC}"
    exit 1
fi
