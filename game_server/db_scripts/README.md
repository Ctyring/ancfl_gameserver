# 数据库初始化脚本使用说明

## 脚本功能

`init_db.sh` 是一个Linux一键初始化脚本，用于：
- 创建游戏服务器所需的所有数据库
- 初始化表结构
- 导入配置数据

## 脚本文件

- `init_db.sh` - 主初始化脚本
- `create_db.sql` - 主数据库创建脚本
- `create_config_db.sql` - 配置数据库创建脚本
- `create_player_data_tables.sql` - 玩家数据表创建脚本
- `insert_config_data.sql` - 配置数据导入脚本

## 使用方法

### 1. 准备工作

1. 确保系统已安装MySQL客户端
2. 确保MySQL服务正在运行
3. 确保有足够的权限创建数据库和表

### 2. 修改脚本参数

编辑 `init_db.sh` 文件，修改以下参数：

```bash
# 数据库连接参数
DB_USER="root"           # MySQL用户名
DB_PASS="password"       # MySQL密码（请修改为实际密码）
DB_HOST="localhost"      # MySQL主机地址
DB_PORT="3306"          # MySQL端口
```

### 3. 执行脚本

```bash
# 给脚本添加执行权限
chmod +x init_db.sh

# 执行脚本
./init_db.sh
```

### 4. 验证结果

脚本执行完成后，会显示初始化结果。你可以通过以下命令验证数据库是否创建成功：

```bash
mysql -u root -p -e "SHOW DATABASES;"
```

应该能看到以下数据库：
- `game_server` - 主数据库
- `game_config` - 配置数据库
- `game_db` - 玩家数据库

## 数据库结构

### 1. game_server 数据库
- 账号相关表（account, account_login_log）
- 角色相关表（role_base, role_property, role_position）
- 核心游戏功能表（bag_item, equip_info, role_skill, role_task, mail, friend_relation, server_list, log_operation）

### 2. game_config 数据库
- Buff配置表（buff_config）
- 任务配置表（task_config, task_condition_config, task_reward_config）
- 技能配置表（skill_config, skill_effect_config）
- 商店配置表（shop_config, shop_item_config）
- 物品配置表（item_config）
- 怪物配置表（monster_config）
- 场景配置表（scene_config）
- 宠物配置表（pet_config, pet_skill_config）
- 坐骑配置表（mount_config, mount_skill_config）
- 翅膀配置表（wing_config, wing_skill_config）
- 称号配置表（title_config）

### 3. game_db 数据库
- 角色技能表（role_skill）
- 角色Buff表（role_buff）
- 角色任务表（role_task, role_task_condition）
- 角色背包表（role_bag）
- 角色装备表（role_equip）
- 角色邮件表（role_mail）
- 角色好友表（role_friend, role_friend_apply）
- 角色公会表（role_guild）
- 角色商店表（role_shop, role_mystery_shop）
- 角色活动表（role_activity）
- 角色场景表（role_scene）
- 角色副本表（role_instance）
- 公会核心表（guild_info, guild_member, guild_skill, guild_warehouse）
- 活动核心表（activity_info, activity_participation）
- 副本核心表（instance_info, instance_stage）
- 交易系统表（auction_house, trade_record）
- 排行榜表（rank_level, rank_power, rank_wealth）
- 成就系统表（achievement_info, achievement_completion）
- 宠物系统表（pet_info, pet_skill）
- 坐骑系统表（mount_info, mount_skill）
- 翅膀系统表（wing_info, wing_skill）
- 称号系统表（title_info, title_achievement）

## 注意事项

1. 脚本会自动创建数据库，无需手动创建
2. 脚本执行过程中会提示执行结果
3. 如果执行失败，请检查MySQL连接参数和权限
4. 脚本仅适用于Linux系统
5. 首次执行时会创建所有表结构和数据，重复执行会覆盖现有数据

## 故障排除

- **无法连接MySQL**：检查MySQL服务是否运行，连接参数是否正确
- **权限不足**：确保MySQL用户有创建数据库和表的权限
- **SQL执行失败**：检查SQL文件是否完整，语法是否正确
- **端口问题**：如果MySQL使用非默认端口，需要修改DB_PORT参数
