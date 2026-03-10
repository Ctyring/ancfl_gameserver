# 游戏服务器项目

基于ancfl框架实现的多进程游戏服务器，参考开源游戏服务器框架设计。

## 项目概述

这是一个完整的MMORPG游戏服务器框架，采用多服务器架构设计，支持完整的游戏功能实现。

### 项目特点
- **跨平台支持**：Windows (Visual Studio 2017+) 和 Linux
- **高性能网络**：基于协程的异步I/O模型
- **多服务器架构**：登录、账号、逻辑、数据、网关、中心、日志、监控服务器
- **完整游戏功能**：角色、背包、装备、战斗、任务、邮件、好友、公会、商店、活动系统
- **共享内存**：跨平台共享内存系统
- **静态数据管理**：Excel转SQLite配置系统
- **完整测试覆盖**：单元测试框架和测试用例

---

## 项目结构

```
game_server/
├── common/                 # 公共代码
│   ├── message_dispatcher.h/.cc   # 消息分发器
│   ├── tcp_service.h/.cc          # TCP服务基类
│   ├── game_service_base.h/.cc    # 游戏服务基类
│   ├── shared_memory.h/.cc        # 共享内存系统
│   └── config_manager.h/.cc       # 配置管理器
├── proto/                  # Protobuf协议定义
│   ├── msg_id.proto        # 消息ID定义
│   ├── msg_base.proto      # 基础数据结构
│   ├── msg_account.proto   # 账号相关协议
│   ├── msg_role.proto      # 角色相关协议
│   ├── msg_bag.proto       # 背包相关协议
│   ├── msg_equip.proto     # 装备相关协议
│   ├── msg_battle.proto    # 战斗相关协议
│   ├── msg_task.proto      # 任务相关协议
│   ├── msg_mail.proto      # 邮件相关协议
│   ├── msg_friend.proto    # 好友相关协议
│   ├── msg_guild.proto     # 公会相关协议
│   ├── msg_shop.proto      # 商店相关协议
│   ├── msg_activity.proto  # 活动相关协议
│   ├── msg_cross.proto     # 跨服相关协议
│   ├── msg_log.proto       # 日志相关协议
│   └── msg_monitor.proto   # 监控相关协议
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
├── logic_server/           # 逻辑服务器
│   ├── main.cc
│   ├── logic_service.h/.cc
│   ├── role_module.h/.cc       # 角色模块
│   ├── bag_module.h/.cc        # 背包模块
│   ├── equip_module.h/.cc      # 装备模块
│   ├── scene_module.h/.cc      # 场景模块
│   ├── skill_module.h/.cc      # 技能模块
│   ├── buff_module.h/.cc       # Buff模块
│   ├── task_module.h/.cc       # 任务模块
│   ├── mail_module.h/.cc       # 邮件模块
│   ├── friend_module.h/.cc     # 好友模块
│   ├── guild_module.h/.cc      # 公会模块
│   ├── shop_module.h/.cc       # 商店模块
│   └── activity_module.h/.cc   # 活动模块
├── db_server/              # 数据服务器
│   ├── main.cc
│   ├── db_service.h
│   └── db_service.cc
├── proxy_server/           # 网关服务器
│   ├── main.cc
│   ├── proxy_service.h
│   └── proxy_service.cc
├── center_server/          # 中心服务器（跨服）
│   ├── main.cc
│   ├── center_server.h
│   └── center_server.cc
├── log_server/             # 日志服务器
│   ├── main.cc
│   ├── log_server.h
│   └── log_server.cc
├── monitor_server/         # 监控服务器
│   ├── main.cc
│   └── monitor_server.h
├── tests/                  # 单元测试
│   ├── CMakeLists.txt
│   ├── test_main.cc
│   ├── test_message_dispatcher.cc
│   ├── test_tcp_service.cc
│   ├── test_shared_memory.cc
│   ├── test_config_manager.cc
│   ├── test_role_module.cc
│   ├── test_bag_module.cc
│   ├── test_equip_module.cc
│   ├── test_scene_module.cc
│   ├── test_skill_module.cc
│   ├── test_buff_module.cc
│   ├── test_task_module.cc
│   ├── test_mail_module.cc
│   ├── test_friend_module.cc
│   ├── test_shop_module.cc
│   ├── test_guild_module.cc
│   ├── test_activity_module.cc
│   ├── test_center_server.cc
│   ├── test_log_server.cc
│   └── test_monitor_server.cc
├── tools/                  # 工具
│   └── excel_to_sqlite.py  # Excel转SQLite工具
├── bin/                    # 可执行文件输出目录
├── lib/                    # 库文件输出目录
├── CMakeLists.txt          # CMake构建文件
└── README.md               # 项目说明
```

---

## 服务器架构

### 服务器角色说明

| 服务器     | 英文名称      | 功能说明                                            |
| ---------- | ------------- | --------------------------------------------------- |
| 登录服务器 | LoginServer   | 接受玩家登录连接，处理登录请求消息                  |
| 账号服务器 | AccountServer | 处理账号登录验证、新账号创建、账号数据保存          |
| 中心服务器 | CenterServer  | 用于跨服活动、跨服战需求                            |
| 逻辑服务器 | LogicServer   | 处理玩家角色逻辑数据、一般逻辑功能                  |
| 数据服务器 | DBServer      | 作为逻辑服和MySQL数据库之间的代理，定期写入玩家数据 |
| 网关服务器 | ProxyServer   | 作为客户端和逻辑服、战场服之间的中转，负责消息转发  |
| 日志服务器 | LogServer     | 负责逻辑服运营日志写入MySQL数据库                   |
| 监控服务器 | MonitorServer | 接受WEB后台控制命令，控制服务器                     |

### 服务器间通信关系

```
Client → ProxyServer → LogicServer → DBServer → MySQL
                ↓
         GameServer (战斗场景)
                ↓
         CenterServer (跨服)
                ↓
         LoginServer → AccountServer
```

---

## 已实现功能

### 第一阶段：基础框架搭建 ✅

#### 1. 网络服务基础
- [x] 基于ancfl创建基础TCP服务器
- [x] 消息分发器 (MessageDispatcher)
- [x] TCP服务基类 (TcpService)
- [x] 游戏服务基类 (GameServiceBase)
- [x] 消息处理器注册机制
- [x] Protobuf消息序列化/反序列化
- [x] 连接管理和心跳机制

#### 2. 协议定义
- [x] 消息ID定义 (msg_id.proto)
- [x] 基础数据结构 (msg_base.proto)
- [x] 账号相关协议 (msg_account.proto)
- [x] 角色相关协议 (msg_role.proto)
- [x] 背包相关协议 (msg_bag.proto)
- [x] 装备相关协议 (msg_equip.proto)
- [x] 战斗相关协议 (msg_battle.proto)
- [x] 任务相关协议 (msg_task.proto)
- [x] 邮件相关协议 (msg_mail.proto)
- [x] 好友相关协议 (msg_friend.proto)
- [x] 公会相关协议 (msg_guild.proto)
- [x] 商店相关协议 (msg_shop.proto)
- [x] 活动相关协议 (msg_activity.proto)
- [x] 跨服相关协议 (msg_cross.proto)
- [x] 日志相关协议 (msg_log.proto)
- [x] 监控相关协议 (msg_monitor.proto)

#### 3. 配置系统
- [x] YAML配置文件加载
- [x] 服务器配置结构
- [x] 配置热更新机制

#### 4. 共享内存系统
- [x] 跨平台共享内存管理
- [x] 数据块状态管理
- [x] 共享内存页面管理
- [x] 集成到服务器框架

### 第二阶段：核心服务器实现 ✅

#### 1. 登录服务器 (LoginServer)
- [x] 账号验证接口
- [x] 服务器列表查询
- [x] 登录码生成
- [x] 与账号服务器通信

#### 2. 账号服务器 (AccountServer)
- [x] 账号注册
- [x] 账号登录验证
- [x] 账号数据存储
- [x] 封号功能

#### 3. 逻辑服务器 (LogicServer)
- [x] 角色创建/登录
- [x] 角色数据加载
- [x] 基础模块框架
- [x] 与DBServer通信

#### 4. 数据服务器 (DBServer)
- [x] 数据库连接池
- [x] 数据缓存机制
- [x] 数据查询接口

#### 5. 网关服务器 (ProxyServer)
- [x] 客户端连接管理
- [x] 消息转发逻辑
- [x] 负载均衡
- [x] 会话管理

### 第三阶段：游戏功能模块 ✅

#### 1. 角色系统
- [x] 角色创建/删除
- [x] 角色属性管理
- [x] 角色等级系统
- [x] 角色数据持久化

#### 2. 背包系统
- [x] 物品管理
- [x] 背包格子管理
- [x] 物品使用/丢弃
- [x] 物品堆叠

#### 3. 装备系统
- [x] 装备穿戴/卸下
- [x] 装备强化
- [x] 装备星级
- [x] 装备套装

#### 4. 战斗系统
- [x] 场景管理
- [x] 对象管理
- [x] 技能系统
- [x] Buff系统
- [x] 移动同步

#### 5. 任务系统
- [x] 任务接取
- [x] 任务完成
- [x] 任务提交
- [x] 任务进度更新

#### 6. 邮件系统
- [x] 邮件发送
- [x] 邮件读取
- [x] 邮件删除
- [x] 附件领取

#### 7. 好友系统
- [x] 好友添加
- [x] 好友删除
- [x] 黑名单
- [x] 亲密度

#### 8. 公会系统
- [x] 公会创建
- [x] 公会管理
- [x] 成员管理
- [x] 公会活动

#### 9. 商店系统
- [x] 商店列表
- [x] 商品购买
- [x] 神秘商店
- [x] 限购系统

#### 10. 活动系统
- [x] 活动列表
- [x] 活动参与
- [x] 进度更新
- [x] 奖励领取

### 第四阶段：高级功能 ✅

#### 1. 跨服系统
- [x] 中心服务器实现
- [x] 服务器注册/发现
- [x] 跨服消息路由
- [x] 跨服战斗

#### 2. 日志系统
- [x] 日志服务器实现
- [x] 运营日志记录
- [x] 日志查询接口
- [x] 日志归档

#### 3. 监控系统
- [x] 监视服务器实现
- [x] 性能监控
- [x] 告警系统
- [x] 服务器控制命令

#### 4. 静态数据系统
- [x] Excel转SQLite工具
- [x] 配置数据加载
- [x] 配置热更新

---

## 构建项目

### 前置要求
- CMake 3.0+
- C++11 编译器
- Protobuf
- MySQL
- SQLite3
- OpenSSL
- YAML-CPP
- Google Test (GTest) - 用于测试

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

Linux:
```bash
make -j4
```

Windows (Visual Studio):
```bash
cmake --build . --config Release
```

---

## 运行测试

```bash
cd tests
mkdir build
cd build
cmake ..
make
./game_server_tests
```

---

## 运行服务器

### 1. 初始化数据库

```bash
mysql -u root -p < db_scripts/create_db.sql
```

### 2. 启动服务器顺序

```bash
# 1. 启动账号服务器
./bin/account_server

# 2. 启动数据服务器
./bin/db_server

# 3. 启动中心服务器
./bin/center_server

# 4. 启动日志服务器
./bin/log_server

# 5. 启动监控服务器
./bin/monitor_server

# 6. 启动逻辑服务器
./bin/logic_server

# 7. 启动网关服务器
./bin/proxy_server

# 8. 启动登录服务器
./bin/login_server
```

---

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

### logic_server.yml
- 服务名称和ID
- 监听IP和端口
- 场景配置
- 模块配置

---

## 协议说明

### 消息ID范围
- **100000-199999**: 通用消息(登录、注册、服务器间通信)
- **200000-299999**: 逻辑服务器消息(角色、背包、副本等)
- **300000-399999**: 战斗服务器消息
- **400000-499999**: 跨服消息
- **500000-599999**: 日志消息
- **600000-699999**: 监控消息

### 消息格式
```
包头 (16字节):
  - msg_id: uint32 (4字节)
  - msg_len: uint32 (4字节)
  - target_id: uint64 (8字节)
  - user_data: uint32 (4字节)

消息体: Protobuf序列化数据
```

---

## 技术栈

| 技术项     | 说明                               |
| ---------- | ---------------------------------- |
| **框架**   | ancfl (基于协程的C++网络框架)      |
| **协议**   | Protobuf                           |
| **数据库** | MySQL (主存储), SQLite3 (静态配置) |
| **缓存**   | Redis                              |
| **配置**   | YAML                               |
| **构建**   | CMake                              |
| **测试**   | Google Test (GTest)                |
| **网络**   | 协程异步I/O                        |
| **内存**   | 共享内存、对象池                   |

---

## 工具说明

### Excel转SQLite工具
```bash
cd tools
python excel_to_sqlite.py <excel目录> <输出sqlite文件>
```

将Excel配置表转换为SQLite数据库，供服务器加载使用。

---

## 参考

本项目参考开源游戏服务器框架设计，使用ancfl框架实现。
