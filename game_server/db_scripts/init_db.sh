#!/bin/bash

# 游戏服务器数据库初始化脚本
# 适用于Linux系统
# 功能：创建数据库、初始化表结构、导入配置数据

echo "========================================"
echo "游戏服务器数据库初始化脚本"
echo "========================================"

# 数据库连接参数
DB_USER="root"
DB_PASS="root"  # 请根据实际情况修改
DB_HOST="localhost"
DB_PORT="3306"

# 脚本目录
SCRIPT_DIR=$(dirname "$0")

# 数据库名称
DB_NAME_GAME_SERVER="game_server"
DB_NAME_GAME_CONFIG="game_config"
DB_NAME_GAME_DB="game_db"

# SQL文件路径
SQL_CREATE_DB="$SCRIPT_DIR/create_db.sql"
SQL_CREATE_CONFIG_DB="$SCRIPT_DIR/create_config_db.sql"
SQL_CREATE_PLAYER_DATA="$SCRIPT_DIR/create_player_data_tables.sql"
SQL_INSERT_CONFIG_DATA="$SCRIPT_DIR/insert_config_data.sql"

# 检查MySQL是否安装
if ! command -v mysql &> /dev/null; then
    echo "错误：MySQL客户端未安装，请先安装MySQL客户端"
    exit 1
fi

# 检查SQL文件是否存在
if [ ! -f "$SQL_CREATE_DB" ] || [ ! -f "$SQL_CREATE_CONFIG_DB" ] || [ ! -f "$SQL_CREATE_PLAYER_DATA" ] || [ ! -f "$SQL_INSERT_CONFIG_DATA" ]; then
    echo "错误：SQL文件不存在，请确保所有SQL文件都在脚本目录中"
    exit 1
fi

# 连接MySQL并执行初始化
echo "正在连接MySQL..."

# 测试连接
mysql -h "$DB_HOST" -P "$DB_PORT" -u "$DB_USER" -p"$DB_PASS" -e "SELECT 1" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "错误：无法连接到MySQL，请检查连接参数"
    exit 1
fi

echo "连接成功，开始初始化数据库..."

# 执行主数据库创建脚本
echo "创建主数据库和表结构..."
mysql -h "$DB_HOST" -P "$DB_PORT" -u "$DB_USER" -p"$DB_PASS" < "$SQL_CREATE_DB"
if [ $? -ne 0 ]; then
    echo "错误：执行create_db.sql失败"
    exit 1
fi

echo "主数据库创建成功"

# 执行配置数据库创建脚本
echo "创建配置数据库和表结构..."
mysql -h "$DB_HOST" -P "$DB_PORT" -u "$DB_USER" -p"$DB_PASS" < "$SQL_CREATE_CONFIG_DB"
if [ $? -ne 0 ]; then
    echo "错误：执行create_config_db.sql失败"
    exit 1
fi

echo "配置数据库创建成功"

# 执行玩家数据表创建脚本
echo "创建玩家数据表结构..."
mysql -h "$DB_HOST" -P "$DB_PORT" -u "$DB_USER" -p"$DB_PASS" < "$SQL_CREATE_PLAYER_DATA"
if [ $? -ne 0 ]; then
    echo "错误：执行create_player_data_tables.sql失败"
    exit 1
fi

echo "玩家数据表创建成功"

# 导入配置数据
echo "导入配置数据..."
mysql -h "$DB_HOST" -P "$DB_PORT" -u "$DB_USER" -p"$DB_PASS" < "$SQL_INSERT_CONFIG_DATA"
if [ $? -ne 0 ]; then
    echo "错误：执行insert_config_data.sql失败"
    exit 1
fi

echo "配置数据导入成功"

# 验证数据库创建结果
echo "验证数据库创建结果..."

# 检查数据库是否存在
DB_LIST=$(mysql -h "$DB_HOST" -P "$DB_PORT" -u "$DB_USER" -p"$DB_PASS" -e "SHOW DATABASES" | grep -E "$DB_NAME_GAME_SERVER|$DB_NAME_GAME_CONFIG|$DB_NAME_GAME_DB")

if [ $(echo "$DB_LIST" | wc -l) -eq 3 ]; then
    echo "所有数据库创建成功！"
else
    echo "警告：部分数据库可能未创建成功"
    echo "已创建的数据库："
    echo "$DB_LIST"
fi

echo "========================================"
echo "数据库初始化完成！"
echo "========================================"
echo "注意：请根据实际情况修改脚本中的数据库连接参数"
echo "特别是密码和主机地址"
