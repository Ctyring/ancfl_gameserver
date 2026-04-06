# 游戏服务器测试计划

## 测试环境准备

### 1. 数据库准备
```bash
# 初始化数据库
cd /root/ancfl/ancfl_gameserver/game_server/db_scripts
./init_db.sh
```

### 2. 编译测试程序
```bash
cd /root/ancfl/ancfl_gameserver/game_server
mkdir -p build && cd build
cmake ..
make game_server_tests
```

---

## 测试列表

### 一、基础组件测试

#### 1. 消息分发器测试 (test_message_dispatcher.cc)
- [x] 消息注册功能
- [x] 消息分发功能
- [x] 消息处理性能测试

#### 2. TCP服务测试 (test_tcp_service.cc)
- [x] TCP连接建立
- [x] 数据包发送与接收
- [x] 连接断开处理
- [x] 并发连接测试

#### 3. 共享内存测试 (test_shared_memory.cc)
- [x] 共享内存创建
- [x] 数据读写
- [x] 进程间通信

#### 4. 配置管理器测试 (test_config_manager.cc)
- [x] 配置文件加载
- [x] 配置项读取
- [x] 配置热更新

---

### 二、核心服务器测试

#### 5. 数据库服务测试 (test_db_server_direct.cc)
- [x] 创建账号
- [x] 创建重复账号（应失败）
- [x] 验证账号
- [x] 验证错误密码
- [x] 获取账号信息
- [x] 账号封禁和解封
- [x] 创建角色
- [x] 获取角色信息
- [x] 更新角色
- [x] 删除角色
- [x] 获取角色列表

#### 6. 中心服务器测试 (test_center_server.cc)
- [x] 服务器初始化
- [x] 服务器启动/停止
- [x] 服务器注册
- [x] 服务器注销
- [x] 获取服务器列表
- [x] 获取服务器信息
- [x] 更新服务器负载
- [x] 选择最低负载服务器
- [x] 创建跨服战斗

#### 7. 日志服务器测试 (test_log_server.cc)
- [x] 日志记录功能
- [x] 日志查询功能
- [x] 日志归档功能

#### 8. 监控服务器测试 (test_monitor_server.cc)
- [x] 监控数据采集
- [x] 性能指标统计
- [x] 告警功能

---

### 三、接入服务器测试

#### 9. 登录服务器测试 (test_login_server.cc)
- [x] 服务器初始化
- [x] 账号验证
- [x] 角色列表获取
- [x] 登录流程测试
- [x] 登出流程测试

#### 10. 账号服务器测试 (test_account_server.cc)
- [ ] 服务器初始化
- [ ] 账号创建
- [ ] 密码修改
- [ ] 账号查询
- [ ] 账号状态管理

#### 11. 代理服务器测试 (test_proxy_service_real.cc)
- [x] 客户端连接
- [x] 客户端断开连接
- [x] 创建会话
- [x] 删除会话
- [x] 添加逻辑服务器
- [x] 删除逻辑服务器
- [x] 负载均衡
- [x] 多客户端
- [x] 会话查找

---

### 四、逻辑服务器及游戏模块测试

#### 12. 逻辑服务器测试 (test_logic_server.cc)
- [ ] 服务器初始化
- [ ] 模块协调
- [ ] 消息处理
- [ ] 游戏逻辑执行

#### 13. 角色模块测试 (test_role_module.cc)
- [x] 创建角色
- [ ] 创建重复角色（应失败）
- [x] 获取角色信息
- [x] 获取不存在的角色（应失败）
- [x] 设置角色等级
- [x] 增加经验值
- [x] 删除角色
- [x] 升级测试
- [x] 连续升级测试
- [x] 角色在线状态
- [x] 角色属性
- [x] 角色位置

#### 14. 背包模块测试 (test_bag_module.cc)
- [ ] 初始化背包
- [ ] 添加物品
- [ ] 添加物品溢出测试
- [ ] 移除物品
- [ ] 移除物品数量不足（应失败）
- [ ] 移除不存在的物品（应失败）
- [ ] 获取物品数量
- [ ] 整理背包

#### 15. 装备模块测试 (test_equip_module.cc)
- [ ] 装备穿戴
- [ ] 装备卸下
- [ ] 装备强化
- [ ] 装备升星
- [ ] 宝石镶嵌
- [ ] 装备属性计算

---

### 五、游戏功能模块测试

#### 16. 场景模块测试 (test_scene_module.cc)
- [ ] 进入场景
- [ ] 离开场景
- [ ] 场景内移动
- [ ] 场景切换
- [ ] 场景玩家列表

#### 17. 技能模块测试 (test_skill_module.cc)
- [ ] 学习技能
- [ ] 升级技能
- [ ] 使用技能
- [ ] 技能冷却
- [ ] 技能效果

#### 18. Buff模块测试 (test_buff_module.cc)
- [ ] 添加Buff
- [ ] 移除Buff
- [ ] Buff叠加
- [ ] Buff过期
- [ ] Buff效果

#### 19. 任务模块测试 (test_task_module.cc)
- [ ] 接取任务
- [ ] 更新任务进度
- [ ] 完成任务
- [ ] 提交任务
- [ ] 放弃任务

---

### 六、社交与扩展功能测试

#### 20. 邮件模块测试 (test_mail_module.cc)
- [ ] 发送邮件
- [ ] 读取邮件
- [ ] 领取附件
- [ ] 删除邮件
- [ ] 邮件列表

#### 21. 好友模块测试 (test_friend_module.cc)
- [ ] 添加好友
- [ ] 删除好友
- [ ] 获取好友列表
- [ ] 好友在线状态
- [ ] 好友操作

#### 22. 商店模块测试 (test_shop_module.cc)
- [ ] 获取商品列表
- [ ] 购买商品
- [ ] 商品限购
- [ ] 商品刷新

#### 23. 公会模块测试 (test_guild_module.cc)
- [ ] 创建公会
- [ ] 加入公会
- [ ] 退出公会
- [ ] 公会职位管理
- [ ] 公会权限

#### 24. 活动模块测试 (test_activity_module.cc)
- [ ] 活动开启
- [ ] 活动参与
- [ ] 活动奖励
- [ ] 活动结束

---

### 七、服务器交互集成测试

#### 25. 服务器注册与发现测试 (test_server_registration.cc)
- [x] 服务器注册流程
- [x] 服务器发现机制
- [x] 服务器状态更新
- [x] 服务器健康检查

#### 26. 消息转发机制测试 (test_message_forwarding.cc)
- [x] 客户端消息转发
- [x] 服务器间消息转发
- [x] 消息路由测试
- [x] 消息优先级测试

#### 26.1 消息转发网络测试 (test_message_forwarding_network.cc)
- [x] 代理服务初始化
- [x] 逻辑服务初始化
- [x] 中心服务器运行状态
- [x] 服务器注册到中心服务器
- [x] 客户端连接流程
- [x] 完整登录流程
- [x] 逻辑服务器选择
- [x] 消息分发器注册
- [x] 会话超时清理
- [x] 多客户端并发连接
- [x] 服务器心跳更新
- [x] 服务器负载更新
- [x] 跨服玩家管理
- [x] 匹配队列管理
- [x] 消息序列化
- [x] 服务停止和重启
- [x] 定时器触发
- [x] 服务器选择最佳服务器
- [x] 广播消息
- [x] 消息路由

#### 27. 跨服务器通信测试 (test_cross_server_communication.cc)
- [ ] 服务器间数据传输
- [ ] 跨服战斗协调
- [ ] 跨服聊天功能
- [ ] 跨服数据同步

#### 28. 系统集成测试 (test_system_integration.cc)
- [ ] 完整登录流程
- [ ] 游戏功能集成
- [ ] 服务器协同工作
- [ ] 系统稳定性测试

---

## 测试执行顺序

### 阶段一：基础组件测试（优先级：高）
1. test_message_dispatcher.cc - 消息分发器
2. test_tcp_service.cc - TCP服务
3. test_shared_memory.cc - 共享内存
4. test_config_manager.cc - 配置管理器

### 阶段二：核心服务器测试（优先级：高）
5. test_db_server_direct.cc - 数据库服务器
6. test_center_server.cc - 中心服务器
7. test_log_server.cc - 日志服务器
8. test_monitor_server.cc - 监控服务器

### 阶段三：接入服务器测试（优先级：高）
9. test_login_server.cc - 登录服务器
10. test_account_server.cc - 账号服务器
11. test_proxy_service_real.cc - 代理服务器

### 阶段四：逻辑服务器及游戏模块测试（优先级：高）
12. test_logic_server.cc - 逻辑服务器
13. test_role_module.cc - 角色模块
14. test_bag_module.cc - 背包模块
15. test_equip_module.cc - 装备模块

### 阶段五：游戏功能模块测试（优先级：中）
16. test_scene_module.cc - 场景模块
17. test_skill_module.cc - 技能模块
18. test_buff_module.cc - Buff模块
19. test_task_module.cc - 任务模块

### 阶段六：社交与扩展功能测试（优先级：中）
20. test_mail_module.cc - 邮件模块
21. test_friend_module.cc - 好友模块
22. test_shop_module.cc - 商店模块
23. test_guild_module.cc - 公会模块
24. test_activity_module.cc - 活动模块

### 阶段七：服务器交互集成测试（优先级：高）
25. test_server_registration.cc - 服务器注册与发现
26. test_message_forwarding.cc - 消息转发机制
27. test_cross_server_communication.cc - 跨服务器通信
28. test_system_integration.cc - 系统集成

---

## 测试执行命令

### 运行所有测试
```bash
cd /root/ancfl/ancfl_gameserver/game_server/tests/bin
./game_server_tests
```

### 运行特定测试
```bash
# 运行单个测试套件
./game_server_tests --gtest_filter=CenterServerTest.*

# 运行单个测试用例
./game_server_tests --gtest_filter=CenterServerTest.Init
```

### 生成测试报告
```bash
./game_server_tests --gtest_output=xml:test_results.xml
```

---

## 测试注意事项

1. **数据库依赖**：部分测试需要MySQL数据库支持，请确保数据库已正确初始化
2. **端口占用**：网络相关测试需要确保端口未被占用
3. **并发测试**：并发测试可能需要调整系统资源限制
4. **清理数据**：每次测试后应清理测试数据，避免影响后续测试
5. **日志输出**：测试过程中注意查看日志，定位问题

---

## 测试结果记录模板

### 测试日期：____年____月____日
### 测试人员：__________
### 测试环境：
- 操作系统：
- 数据库版本：
- 编译器版本：

### 测试结果统计
- 总测试用例数：
- 通过数：
- 失败数：
- 跳过数：

### 失败用例详情
| 测试用例 | 失败原因 | 解决方案 |
|---------|---------|--------|
|         |         |        |

### 备注