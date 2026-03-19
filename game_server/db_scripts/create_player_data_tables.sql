-- 玩家数据表创建脚本
-- 数据库: game_db

-- 创建玩家数据库
CREATE DATABASE IF NOT EXISTS game_db DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

USE game_db;

-- ==================== 角色技能表 ====================
CREATE TABLE IF NOT EXISTS role_skill (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    skill_config_id INT UNSIGNED NOT NULL COMMENT '技能配置ID',
    skill_level INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '技能等级',
    exp INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '技能经验',
    is_active TINYINT NOT NULL DEFAULT 1 COMMENT '是否激活 0:否 1:是',
    slot_index INT NOT NULL DEFAULT -1 COMMENT '快捷栏位置 -1:未设置',
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (id),
    UNIQUE KEY uk_role_skill (role_id, skill_config_id),
    KEY idx_role_id (role_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='角色技能表';

-- ==================== 角色Buff表 ====================
CREATE TABLE IF NOT EXISTS role_buff (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    buff_config_id INT UNSIGNED NOT NULL COMMENT 'Buff配置ID',
    caster_id BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '施法者ID',
    stack_count INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '堆叠层数',
    remaining_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '剩余时间(秒)',
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    expire_time TIMESTAMP NOT NULL COMMENT '过期时间',
    PRIMARY KEY (id),
    KEY idx_role_id (role_id),
    KEY idx_expire_time (expire_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='角色Buff表';

-- ==================== 角色任务表 ====================
CREATE TABLE IF NOT EXISTS role_task (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    task_config_id INT UNSIGNED NOT NULL COMMENT '任务配置ID',
    status INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '状态 1:未接取 2:进行中 3:已完成 4:已提交',
    accept_time TIMESTAMP NULL DEFAULT NULL COMMENT '接取时间',
    complete_time TIMESTAMP NULL DEFAULT NULL COMMENT '完成时间',
    submit_time TIMESTAMP NULL DEFAULT NULL COMMENT '提交时间',
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (id),
    UNIQUE KEY uk_role_task (role_id, task_config_id),
    KEY idx_role_id (role_id),
    KEY idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='角色任务表';

-- 任务条件进度表
CREATE TABLE IF NOT EXISTS role_task_condition (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    task_id BIGINT UNSIGNED NOT NULL COMMENT '任务ID',
    condition_type INT UNSIGNED NOT NULL COMMENT '条件类型',
    target_id INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '目标ID',
    target_count INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '目标数量',
    current_count INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '当前进度',
    PRIMARY KEY (id),
    UNIQUE KEY uk_role_task_condition (role_id, task_id, condition_type, target_id),
    KEY idx_role_id (role_id),
    KEY idx_task_id (task_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='任务条件进度表';

-- ==================== 角色背包表 ====================
CREATE TABLE IF NOT EXISTS role_bag (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    item_config_id INT UNSIGNED NOT NULL COMMENT '物品配置ID',
    item_count INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '物品数量',
    slot_index INT UNSIGNED NOT NULL COMMENT '格子位置',
    is_bind TINYINT NOT NULL DEFAULT 0 COMMENT '是否绑定 0:否 1:是',
    extra_data TEXT COMMENT '额外数据 JSON格式',
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (id),
    UNIQUE KEY uk_role_slot (role_id, slot_index),
    KEY idx_role_id (role_id),
    KEY idx_item_id (item_config_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='角色背包表';

-- ==================== 角色装备表 ====================
CREATE TABLE IF NOT EXISTS role_equip (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    item_config_id INT UNSIGNED NOT NULL COMMENT '装备配置ID',
    equip_slot INT UNSIGNED NOT NULL COMMENT '装备槽位 1:武器 2:头盔 3:衣服 4:裤子 5:鞋子 6:手套 7:项链 8:戒指 9:腰带 10:饰品',
    enhance_level INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '强化等级',
    star_level INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '升星等级',
    gem_slot_1 INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '宝石槽1',
    gem_slot_2 INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '宝石槽2',
    gem_slot_3 INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '宝石槽3',
    extra_attr TEXT COMMENT '额外属性 JSON格式',
    is_bind TINYINT NOT NULL DEFAULT 0 COMMENT '是否绑定 0:否 1:是',
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (id),
    UNIQUE KEY uk_role_slot (role_id, equip_slot),
    KEY idx_role_id (role_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='角色装备表';

-- ==================== 角色邮件表 ====================
CREATE TABLE IF NOT EXISTS role_mail (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    sender_id BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '发送者ID 0:系统',
    sender_name VARCHAR(64) NOT NULL DEFAULT '系统' COMMENT '发送者名称',
    title VARCHAR(128) NOT NULL DEFAULT '' COMMENT '邮件标题',
    content TEXT COMMENT '邮件内容',
    attachment TEXT COMMENT '附件 JSON格式',
    is_read TINYINT NOT NULL DEFAULT 0 COMMENT '是否已读 0:否 1:是',
    is_received TINYINT NOT NULL DEFAULT 0 COMMENT '附件是否已领取 0:否 1:是',
    mail_type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '邮件类型 1:普通 2:系统 3:活动',
    expire_time TIMESTAMP NOT NULL COMMENT '过期时间',
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    PRIMARY KEY (id),
    KEY idx_role_id (role_id),
    KEY idx_expire_time (expire_time),
    KEY idx_is_read (is_read)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='角色邮件表';

-- ==================== 角色好友表 ====================
CREATE TABLE IF NOT EXISTS role_friend (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    friend_id BIGINT UNSIGNED NOT NULL COMMENT '好友ID',
    friend_name VARCHAR(64) NOT NULL DEFAULT '' COMMENT '好友名称',
    intimacy INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '亲密度',
    is_blacklist TINYINT NOT NULL DEFAULT 0 COMMENT '是否黑名单 0:否 1:是',
    remark VARCHAR(64) NOT NULL DEFAULT '' COMMENT '备注',
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (id),
    UNIQUE KEY uk_role_friend (role_id, friend_id),
    KEY idx_role_id (role_id),
    KEY idx_friend_id (friend_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='角色好友表';

-- 好友申请记录表
CREATE TABLE IF NOT EXISTS role_friend_apply (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '申请人ID',
    target_id BIGINT UNSIGNED NOT NULL COMMENT '目标ID',
    message VARCHAR(256) NOT NULL DEFAULT '' COMMENT '申请消息',
    status INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '状态 1:待处理 2:已同意 3:已拒绝',
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    PRIMARY KEY (id),
    UNIQUE KEY uk_apply (role_id, target_id),
    KEY idx_target_id (target_id),
    KEY idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='好友申请记录表';

-- ==================== 角色公会表 ====================
CREATE TABLE IF NOT EXISTS role_guild (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    guild_id BIGINT UNSIGNED NOT NULL COMMENT '公会ID',
    position INT UNSIGNED NOT NULL DEFAULT 3 COMMENT '职位 1:会长 2:副会长 3:成员',
    contribution INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '贡献值',
    total_contribution INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '总贡献值',
    join_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '加入时间',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (id),
    UNIQUE KEY uk_role_guild (role_id),
    KEY idx_guild_id (guild_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='角色公会表';

-- ==================== 角色商店数据表 ====================
CREATE TABLE IF NOT EXISTS role_shop (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    shop_id INT UNSIGNED NOT NULL COMMENT '商店ID',
    item_config_id INT UNSIGNED NOT NULL COMMENT '物品配置ID',
    buy_count INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '已购买数量',
    reset_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '重置时间',
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (id),
    UNIQUE KEY uk_role_shop_item (role_id, shop_id, item_config_id),
    KEY idx_role_id (role_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='角色商店数据表';

-- 神秘商店数据表
CREATE TABLE IF NOT EXISTS role_mystery_shop (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    shop_id INT UNSIGNED NOT NULL COMMENT '商店ID',
    slot_index INT UNSIGNED NOT NULL COMMENT '槽位索引',
    item_config_id INT UNSIGNED NOT NULL COMMENT '物品配置ID',
    item_count INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '物品数量',
    price_type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '货币类型',
    price INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '价格',
    discount INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '折扣',
    is_bought TINYINT NOT NULL DEFAULT 0 COMMENT '是否已购买 0:否 1:是',
    expire_time TIMESTAMP NOT NULL COMMENT '过期时间',
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    PRIMARY KEY (id),
    UNIQUE KEY uk_role_shop_slot (role_id, shop_id, slot_index),
    KEY idx_role_id (role_id),
    KEY idx_expire_time (expire_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='神秘商店数据表';

-- ==================== 角色活动数据表 ====================
CREATE TABLE IF NOT EXISTS role_activity (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    activity_id INT UNSIGNED NOT NULL COMMENT '活动ID',
    progress INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '进度',
    is_completed TINYINT NOT NULL DEFAULT 0 COMMENT '是否完成 0:否 1:是',
    is_received TINYINT NOT NULL DEFAULT 0 COMMENT '奖励是否已领取 0:否 1:是',
    complete_time TIMESTAMP NULL DEFAULT NULL COMMENT '完成时间',
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (id),
    UNIQUE KEY uk_role_activity (role_id, activity_id),
    KEY idx_role_id (role_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='角色活动数据表';

-- ==================== 角色场景数据表 ====================
CREATE TABLE IF NOT EXISTS role_scene (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    scene_id INT UNSIGNED NOT NULL COMMENT '场景ID',
    pos_x FLOAT NOT NULL DEFAULT 0 COMMENT 'X坐标',
    pos_y FLOAT NOT NULL DEFAULT 0 COMMENT 'Y坐标',
    pos_z FLOAT NOT NULL DEFAULT 0 COMMENT 'Z坐标',
    direction FLOAT NOT NULL DEFAULT 0 COMMENT '朝向',
    is_entered TINYINT NOT NULL DEFAULT 0 COMMENT '是否已进入场景 0:否 1:是',
    enter_time TIMESTAMP NULL DEFAULT NULL COMMENT '进入时间',
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (id),
    UNIQUE KEY uk_role_scene (role_id),
    KEY idx_scene_id (scene_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='角色场景数据表';

-- ==================== 角色副本数据表 ====================
CREATE TABLE IF NOT EXISTS role_instance (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    instance_id INT UNSIGNED NOT NULL COMMENT '副本ID',
    clear_count INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '通关次数',
    today_count INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '今日次数',
    last_enter_time TIMESTAMP NULL DEFAULT NULL COMMENT '最后进入时间',
    best_clear_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '最佳通关时间(秒)',
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (id),
    UNIQUE KEY uk_role_instance (role_id, instance_id),
    KEY idx_role_id (role_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='角色副本数据表';

-- ==================== 公会核心表 ====================

-- 公会信息表
CREATE TABLE IF NOT EXISTS guild_info (
    guild_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '公会ID',
    guild_name VARCHAR(64) NOT NULL COMMENT '公会名称',
    leader_id BIGINT UNSIGNED NOT NULL COMMENT '会长ID',
    leader_name VARCHAR(64) NOT NULL COMMENT '会长名称',
    level INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '公会等级',
    exp INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '公会经验',
    members_count INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '成员数量',
    max_members INT UNSIGNED NOT NULL DEFAULT 50 COMMENT '最大成员数',
    notice VARCHAR(512) NOT NULL DEFAULT '' COMMENT '公会公告',
    join_requirement INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '加入要求',
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (guild_id),
    UNIQUE KEY uk_guild_name (guild_name),
    KEY idx_leader_id (leader_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='公会信息表';

-- 公会成员表
CREATE TABLE IF NOT EXISTS guild_member (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    guild_id BIGINT UNSIGNED NOT NULL COMMENT '公会ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    role_name VARCHAR(64) NOT NULL COMMENT '角色名称',
    position INT UNSIGNED NOT NULL DEFAULT 3 COMMENT '职位 1:会长 2:副会长 3:成员',
    contribution INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '贡献值',
    total_contribution INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '总贡献值',
    join_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '加入时间',
    last_active_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '最后活跃时间',
    PRIMARY KEY (id),
    UNIQUE KEY uk_guild_role (guild_id, role_id),
    KEY idx_guild_id (guild_id),
    KEY idx_role_id (role_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='公会成员表';

-- 公会技能表
CREATE TABLE IF NOT EXISTS guild_skill (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    guild_id BIGINT UNSIGNED NOT NULL COMMENT '公会ID',
    skill_id INT UNSIGNED NOT NULL COMMENT '技能ID',
    skill_level INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '技能等级',
    exp INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '技能经验',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (id),
    UNIQUE KEY uk_guild_skill (guild_id, skill_id),
    KEY idx_guild_id (guild_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='公会技能表';

-- 公会仓库表
CREATE TABLE IF NOT EXISTS guild_warehouse (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    guild_id BIGINT UNSIGNED NOT NULL COMMENT '公会ID',
    item_config_id INT UNSIGNED NOT NULL COMMENT '物品配置ID',
    item_count INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '物品数量',
    contributor_id BIGINT UNSIGNED NOT NULL COMMENT '贡献者ID',
    contribute_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '贡献时间',
    PRIMARY KEY (id),
    KEY idx_guild_id (guild_id),
    KEY idx_item_id (item_config_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='公会仓库表';

-- ==================== 活动核心表 ====================

-- 活动信息表
CREATE TABLE IF NOT EXISTS activity_info (
    activity_id INT UNSIGNED NOT NULL COMMENT '活动ID',
    activity_name VARCHAR(128) NOT NULL COMMENT '活动名称',
    activity_type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '活动类型',
    start_time TIMESTAMP NOT NULL COMMENT '开始时间',
    end_time TIMESTAMP NOT NULL COMMENT '结束时间',
    status INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '状态 1:未开始 2:进行中 3:已结束',
    config TEXT COMMENT '活动配置 JSON格式',
    description TEXT COMMENT '活动描述',
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (activity_id),
    KEY idx_activity_type (activity_type),
    KEY idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='活动信息表';

-- 活动参与记录表
CREATE TABLE IF NOT EXISTS activity_participation (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    activity_id INT UNSIGNED NOT NULL COMMENT '活动ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    progress INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '活动进度',
    is_completed TINYINT NOT NULL DEFAULT 0 COMMENT '是否完成 0:否 1:是',
    is_rewarded TINYINT NOT NULL DEFAULT 0 COMMENT '是否领取奖励 0:否 1:是',
    participate_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '参与时间',
    complete_time TIMESTAMP NULL DEFAULT NULL COMMENT '完成时间',
    PRIMARY KEY (id),
    UNIQUE KEY uk_activity_role (activity_id, role_id),
    KEY idx_activity_id (activity_id),
    KEY idx_role_id (role_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='活动参与记录表';

-- ==================== 副本核心表 ====================

-- 副本信息表
CREATE TABLE IF NOT EXISTS instance_info (
    instance_id INT UNSIGNED NOT NULL COMMENT '副本ID',
    instance_name VARCHAR(128) NOT NULL COMMENT '副本名称',
    type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '副本类型',
    level_requirement INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '等级要求',
    recommend_power INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '推荐战力',
    max_players INT UNSIGNED NOT NULL DEFAULT 5 COMMENT '最大玩家数',
    reset_time INT UNSIGNED NOT NULL DEFAULT 86400 COMMENT '重置时间(秒)',
    daily_limit INT UNSIGNED NOT NULL DEFAULT 3 COMMENT '每日限制次数',
    entry_cost TEXT COMMENT '进入消耗 JSON格式',
    drop_rewards TEXT COMMENT '掉落奖励 JSON格式',
    description TEXT COMMENT '副本描述',
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (instance_id),
    KEY idx_type (type),
    KEY idx_level_requirement (level_requirement)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='副本信息表';

-- 副本关卡表
CREATE TABLE IF NOT EXISTS instance_stage (
    stage_id INT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '关卡ID',
    instance_id INT UNSIGNED NOT NULL COMMENT '副本ID',
    stage_name VARCHAR(128) NOT NULL COMMENT '关卡名称',
    stage_order INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '关卡顺序',
    monster_waves TEXT COMMENT '怪物波次 JSON格式',
    boss_id INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'BOSS ID',
    time_limit INT UNSIGNED NOT NULL DEFAULT 600 COMMENT '时间限制(秒)',
    rewards TEXT COMMENT '关卡奖励 JSON格式',
    PRIMARY KEY (stage_id),
    KEY idx_instance_id (instance_id),
    KEY idx_stage_order (stage_order)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='副本关卡表';

-- ==================== 交易系统表 ====================

-- 拍卖行表
CREATE TABLE IF NOT EXISTS auction_house (
    auction_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '拍卖ID',
    seller_id BIGINT UNSIGNED NOT NULL COMMENT '卖家ID',
    seller_name VARCHAR(64) NOT NULL COMMENT '卖家名称',
    item_config_id INT UNSIGNED NOT NULL COMMENT '物品配置ID',
    item_count INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '物品数量',
    starting_price INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '起拍价',
    current_price INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '当前价格',
    buyout_price INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '一口价',
    bidder_id BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '当前出价者ID',
    bidder_name VARCHAR(64) NOT NULL DEFAULT '' COMMENT '当前出价者名称',
    start_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '开始时间',
    end_time TIMESTAMP NOT NULL COMMENT '结束时间',
    status INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '状态 1:拍卖中 2:已成交 3:流拍',
    PRIMARY KEY (auction_id),
    KEY idx_seller_id (seller_id),
    KEY idx_item_id (item_config_id),
    KEY idx_status (status),
    KEY idx_end_time (end_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='拍卖行表';

-- 交易记录表
CREATE TABLE IF NOT EXISTS trade_record (
    trade_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '交易ID',
    buyer_id BIGINT UNSIGNED NOT NULL COMMENT '买家ID',
    buyer_name VARCHAR(64) NOT NULL COMMENT '买家名称',
    seller_id BIGINT UNSIGNED NOT NULL COMMENT '卖家ID',
    seller_name VARCHAR(64) NOT NULL COMMENT '卖家名称',
    item_config_id INT UNSIGNED NOT NULL COMMENT '物品配置ID',
    item_count INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '物品数量',
    price INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '交易价格',
    trade_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '交易时间',
    trade_type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '交易类型 1:拍卖行 2:直接交易',
    PRIMARY KEY (trade_id),
    KEY idx_buyer_id (buyer_id),
    KEY idx_seller_id (seller_id),
    KEY idx_trade_time (trade_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='交易记录表';

-- ==================== 排行榜表 ====================

-- 等级排行榜
CREATE TABLE IF NOT EXISTS rank_level (
    rank_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '排名ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    role_name VARCHAR(64) NOT NULL COMMENT '角色名称',
    level INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '等级',
    exp BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '经验值',
    rank INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '排名',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (rank_id),
    UNIQUE KEY uk_role_id (role_id),
    KEY idx_rank (rank),
    KEY idx_level (level)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='等级排行榜';

-- 战力排行榜
CREATE TABLE IF NOT EXISTS rank_power (
    rank_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '排名ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    role_name VARCHAR(64) NOT NULL COMMENT '角色名称',
    power INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '战力',
    rank INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '排名',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (rank_id),
    UNIQUE KEY uk_role_id (role_id),
    KEY idx_rank (rank),
    KEY idx_power (power)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='战力排行榜';

-- 财富排行榜
CREATE TABLE IF NOT EXISTS rank_wealth (
    rank_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '排名ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    role_name VARCHAR(64) NOT NULL COMMENT '角色名称',
    wealth INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '财富值',
    rank INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '排名',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (rank_id),
    UNIQUE KEY uk_role_id (role_id),
    KEY idx_rank (rank),
    KEY idx_wealth (wealth)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='财富排行榜';

-- ==================== 成就系统表 ====================

-- 成就信息表
CREATE TABLE IF NOT EXISTS achievement_info (
    achievement_id INT UNSIGNED NOT NULL COMMENT '成就ID',
    achievement_name VARCHAR(128) NOT NULL COMMENT '成就名称',
    achievement_type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '成就类型',
    target_count INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '目标数量',
    rewards TEXT COMMENT '奖励 JSON格式',
    description TEXT COMMENT '成就描述',
    icon VARCHAR(128) NOT NULL DEFAULT '' COMMENT '成就图标',
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    PRIMARY KEY (achievement_id),
    KEY idx_achievement_type (achievement_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='成就信息表';

-- 成就完成记录表
CREATE TABLE IF NOT EXISTS achievement_completion (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    achievement_id INT UNSIGNED NOT NULL COMMENT '成就ID',
    current_count INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '当前进度',
    is_completed TINYINT NOT NULL DEFAULT 0 COMMENT '是否完成 0:否 1:是',
    is_rewarded TINYINT NOT NULL DEFAULT 0 COMMENT '是否领取奖励 0:否 1:是',
    complete_time TIMESTAMP NULL DEFAULT NULL COMMENT '完成时间',
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (id),
    UNIQUE KEY uk_role_achievement (role_id, achievement_id),
    KEY idx_role_id (role_id),
    KEY idx_achievement_id (achievement_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='成就完成记录表';

-- ==================== 宠物系统表 ====================

-- 宠物信息表
CREATE TABLE IF NOT EXISTS pet_info (
    pet_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '宠物ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    pet_config_id INT UNSIGNED NOT NULL COMMENT '宠物配置ID',
    pet_name VARCHAR(64) NOT NULL COMMENT '宠物名称',
    level INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '宠物等级',
    exp INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '宠物经验',
    intimacy INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '亲密度',
    is_active TINYINT NOT NULL DEFAULT 0 COMMENT '是否激活 0:否 1:是',
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (pet_id),
    KEY idx_role_id (role_id),
    KEY idx_pet_config_id (pet_config_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='宠物信息表';

-- 宠物技能表
CREATE TABLE IF NOT EXISTS pet_skill (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    pet_id BIGINT UNSIGNED NOT NULL COMMENT '宠物ID',
    skill_id INT UNSIGNED NOT NULL COMMENT '技能ID',
    skill_level INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '技能等级',
    exp INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '技能经验',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (id),
    UNIQUE KEY uk_pet_skill (pet_id, skill_id),
    KEY idx_pet_id (pet_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='宠物技能表';

-- ==================== 坐骑系统表 ====================

-- 坐骑信息表
CREATE TABLE IF NOT EXISTS mount_info (
    mount_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '坐骑ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    mount_config_id INT UNSIGNED NOT NULL COMMENT '坐骑配置ID',
    level INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '坐骑等级',
    exp INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '坐骑经验',
    is_active TINYINT NOT NULL DEFAULT 0 COMMENT '是否激活 0:否 1:是',
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (mount_id),
    KEY idx_role_id (role_id),
    KEY idx_mount_config_id (mount_config_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='坐骑信息表';

-- 坐骑技能表
CREATE TABLE IF NOT EXISTS mount_skill (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    mount_id BIGINT UNSIGNED NOT NULL COMMENT '坐骑ID',
    skill_id INT UNSIGNED NOT NULL COMMENT '技能ID',
    skill_level INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '技能等级',
    exp INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '技能经验',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (id),
    UNIQUE KEY uk_mount_skill (mount_id, skill_id),
    KEY idx_mount_id (mount_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='坐骑技能表';

-- ==================== 翅膀系统表 ====================

-- 翅膀信息表
CREATE TABLE IF NOT EXISTS wing_info (
    wing_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '翅膀ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    wing_config_id INT UNSIGNED NOT NULL COMMENT '翅膀配置ID',
    level INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '翅膀等级',
    exp INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '翅膀经验',
    is_active TINYINT NOT NULL DEFAULT 0 COMMENT '是否激活 0:否 1:是',
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (wing_id),
    KEY idx_role_id (role_id),
    KEY idx_wing_config_id (wing_config_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='翅膀信息表';

-- 翅膀技能表
CREATE TABLE IF NOT EXISTS wing_skill (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    wing_id BIGINT UNSIGNED NOT NULL COMMENT '翅膀ID',
    skill_id INT UNSIGNED NOT NULL COMMENT '技能ID',
    skill_level INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '技能等级',
    exp INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '技能经验',
    update_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    PRIMARY KEY (id),
    UNIQUE KEY uk_wing_skill (wing_id, skill_id),
    KEY idx_wing_id (wing_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='翅膀技能表';

-- ==================== 称号系统表 ====================

-- 称号信息表
CREATE TABLE IF NOT EXISTS title_info (
    title_id INT UNSIGNED NOT NULL COMMENT '称号ID',
    title_name VARCHAR(64) NOT NULL COMMENT '称号名称',
    title_type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '称号类型',
    effects TEXT COMMENT '称号效果 JSON格式',
    duration INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '持续时间(秒) 0:永久',
    description TEXT COMMENT '称号描述',
    icon VARCHAR(128) NOT NULL DEFAULT '' COMMENT '称号图标',
    create_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    PRIMARY KEY (title_id),
    KEY idx_title_type (title_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='称号信息表';

-- 称号获得记录表
CREATE TABLE IF NOT EXISTS title_achievement (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    title_id INT UNSIGNED NOT NULL COMMENT '称号ID',
    obtain_time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '获得时间',
    expire_time TIMESTAMP NULL DEFAULT NULL COMMENT '过期时间',
    is_equipped TINYINT NOT NULL DEFAULT 0 COMMENT '是否装备 0:否 1:是',
    PRIMARY KEY (id),
    UNIQUE KEY uk_role_title (role_id, title_id),
    KEY idx_role_id (role_id),
    KEY idx_title_id (title_id),
    KEY idx_expire_time (expire_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='称号获得记录表';
