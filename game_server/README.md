# 游戏服务器项目

基于ancfl框架实现的多进程游戏服务器，参考开源游戏服务器框架设计。

## 项目结构

```
game_server/
├── common/                 # 公共代码
│   ├── message_dispatcher.h/.cc   # 消息分发器
│   ├── tcp_service.h/.cc          # TCP服务基类
│   └── game_service_base.h/.cc    # 游戏服务基类
├── proto/                  # Protobuf协议定义
│   ├── msg_id.proto        # 消息ID定义
│   ├── msg_base.proto      # 基础数据结构
│   ├── msg_account.proto   # 账号相关协议
│   ├── msg_role.proto      # 角色相关协议
│   ├── msg_bag.proto       # 背包相关协议
│   ├── msg_equip.proto     # 装备相关协议
│   └── msg_battle.proto    # 战斗相关协议
├── conf/                   # 配置文件
│   ├── server.yml          # 全局配置
│   ├── login_server.yml    # 登录服务器配置
│   ├── account_server.yml  # 账号服务器配置
│   ├── logic_server.yml    # 逻辑服务器配置
│   ├── db_server.yml       # 数据服务器配置
│   └── proxy_server.yml    # 网关服务器配置
├── db_scripts/             # 数据库脚本
│   └── create_db.sql       # 数据库创建脚本
├── login_server/           # 登录服务器
│   ├── main.cc
│   ├── login_service.h
│   └── login_service.cc
├── account_server/         # 账号服务器
│   ├── main.cc
│   ├── account_service.h
│   └── account_service.cc
├── logic_server/           # 逻辑服务器 (待实现)
├── db_server/              # 数据服务器 (待实现)
├── proxy_server/           # 网关服务器 (待实现)
├── bin/                    # 可执行文件输出目录
├── lib/                    # 库文件输出目录
├── CMakeLists.txt          # CMake构建文件
└── README.md               # 项目说明
```

## 已实现功能

### 第一阶段：基础框架搭建 ✅

#### 1. 网络服务基础
- [x] 消息分发器 (MessageDispatcher)
- [x] TCP服务基类 (TcpService)
- [x] 游戏服务基类 (GameServiceBase)
- [x] 消息处理器注册机制
- [x] 心跳检测机制

#### 2. 协议定义
- [x] 消息ID定义 (msg_id.proto)
- [x] 基础数据结构 (msg_base.proto)
- [x] 账号相关协议 (msg_account.proto)
- [x] 角色相关协议 (msg_role.proto)
- [x] 背包相关协议 (msg_bag.proto)
- [x] 装备相关协议 (msg_equip.proto)
- [x] 战斗相关协议 (msg_battle.proto)

#### 3. 配置系统
- [x] YAML配置文件
- [x] 服务器配置
- [x] 数据库配置
- [x] 心跳配置

### 第二阶段：核心服务器实现 (部分完成)

#### 1. 登录服务器 (LoginServer) - 框架完成
- [x] 服务初始化和反初始化
- [x] 消息处理器注册
- [x] 定时器处理
- [x] 逻辑服务器管理
- [ ] 消息处理实现

#### 2. 账号服务器 (AccountServer) - 框架完成
- [x] 数据库连接
- [x] 账号缓存机制
- [x] 账号创建/验证
- [x] 封号/解封功能
- [x] 登录日志记录
- [ ] 消息处理实现

#### 3. 逻辑服务器 (LogicServer) - 待实现
- [ ] 角色管理
- [ ] 背包系统
- [ ] 装备系统
- [ ] 技能系统
- [ ] 任务系统

#### 4. 数据服务器 (DBServer) - 待实现
- [ ] 数据库代理
- [ ] 数据缓存
- [ ] 批量写入

#### 5. 网关服务器 (ProxyServer) - 待实现
- [ ] 客户端连接管理
- [ ] 消息转发
- [ ] 负载均衡

## 构建项目

### 1. 生成Protobuf代码

```bash
cd game_server
protoc --cpp_out=./proto -I ./proto proto/*.proto
```

### 2. 创建构建目录

```bash
mkdir build
cd build
```

### 3. 运行CMake

```bash
cmake ..
```

### 4. 编译

```bash
make -j4
```

## 运行服务器

### 1. 初始化数据库

```bash
mysql -u root -p < db_scripts/create_db.sql
```

### 2. 启动服务器

```bash
# 启动账号服务器
./bin/account_server

# 启动登录服务器
./bin/login_server
```

## 配置文件说明

### server.yml
- 日志配置
- 数据库配置
- Redis配置
- Zookeeper配置

### login_server.yml
- 服务名称和ID
- 监听IP和端口
- 心跳配置
- 连接的其他服务器

### account_server.yml
- 服务名称和ID
- 监听IP和端口
- 数据库连接信息

## 协议说明

### 消息ID范围
- 100000-199999: 通用消息
- 200000-299999: 逻辑服务器消息

### 消息格式
```
包头 (16字节):
  - msg_id: uint32 (4字节)
  - msg_len: uint32 (4字节)
  - target_id: uint64 (8字节)
  - user_data: uint32 (4字节)

消息体: Protobuf序列化数据
```

## 开发计划

### 已完成
1. ✅ 基础框架搭建
2. ✅ 协议定义
3. ✅ 配置系统
4. ✅ 登录服务器框架
5. ✅ 账号服务器框架

### 待完成
1. ⏳ 完善登录服务器消息处理
2. ⏳ 完善账号服务器消息处理
3. ⏳ 实现逻辑服务器
4. ⏳ 实现数据服务器
5. ⏳ 实现网关服务器
6. ⏳ 实现游戏功能模块

## 技术栈

- **框架**: ancfl (基于协程的C++网络框架)
- **协议**: Protobuf
- **数据库**: MySQL
- **缓存**: Redis
- **配置**: YAML
- **构建**: CMake

## 参考

本项目参考开源游戏服务器框架设计，使用ancfl框架实现。
