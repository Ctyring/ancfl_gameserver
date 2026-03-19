-- 游戏配置数据库创建脚本
-- 数据库: game_config

-- 创建配置数据库
CREATE DATABASE IF NOT EXISTS game_config DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

USE game_config;

-- ==================== Buff配置表 ====================
CREATE TABLE IF NOT EXISTS buff_config (
    id INT UNSIGNED NOT NULL COMMENT 'Buff配置ID',
    name VARCHAR(64) NOT NULL DEFAULT '' COMMENT 'Buff名称',
    type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT 'Buff类型 1:增益 2:减益 3:持续伤害 4:持续治疗 5:控制 6:护盾',
    effect_type INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '效果类型 1:攻击提升 2:攻击降低 3:防御提升 4:防御降低 5:速度提升 6:速度降低 7:生命恢复 8:法力恢复 9:眩晕 10:沉默 11:中毒 12:燃烧 13:冰冻 14:护盾',
    effect_value INT NOT NULL DEFAULT 0 COMMENT '效果值',
    duration INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '持续时间(秒)',
    trigger_interval INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '触发间隔(秒)',
    max_stack INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '最大堆叠数',
    is_debuff TINYINT NOT NULL DEFAULT 0 COMMENT '是否减益 0:否 1:是',
    can_dispel TINYINT NOT NULL DEFAULT 1 COMMENT '是否可驱散 0:否 1:是',
    description VARCHAR(256) NOT NULL DEFAULT '' COMMENT 'Buff描述',
    icon VARCHAR(128) NOT NULL DEFAULT '' COMMENT '图标路径',
    PRIMARY KEY (id),
    KEY idx_type (type),
    KEY idx_effect_type (effect_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Buff配置表';

-- ==================== 任务配置表 ====================
CREATE TABLE IF NOT EXISTS task_config (
    id INT UNSIGNED NOT NULL COMMENT '任务配置ID',
    name VARCHAR(64) NOT NULL DEFAULT '' COMMENT '任务名称',
    type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '任务类型 1:主线 2:支线 3:日常 4:周常 5:成就',
    level_requirement INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '等级要求',
    pre_task_id INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '前置任务ID',
    description VARCHAR(512) NOT NULL DEFAULT '' COMMENT '任务描述',
    accept_npc INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '接取NPC',
    submit_npc INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '提交NPC',
    PRIMARY KEY (id),
    KEY idx_type (type),
    KEY idx_level (level_requirement),
    KEY idx_pre_task (pre_task_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='任务配置表';

-- 任务条件配置表
CREATE TABLE IF NOT EXISTS task_condition_config (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    task_id INT UNSIGNED NOT NULL COMMENT '任务ID',
    condition_type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '条件类型 1:击杀怪物 2:收集物品 3:与NPC对话 4:达到等级 5:到达位置 6:完成副本 7:穿戴装备 8:技能等级 9:击杀玩家',
    target_id INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '目标ID',
    target_count INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '目标数量',
    description VARCHAR(256) NOT NULL DEFAULT '' COMMENT '条件描述',
    PRIMARY KEY (id),
    KEY idx_task_id (task_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='任务条件配置表';

-- 任务奖励配置表
CREATE TABLE IF NOT EXISTS task_reward_config (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    task_id INT UNSIGNED NOT NULL COMMENT '任务ID',
    reward_type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '奖励类型 1:经验 2:金币 3:钻石 4:物品 5:声望 6:荣誉',
    reward_id INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '奖励ID(物品ID)',
    reward_count INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '奖励数量',
    PRIMARY KEY (id),
    KEY idx_task_id (task_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='任务奖励配置表';

-- ==================== 技能配置表 ====================
CREATE TABLE IF NOT EXISTS skill_config (
    id INT UNSIGNED NOT NULL COMMENT '技能配置ID',
    name VARCHAR(64) NOT NULL DEFAULT '' COMMENT '技能名称',
    type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '技能类型 1:主动 2:被动 3:Buff 4:Debuff 5:治疗 6:控制',
    target_type INT UNSIGNED NOT NULL DEFAULT 2 COMMENT '目标类型 1:自身 2:单体 3:范围 4:直线 5:扇形',
    cast_range INT UNSIGNED NOT NULL DEFAULT 5 COMMENT '施法范围',
    aoe_radius INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'AOE半径',
    cooldown INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '冷却时间(秒)',
    mp_cost INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '法力消耗',
    damage INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '伤害值',
    heal INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '治疗值',
    description VARCHAR(512) NOT NULL DEFAULT '' COMMENT '技能描述',
    icon VARCHAR(128) NOT NULL DEFAULT '' COMMENT '图标路径',
    PRIMARY KEY (id),
    KEY idx_type (type),
    KEY idx_target_type (target_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='技能配置表';

-- 技能效果配置表
CREATE TABLE IF NOT EXISTS skill_effect_config (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    skill_id INT UNSIGNED NOT NULL COMMENT '技能ID',
    effect_type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '效果类型',
    effect_value INT NOT NULL DEFAULT 0 COMMENT '效果值',
    duration INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '持续时间(秒)',
    trigger_interval INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '触发间隔(秒)',
    PRIMARY KEY (id),
    KEY idx_skill_id (skill_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='技能效果配置表';

-- ==================== 商店配置表 ====================
CREATE TABLE IF NOT EXISTS shop_config (
    id INT UNSIGNED NOT NULL COMMENT '商店ID',
    name VARCHAR(64) NOT NULL DEFAULT '' COMMENT '商店名称',
    type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '商店类型 1:普通 2:神秘 3:公会 4:PVP 5:活动',
    refresh_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '自动刷新时间(秒) 0:不自动刷新',
    refresh_cost INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '手动刷新消耗',
    is_open TINYINT NOT NULL DEFAULT 1 COMMENT '是否开放 0:关闭 1:开放',
    description VARCHAR(256) NOT NULL DEFAULT '' COMMENT '商店描述',
    PRIMARY KEY (id),
    KEY idx_type (type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='商店配置表';

-- 商店商品配置表
CREATE TABLE IF NOT EXISTS shop_item_config (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'ID',
    shop_id INT UNSIGNED NOT NULL COMMENT '商店ID',
    item_config_id INT UNSIGNED NOT NULL COMMENT '物品配置ID',
    item_count INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '物品数量',
    price_type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '货币类型 1:金币 2:钻石 3:公会币 4:荣誉 5:积分',
    price INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '价格',
    discount INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '折扣 0-100',
    limit_count INT UNSIGNED NOT NULL DEFAULT 999 COMMENT '限购数量',
    require_level INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '等级要求',
    require_vip INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'VIP等级要求',
    sort_order INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '排序',
    PRIMARY KEY (id),
    KEY idx_shop_id (shop_id),
    KEY idx_item_id (item_config_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='商店商品配置表';

-- ==================== 物品配置表 ====================
CREATE TABLE IF NOT EXISTS item_config (
    id INT UNSIGNED NOT NULL COMMENT '物品配置ID',
    name VARCHAR(64) NOT NULL DEFAULT '' COMMENT '物品名称',
    type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '物品类型 1:消耗品 2:装备 3:材料 4:宝石 5:任务物品',
    quality INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '品质 1:普通 2:优秀 3:精良 4:史诗 5:传说',
    max_stack INT UNSIGNED NOT NULL DEFAULT 99 COMMENT '最大堆叠数',
    is_bind TINYINT NOT NULL DEFAULT 0 COMMENT '是否绑定 0:否 1:是',
    description VARCHAR(512) NOT NULL DEFAULT '' COMMENT '物品描述',
    icon VARCHAR(128) NOT NULL DEFAULT '' COMMENT '图标路径',
    PRIMARY KEY (id),
    KEY idx_type (type),
    KEY idx_quality (quality)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='物品配置表';

-- ==================== 怪物配置表 ====================
CREATE TABLE IF NOT EXISTS monster_config (
    id INT UNSIGNED NOT NULL COMMENT '怪物配置ID',
    name VARCHAR(64) NOT NULL DEFAULT '' COMMENT '怪物名称',
    level INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '等级',
    type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '类型 1:普通 2:精英 3:BOSS',
    hp INT UNSIGNED NOT NULL DEFAULT 100 COMMENT '血量',
    mp INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '法力',
    attack INT UNSIGNED NOT NULL DEFAULT 10 COMMENT '攻击',
    defense INT UNSIGNED NOT NULL DEFAULT 5 COMMENT '防御',
    speed INT UNSIGNED NOT NULL DEFAULT 100 COMMENT '速度',
    exp INT UNSIGNED NOT NULL DEFAULT 10 COMMENT '经验值',
    drop_items VARCHAR(512) NOT NULL DEFAULT '' COMMENT '掉落物品 JSON格式',
    PRIMARY KEY (id),
    KEY idx_level (level),
    KEY idx_type (type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='怪物配置表';

-- ==================== 场景配置表 ====================
CREATE TABLE IF NOT EXISTS scene_config (
    id INT UNSIGNED NOT NULL COMMENT '场景ID',
    name VARCHAR(64) NOT NULL DEFAULT '' COMMENT '场景名称',
    type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '场景类型 1:主城 2:野外 3:副本 4:战场',
    max_player INT UNSIGNED NOT NULL DEFAULT 100 COMMENT '最大玩家数',
    pk_mode INT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'PK模式 0:安全区 1:自由PK 2:阵营PK',
    enter_level INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '进入等级',
    description VARCHAR(256) NOT NULL DEFAULT '' COMMENT '场景描述',
    PRIMARY KEY (id),
    KEY idx_type (type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='场景配置表';

-- ==================== 宠物配置表 ====================
CREATE TABLE IF NOT EXISTS pet_config (
    id INT UNSIGNED NOT NULL COMMENT '宠物配置ID',
    name VARCHAR(64) NOT NULL DEFAULT '' COMMENT '宠物名称',
    type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '宠物类型',
    quality INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '品质 1:普通 2:优秀 3:精良 4:史诗 5:传说',
    hp INT UNSIGNED NOT NULL DEFAULT 100 COMMENT '血量',
    attack INT UNSIGNED NOT NULL DEFAULT 10 COMMENT '攻击',
    defense INT UNSIGNED NOT NULL DEFAULT 5 COMMENT '防御',
    speed INT UNSIGNED NOT NULL DEFAULT 100 COMMENT '速度',
    skill_slots INT UNSIGNED NOT NULL DEFAULT 3 COMMENT '技能槽位',
    growth_rate FLOAT NOT NULL DEFAULT 1.0 COMMENT '成长率',
    obtain_way VARCHAR(256) NOT NULL DEFAULT '' COMMENT '获取方式',
    description VARCHAR(512) NOT NULL DEFAULT '' COMMENT '宠物描述',
    icon VARCHAR(128) NOT NULL DEFAULT '' COMMENT '图标路径',
    PRIMARY KEY (id),
    KEY idx_type (type),
    KEY idx_quality (quality)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='宠物配置表';

-- 宠物技能配置表
CREATE TABLE IF NOT EXISTS pet_skill_config (
    id INT UNSIGNED NOT NULL COMMENT '宠物技能配置ID',
    name VARCHAR(64) NOT NULL DEFAULT '' COMMENT '技能名称',
    type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '技能类型',
    effect_type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '效果类型',
    effect_value INT NOT NULL DEFAULT 0 COMMENT '效果值',
    cooldown INT UNSIGNED NOT NULL DEFAULT 10 COMMENT '冷却时间(秒)',
    duration INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '持续时间(秒)',
    description VARCHAR(256) NOT NULL DEFAULT '' COMMENT '技能描述',
    icon VARCHAR(128) NOT NULL DEFAULT '' COMMENT '图标路径',
    PRIMARY KEY (id),
    KEY idx_type (type),
    KEY idx_effect_type (effect_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='宠物技能配置表';

-- ==================== 坐骑配置表 ====================
CREATE TABLE IF NOT EXISTS mount_config (
    id INT UNSIGNED NOT NULL COMMENT '坐骑配置ID',
    name VARCHAR(64) NOT NULL DEFAULT '' COMMENT '坐骑名称',
    type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '坐骑类型',
    quality INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '品质 1:普通 2:优秀 3:精良 4:史诗 5:传说',
    speed_bonus INT UNSIGNED NOT NULL DEFAULT 100 COMMENT '速度加成',
    hp_bonus INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '生命加成',
    attack_bonus INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '攻击加成',
    defense_bonus INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '防御加成',
    skill_slots INT UNSIGNED NOT NULL DEFAULT 2 COMMENT '技能槽位',
    obtain_way VARCHAR(256) NOT NULL DEFAULT '' COMMENT '获取方式',
    description VARCHAR(512) NOT NULL DEFAULT '' COMMENT '坐骑描述',
    icon VARCHAR(128) NOT NULL DEFAULT '' COMMENT '图标路径',
    model VARCHAR(128) NOT NULL DEFAULT '' COMMENT '模型路径',
    PRIMARY KEY (id),
    KEY idx_type (type),
    KEY idx_quality (quality)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='坐骑配置表';

-- 坐骑技能配置表
CREATE TABLE IF NOT EXISTS mount_skill_config (
    id INT UNSIGNED NOT NULL COMMENT '坐骑技能配置ID',
    name VARCHAR(64) NOT NULL DEFAULT '' COMMENT '技能名称',
    type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '技能类型',
    effect_type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '效果类型',
    effect_value INT NOT NULL DEFAULT 0 COMMENT '效果值',
    cooldown INT UNSIGNED NOT NULL DEFAULT 30 COMMENT '冷却时间(秒)',
    duration INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '持续时间(秒)',
    description VARCHAR(256) NOT NULL DEFAULT '' COMMENT '技能描述',
    icon VARCHAR(128) NOT NULL DEFAULT '' COMMENT '图标路径',
    PRIMARY KEY (id),
    KEY idx_type (type),
    KEY idx_effect_type (effect_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='坐骑技能配置表';

-- ==================== 翅膀配置表 ====================
CREATE TABLE IF NOT EXISTS wing_config (
    id INT UNSIGNED NOT NULL COMMENT '翅膀配置ID',
    name VARCHAR(64) NOT NULL DEFAULT '' COMMENT '翅膀名称',
    type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '翅膀类型',
    quality INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '品质 1:普通 2:优秀 3:精良 4:史诗 5:传说',
    hp_bonus INT UNSIGNED NOT NULL DEFAULT 100 COMMENT '生命加成',
    attack_bonus INT UNSIGNED NOT NULL DEFAULT 50 COMMENT '攻击加成',
    defense_bonus INT UNSIGNED NOT NULL DEFAULT 30 COMMENT '防御加成',
    speed_bonus INT UNSIGNED NOT NULL DEFAULT 20 COMMENT '速度加成',
    skill_slots INT UNSIGNED NOT NULL DEFAULT 2 COMMENT '技能槽位',
    obtain_way VARCHAR(256) NOT NULL DEFAULT '' COMMENT '获取方式',
    description VARCHAR(512) NOT NULL DEFAULT '' COMMENT '翅膀描述',
    icon VARCHAR(128) NOT NULL DEFAULT '' COMMENT '图标路径',
    model VARCHAR(128) NOT NULL DEFAULT '' COMMENT '模型路径',
    PRIMARY KEY (id),
    KEY idx_type (type),
    KEY idx_quality (quality)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='翅膀配置表';

-- 翅膀技能配置表
CREATE TABLE IF NOT EXISTS wing_skill_config (
    id INT UNSIGNED NOT NULL COMMENT '翅膀技能配置ID',
    name VARCHAR(64) NOT NULL DEFAULT '' COMMENT '技能名称',
    type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '技能类型',
    effect_type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '效果类型',
    effect_value INT NOT NULL DEFAULT 0 COMMENT '效果值',
    cooldown INT UNSIGNED NOT NULL DEFAULT 20 COMMENT '冷却时间(秒)',
    duration INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '持续时间(秒)',
    description VARCHAR(256) NOT NULL DEFAULT '' COMMENT '技能描述',
    icon VARCHAR(128) NOT NULL DEFAULT '' COMMENT '图标路径',
    PRIMARY KEY (id),
    KEY idx_type (type),
    KEY idx_effect_type (effect_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='翅膀技能配置表';

-- ==================== 称号配置表 ====================
CREATE TABLE IF NOT EXISTS title_config (
    id INT UNSIGNED NOT NULL COMMENT '称号配置ID',
    name VARCHAR(64) NOT NULL DEFAULT '' COMMENT '称号名称',
    type INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '称号类型 1:成就 2:活动 3:职业 4:节日 5:特殊',
    hp_bonus INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '生命加成',
    attack_bonus INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '攻击加成',
    defense_bonus INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '防御加成',
    speed_bonus INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '速度加成',
    duration INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '持续时间(秒) 0:永久',
    obtain_condition VARCHAR(256) NOT NULL DEFAULT '' COMMENT '获取条件',
    description VARCHAR(512) NOT NULL DEFAULT '' COMMENT '称号描述',
    icon VARCHAR(128) NOT NULL DEFAULT '' COMMENT '图标路径',
    PRIMARY KEY (id),
    KEY idx_type (type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='称号配置表';
