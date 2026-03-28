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
mkdir -p build_tests && cd build_tests
cmake ../tests
make
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
- [ ] 共享内存创建
- [ ] 数据读写
- [ ] 进程间通信

#### 4. 配置管理器测试 (test_config_manager.cc)
- [ ] 配置文件加载
- [ ] 配置项读取
- [ ] 配置热更新

---

### 二、服务器功能测试

#### 5. 中心服务器测试 (test_center_server.cc)
- [ ] 服务器初始化
- [ ] 服务器启动/停止
- [ ] 服务器注册
- [ ] 服务器注销
- [ ] 获取服务器列表
- [ ] 获取服务器信息
- [ ] 更新服务器负载
- [ ] 选择最低负载服务器
- [ ] 创建跨服战斗

#### 6. 日志服务器测试 (test_log_server.cc)
- [ ] 日志记录功能
- [ ] 日志查询功能
- [ ] 日志归档功能

#### 7. 监控服务器测试 (test_monitor_server.cc)
- [ ] 监控数据采集
- [ ] 性能指标统计
- [ ] 告警功能

---

### 三、游戏模块测试

#### 8. 角色模块测试 (test_role_module.cc)
- [ ] 创建角色
- [ ] 创建重复角色（应失败）
- [ ] 获取角色信息
- [ ] 获取不存在的角色（应失败）
- [ ] 设置角色等级
- [ ] 增加经验值

#### 9. 背包模块测试 (test_bag_module.cc)
- [ ] 初始化背包
- [ ] 添加物品
- [ ] 添加物品溢出测试
- [ ] 移除物品
- [ ] 移除物品数量不足（应失败）
- [ ] 移除不存在的物品（应失败）
- [ ] 获取物品数量
- [ ] 整理背包

#### 10. 装备模块测试 (test_equip_module.cc)
- [ ] 装备穿戴
- [ ] 装备卸下
- [ ] 装备强化
- [ ] 装备升星
- [ ] 宝石镶嵌
- [ ] 装备属性计算

#### 11. 场景模块测试 (test_scene_module.cc)
- [ ] 进入场景
- [ ] 离开场景
- [ ] 场景内移动
- [ ] 场景切换
- [ ] 场景玩家列表

#### 12. 技能模块测试 (test_skill_module.cc)
- [ ] 学习技能
- [ ] 升级技能
- [ ] 使用技能
- [ ] 技能冷却
- [ ] 技能效果

#### 13. Buff模块测试 (test_buff_module.cc)
- [ ] 添加Buff
- [ ] 移除Buff
- [ ] Buff叠加
- [ ] Buff过期
- [ ] Buff效果

#### 14. 任务模块测试 (test_task_module.cc)
- [ ] 接取任务
- [ ] 更新任务进度
- [ ] 完成任务
- [ ] 提交任务
- [ ] 放弃任务

#### 15. 邮件模块测试 (test_mail_module.cc)
- [ ] 发送邮件
- [ ] 读取邮件
- [ ] 领取附件
- [ ] 删除邮件
- [ ] 邮件列表

#### 16. 好友模块测试 (test_friend_module.cc)
- [ ] 添加好友
- [ ] 删除好友
- [ ] 获取好友列表
- [ ] 好友在线状态
- [ ] 好友操作

#### 17. 商店模块测试 (test_shop_module.cc)
- [ ] 获取商品列表
- [ ] 购买商品
- [ ] 商品限购
- [ ] 商品刷新

#### 18. 公会模块测试 (test_guild_module.cc)
- [ ] 创建公会
- [ ] 加入公会
- [ ] 退出公会
- [ ] 公会职位管理
- [ ] 公会权限

#### 19. 活动模块测试 (test_activity_module.cc)
- [ ] 活动开启
- [ ] 活动参与
- [ ] 活动奖励
- [ ] 活动结束

---

## 测试执行顺序

### 阶段一：基础组件测试（优先级：高）
1. test_message_dispatcher.cc - 消息分发器
2. test_tcp_service.cc - TCP服务
3. test_shared_memory.cc - 共享内存
4. test_config_manager.cc - 配置管理器

### 阶段二：服务器功能测试（优先级：高）
5. test_center_server.cc - 中心服务器
6. test_log_server.cc - 日志服务器
7. test_monitor_server.cc - 监控服务器

### 阶段三：核心游戏模块测试（优先级：高）
8. test_role_module.cc - 角色模块
9. test_bag_module.cc - 背包模块
10. test_equip_module.cc - 装备模块

### 阶段四：游戏功能模块测试（优先级：中）
11. test_scene_module.cc - 场景模块
12. test_skill_module.cc - 技能模块
13. test_buff_module.cc - Buff模块

### 阶段五：扩展功能模块测试（优先级：中）
14. test_task_module.cc - 任务模块
15. test_mail_module.cc - 邮件模块
16. test_friend_module.cc - 好友模块

### 阶段六：高级功能模块测试（优先级：低）
17. test_shop_module.cc - 商店模块
18. test_guild_module.cc - 公会模块
19. test_activity_module.cc - 活动模块

---

## 测试执行命令

### 运行所有测试
```bash
cd /root/ancfl/ancfl_gameserver/game_server/build_tests
./bin/game_server_tests
```

### 运行特定测试
```bash
# 运行单个测试套件
./bin/game_server_tests --gtest_filter=CenterServerTest.*

# 运行单个测试用例
./bin/game_server_tests --gtest_filter=CenterServerTest.Init
```

### 生成测试报告
```bash
./bin/game_server_tests --gtest_output=xml:test_results.xml
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
|---------|---------|---------|
|         |         |         |

### 备注
