-- 游戏服务器数据库创建脚本
-- 数据库: game_server

-- 创建数据库
CREATE DATABASE IF NOT EXISTS game_server DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

USE game_server;

-- ==================== 账号相关表 ====================

-- 账号表
CREATE TABLE IF NOT EXISTS account (
    account_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '账号ID',
    account_name VARCHAR(64) NOT NULL COMMENT '账号名',
    password VARCHAR(128) NOT NULL COMMENT '密码(MD5加密)',
    channel INT NOT NULL DEFAULT 0 COMMENT '渠道ID',
    create_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '创建时间',
    last_login_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '最后登录时间',
    last_login_ip INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '最后登录IP',
    is_sealed TINYINT NOT NULL DEFAULT 0 COMMENT '是否封号 0:否 1:是',
    seal_end_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '封号结束时间',
    review TINYINT NOT NULL DEFAULT 0 COMMENT '是否评审账号',
    PRIMARY KEY (account_id),
    UNIQUE KEY uk_account_name (account_name),
    KEY idx_channel (channel),
    KEY idx_create_time (create_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='账号表';

-- 账号登录日志表
CREATE TABLE IF NOT EXISTS account_login_log (
    log_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '日志ID',
    account_id BIGINT UNSIGNED NOT NULL COMMENT '账号ID',
    login_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '登录时间',
    login_ip INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '登录IP',
    channel INT NOT NULL DEFAULT 0 COMMENT '渠道',
    version VARCHAR(32) NOT NULL DEFAULT '' COMMENT '客户端版本',
    uuid VARCHAR(64) NOT NULL DEFAULT '' COMMENT '设备UUID',
    idfa VARCHAR(64) NOT NULL DEFAULT '' COMMENT 'IDFA',
    imodel VARCHAR(64) NOT NULL DEFAULT '' COMMENT '设备型号',
    imei VARCHAR(64) NOT NULL DEFAULT '' COMMENT 'IMEI',
    PRIMARY KEY (log_id),
    KEY idx_account_id (account_id),
    KEY idx_login_time (login_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='账号登录日志表';

-- ==================== 角色相关表 ====================

-- 角色基础信息表
CREATE TABLE IF NOT EXISTS role_base (
    role_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '角色ID',
    account_id BIGINT UNSIGNED NOT NULL COMMENT '账号ID',
    server_id INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '服务器ID',
    role_name VARCHAR(64) NOT NULL COMMENT '角色名',
    career TINYINT NOT NULL DEFAULT 1 COMMENT '职业',
    level INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '等级',
    exp BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '经验值',
    head_id INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '头像ID',
    portrait_frame INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '头像框',
    create_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '创建时间',
    last_login_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '最后登录时间',
    is_deleted TINYINT NOT NULL DEFAULT 0 COMMENT '是否删除',
    delete_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '删除时间',
    PRIMARY KEY (role_id),
    UNIQUE KEY uk_role_name (role_name),
    KEY idx_account_id (account_id),
    KEY idx_server_id (server_id),
    KEY idx_level (level)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='角色基础信息表';

-- 角色属性表
CREATE TABLE IF NOT EXISTS role_property (
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    hp INT UNSIGNED NOT NULL DEFAULT 100 COMMENT '当前血量',
    max_hp INT UNSIGNED NOT NULL DEFAULT 100 COMMENT '最大血量',
    mp INT UNSIGNED NOT NULL DEFAULT 100 COMMENT '当前魔法',
    max_mp INT UNSIGNED NOT NULL DEFAULT 100 COMMENT '最大魔法',
    atk INT UNSIGNED NOT NULL DEFAULT 10 COMMENT '攻击力',
    def INT UNSIGNED NOT NULL DEFAULT 5 COMMENT '防御力',
    magic_atk INT UNSIGNED NOT NULL DEFAULT 10 COMMENT '魔法攻击',
    magic_def INT UNSIGNED NOT NULL DEFAULT 5 COMMENT '魔法防御',
    crit INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '暴击率',
    crit_def INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '暴击抵抗',
    hit INT UNSIGNED NOT NULL DEFAULT 100 COMMENT '命中率',
    dodge INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '闪避率',
    move_speed INT UNSIGNED NOT NULL DEFAULT 100 COMMENT '移动速度',
    atk_speed INT UNSIGNED NOT NULL DEFAULT 100 COMMENT '攻击速度',
    update_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '更新时间',
    PRIMARY KEY (role_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='角色属性表';

-- 角色位置表
CREATE TABLE IF NOT EXISTS role_position (
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    scene_id INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '场景ID',
    pos_x FLOAT NOT NULL DEFAULT 0 COMMENT 'X坐标',
    pos_y FLOAT NOT NULL DEFAULT 0 COMMENT 'Y坐标',
    pos_z FLOAT NOT NULL DEFAULT 0 COMMENT 'Z坐标',
    rot_x FLOAT NOT NULL DEFAULT 0 COMMENT 'X旋转',
    rot_y FLOAT NOT NULL DEFAULT 0 COMMENT 'Y旋转',
    rot_z FLOAT NOT NULL DEFAULT 0 COMMENT 'Z旋转',
    update_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '更新时间',
    PRIMARY KEY (role_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='角色位置表';

-- ==================== 背包相关表 ====================

-- 背包物品表
CREATE TABLE IF NOT EXISTS bag_item (
    item_uid BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '物品唯一ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    item_id INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '物品ID',
    item_num INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '物品数量',
    grid_idx INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '格子索引',
    is_bind TINYINT NOT NULL DEFAULT 0 COMMENT '是否绑定',
    get_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '获得时间',
    expire_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '过期时间',
    PRIMARY KEY (item_uid),
    KEY idx_role_id (role_id),
    KEY idx_item_id (item_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='背包物品表';

-- ==================== 装备相关表 ====================

-- 装备信息表
CREATE TABLE IF NOT EXISTS equip_info (
    item_uid BIGINT UNSIGNED NOT NULL COMMENT '物品唯一ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    item_id INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '物品ID',
    strengthen_level INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '强化等级',
    star_level INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '星级',
    gem_slot1 INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '宝石槽1',
    gem_slot2 INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '宝石槽2',
    gem_slot3 INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '宝石槽3',
    gem_slot4 INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '宝石槽4',
    extra_attrs VARCHAR(256) NOT NULL DEFAULT '' COMMENT '额外属性(JSON)',
    is_wear TINYINT NOT NULL DEFAULT 0 COMMENT '是否穿戴',
    wear_pos INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '穿戴位置',
    update_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '更新时间',
    PRIMARY KEY (item_uid),
    KEY idx_role_id (role_id),
    KEY idx_item_id (item_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='装备信息表';

-- ==================== 技能相关表 ====================

-- 角色技能表
CREATE TABLE IF NOT EXISTS role_skill (
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    skill_id INT UNSIGNED NOT NULL COMMENT '技能ID',
    skill_level INT UNSIGNED NOT NULL DEFAULT 1 COMMENT '技能等级',
    key_pos INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '快捷键位置',
    update_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '更新时间',
    PRIMARY KEY (role_id, skill_id),
    KEY idx_role_id (role_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='角色技能表';

-- ==================== 任务相关表 ====================

-- 角色任务表
CREATE TABLE IF NOT EXISTS role_task (
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    task_id INT UNSIGNED NOT NULL COMMENT '任务ID',
    task_status TINYINT NOT NULL DEFAULT 0 COMMENT '任务状态 0:未接 1:已接 2:完成 3:已交',
    progress INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '任务进度',
    accept_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '接取时间',
    complete_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '完成时间',
    submit_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '提交时间',
    PRIMARY KEY (role_id, task_id),
    KEY idx_role_id (role_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='角色任务表';

-- ==================== 邮件相关表 ====================

-- 邮件表
CREATE TABLE IF NOT EXISTS mail (
    mail_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '邮件ID',
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    sender_name VARCHAR(64) NOT NULL DEFAULT '' COMMENT '发送者名称',
    title VARCHAR(128) NOT NULL DEFAULT '' COMMENT '标题',
    content TEXT COMMENT '内容',
    attachment VARCHAR(512) NOT NULL DEFAULT '' COMMENT '附件(JSON)',
    is_read TINYINT NOT NULL DEFAULT 0 COMMENT '是否已读',
    is_got_attachment TINYINT NOT NULL DEFAULT 0 COMMENT '是否已领取附件',
    create_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '创建时间',
    expire_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '过期时间',
    PRIMARY KEY (mail_id),
    KEY idx_role_id (role_id),
    KEY idx_create_time (create_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='邮件表';

-- ==================== 好友相关表 ====================

-- 好友关系表
CREATE TABLE IF NOT EXISTS friend_relation (
    role_id BIGINT UNSIGNED NOT NULL COMMENT '角色ID',
    friend_id BIGINT UNSIGNED NOT NULL COMMENT '好友角色ID',
    friend_name VARCHAR(64) NOT NULL DEFAULT '' COMMENT '好友名称',
    friend_level INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '好友等级',
    friend_career TINYINT NOT NULL DEFAULT 0 COMMENT '好友职业',
    intimacy INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '亲密度',
    create_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '添加时间',
    PRIMARY KEY (role_id, friend_id),
    KEY idx_role_id (role_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='好友关系表';

-- ==================== 服务器相关表 ====================

-- 服务器列表表
CREATE TABLE IF NOT EXISTS server_list (
    server_id INT UNSIGNED NOT NULL COMMENT '服务器ID',
    server_name VARCHAR(64) NOT NULL DEFAULT '' COMMENT '服务器名称',
    server_flag TINYINT NOT NULL DEFAULT 0 COMMENT '服务器标记',
    corner_mark TINYINT NOT NULL DEFAULT 0 COMMENT '角标',
    server_status TINYINT NOT NULL DEFAULT 0 COMMENT '服务器状态',
    server_addr VARCHAR(64) NOT NULL DEFAULT '' COMMENT '服务器地址',
    server_port INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '服务器端口',
    open_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '开服时间',
    max_online INT UNSIGNED NOT NULL DEFAULT 1000 COMMENT '最大在线人数',
    cur_online INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '当前在线人数',
    is_visible TINYINT NOT NULL DEFAULT 1 COMMENT '是否可见',
    PRIMARY KEY (server_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='服务器列表表';

-- ==================== 日志相关表 ====================

-- 运营日志表
CREATE TABLE IF NOT EXISTS log_operation (
    log_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '日志ID',
    log_time INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '日志时间',
    log_type INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '日志类型',
    account_id BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '账号ID',
    role_id BIGINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '角色ID',
    role_level INT UNSIGNED NOT NULL DEFAULT 0 COMMENT '角色等级',
    param1 BIGINT NOT NULL DEFAULT 0 COMMENT '参数1',
    param2 BIGINT NOT NULL DEFAULT 0 COMMENT '参数2',
    param3 BIGINT NOT NULL DEFAULT 0 COMMENT '参数3',
    param4 VARCHAR(256) NOT NULL DEFAULT '' COMMENT '参数4',
    param5 VARCHAR(256) NOT NULL DEFAULT '' COMMENT '参数5',
    PRIMARY KEY (log_id),
    KEY idx_log_time (log_time),
    KEY idx_log_type (log_type),
    KEY idx_role_id (role_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='运营日志表';

-- 插入默认服务器配置
INSERT INTO server_list (server_id, server_name, server_flag, corner_mark, server_status, server_addr, server_port, open_time, max_online, cur_online, is_visible) VALUES
(1, '测试服1', 1, 1, 1, '127.0.0.1', 8001, UNIX_TIMESTAMP(), 1000, 0, 1),
(2, '测试服2', 1, 0, 1, '127.0.0.1', 8002, UNIX_TIMESTAMP(), 1000, 0, 1)
ON DUPLICATE KEY UPDATE server_name = VALUES(server_name);
