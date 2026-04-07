-- 游戏测试账号和角色数据插入脚本
-- 使用前请先执行 create_db.sql 和 create_player_data_tables.sql 创建数据库和表

USE game_server;

-- ==================== 测试账号数据 ====================
-- 插入10个测试账号，密码统一为: 123456 (MD5: e10adc3949ba59abbe56e057f20f883e)
INSERT INTO account (account_id, account_name, password, channel, create_time, last_login_time, last_login_ip, is_sealed, seal_end_time, review) VALUES
(10001, 'test001', 'e10adc3949ba59abbe56e057f20f883e', 0, UNIX_TIMESTAMP(), UNIX_TIMESTAMP(), INET_ATON('127.0.0.1'), 0, 0, 0),
(10002, 'test002', 'e10adc3949ba59abbe56e057f20f883e', 0, UNIX_TIMESTAMP(), UNIX_TIMESTAMP(), INET_ATON('127.0.0.1'), 0, 0, 0),
(10003, 'test003', 'e10adc3949ba59abbe56e057f20f883e', 0, UNIX_TIMESTAMP(), UNIX_TIMESTAMP(), INET_ATON('127.0.0.1'), 0, 0, 0),
(10004, 'test004', 'e10adc3949ba59abbe56e057f20f883e', 0, UNIX_TIMESTAMP(), UNIX_TIMESTAMP(), INET_ATON('127.0.0.1'), 0, 0, 0),
(10005, 'test005', 'e10adc3949ba59abbe56e057f20f883e', 0, UNIX_TIMESTAMP(), UNIX_TIMESTAMP(), INET_ATON('127.0.0.1'), 0, 0, 0),
(10006, 'test006', 'e10adc3949ba59abbe56e057f20f883e', 0, UNIX_TIMESTAMP(), UNIX_TIMESTAMP(), INET_ATON('127.0.0.1'), 0, 0, 0),
(10007, 'test007', 'e10adc3949ba59abbe56e057f20f883e', 0, UNIX_TIMESTAMP(), UNIX_TIMESTAMP(), INET_ATON('127.0.0.1'), 0, 0, 0),
(10008, 'test008', 'e10adc3949ba59abbe56e057f20f883e', 0, UNIX_TIMESTAMP(), UNIX_TIMESTAMP(), INET_ATON('127.0.0.1'), 0, 0, 0),
(10009, 'test009', 'e10adc3949ba59abbe56e057f20f883e', 0, UNIX_TIMESTAMP(), UNIX_TIMESTAMP(), INET_ATON('127.0.0.1'), 0, 0, 0),
(10010, 'test010', 'e10adc3949ba59abbe56e057f20f883e', 0, UNIX_TIMESTAMP(), UNIX_TIMESTAMP(), INET_ATON('127.0.0.1'), 0, 0, 0);

-- ==================== 角色基础信息数据 ====================
INSERT INTO role_base (role_id, account_id, server_id, role_name, career, level, exp, head_id, portrait_frame, create_time, last_login_time, is_deleted, delete_time) VALUES
(20001, 10001, 1, '勇士001', 1, 10, 5000, 1, 0, UNIX_TIMESTAMP(), UNIX_TIMESTAMP(), 0, 0),
(20002, 10002, 1, '法师002', 2, 15, 12000, 2, 0, UNIX_TIMESTAMP(), UNIX_TIMESTAMP(), 0, 0),
(20003, 10003, 1, '射手003', 3, 8, 3000, 3, 0, UNIX_TIMESTAMP(), UNIX_TIMESTAMP(), 0, 0),
(20004, 10004, 1, '战士004', 1, 20, 25000, 1, 1, UNIX_TIMESTAMP(), UNIX_TIMESTAMP(), 0, 0),
(20005, 10005, 1, '法师005', 2, 12, 8000, 2, 0, UNIX_TIMESTAMP(), UNIX_TIMESTAMP(), 0, 0),
(20006, 10006, 1, '射手006', 3, 25, 45000, 3, 2, UNIX_TIMESTAMP(), UNIX_TIMESTAMP(), 0, 0),
(20007, 10007, 1, '勇士007', 1, 5, 1000, 1, 0, UNIX_TIMESTAMP(), UNIX_TIMESTAMP(), 0, 0),
(20008, 10008, 1, '法师008', 2, 30, 80000, 2, 3, UNIX_TIMESTAMP(), UNIX_TIMESTAMP(), 0, 0),
(20009, 10009, 1, '射手009', 3, 18, 18000, 3, 0, UNIX_TIMESTAMP(), UNIX_TIMESTAMP(), 0, 0),
(20010, 10010, 1, '勇士010', 1, 35, 120000, 1, 4, UNIX_TIMESTAMP(), UNIX_TIMESTAMP(), 0, 0);

-- ==================== 角色属性数据 ====================
INSERT INTO role_property (role_id, hp, max_hp, mp, max_mp, atk, def, magic_atk, magic_def, crit, crit_def, hit, dodge, move_speed, atk_speed, update_time) VALUES
(20001, 500, 500, 200, 200, 50, 30, 20, 15, 10, 5, 100, 10, 100, 100, UNIX_TIMESTAMP()),
(20002, 350, 350, 400, 400, 20, 20, 80, 40, 15, 8, 100, 15, 100, 100, UNIX_TIMESTAMP()),
(20003, 400, 400, 250, 250, 60, 25, 30, 20, 20, 10, 120, 20, 120, 110, UNIX_TIMESTAMP()),
(20004, 800, 800, 300, 300, 100, 60, 40, 30, 25, 15, 110, 12, 100, 100, UNIX_TIMESTAMP()),
(20005, 450, 450, 350, 350, 35, 28, 65, 35, 18, 10, 105, 18, 100, 100, UNIX_TIMESTAMP()),
(20006, 600, 600, 450, 450, 120, 40, 50, 30, 30, 20, 130, 25, 130, 120, UNIX_TIMESTAMP()),
(20007, 300, 300, 150, 150, 30, 20, 15, 10, 8, 3, 95, 8, 95, 95, UNIX_TIMESTAMP()),
(20008, 500, 500, 600, 600, 30, 35, 150, 60, 35, 25, 115, 22, 100, 100, UNIX_TIMESTAMP()),
(20009, 550, 550, 320, 320, 85, 35, 45, 28, 22, 12, 125, 18, 115, 105, UNIX_TIMESTAMP()),
(20010, 1200, 1200, 400, 400, 180, 100, 60, 50, 40, 30, 130, 20, 110, 110, UNIX_TIMESTAMP());

-- ==================== 角色位置数据 ====================
INSERT INTO role_position (role_id, scene_id, pos_x, pos_y, pos_z, rot_x, rot_y, rot_z, update_time) VALUES
(20001, 1001, 100.5, 0.0, 200.3, 0.0, 45.0, 0.0, UNIX_TIMESTAMP()),
(20002, 1001, 150.2, 0.0, 180.7, 0.0, 90.0, 0.0, UNIX_TIMESTAMP()),
(20003, 1002, 80.0, 10.0, 120.5, 0.0, 0.0, 0.0, UNIX_TIMESTAMP()),
(20004, 1001, 200.0, 0.0, 250.0, 0.0, 180.0, 0.0, UNIX_TIMESTAMP()),
(20005, 1003, 50.5, 5.0, 80.2, 0.0, 30.0, 0.0, UNIX_TIMESTAMP()),
(20006, 1002, 300.0, 20.0, 400.0, 0.0, 270.0, 0.0, UNIX_TIMESTAMP()),
(20007, 1001, 25.0, 0.0, 50.0, 0.0, 60.0, 0.0, UNIX_TIMESTAMP()),
(20008, 1004, 500.0, 0.0, 500.0, 0.0, 135.0, 0.0, UNIX_TIMESTAMP()),
(20009, 1003, 120.0, 8.0, 160.0, 0.0, 15.0, 0.0, UNIX_TIMESTAMP()),
(20010, 1004, 600.0, 0.0, 600.0, 0.0, 225.0, 0.0, UNIX_TIMESTAMP());

-- ==================== 背包物品数据 ====================
INSERT INTO bag_item (item_uid, role_id, item_id, item_num, grid_idx, is_bind, get_time, expire_time) VALUES
(30001, 20001, 1001, 99, 0, 0, UNIX_TIMESTAMP(), 0),
(30002, 20001, 1002, 50, 1, 0, UNIX_TIMESTAMP(), 0),
(30003, 20001, 2001, 5, 2, 1, UNIX_TIMESTAMP(), 0),
(30004, 20002, 1001, 50, 0, 0, UNIX_TIMESTAMP(), 0),
(30005, 20002, 3001, 20, 1, 0, UNIX_TIMESTAMP(), 0),
(30006, 20003, 1001, 30, 0, 0, UNIX_TIMESTAMP(), 0),
(30007, 20003, 2002, 3, 1, 1, UNIX_TIMESTAMP(), 0),
(30008, 20004, 1001, 200, 0, 0, UNIX_TIMESTAMP(), 0),
(30009, 20004, 1002, 100, 1, 0, UNIX_TIMESTAMP(), 0),
(30010, 20004, 2001, 10, 2, 0, UNIX_TIMESTAMP(), 0),
(30011, 20005, 1001, 80, 0, 0, UNIX_TIMESTAMP(), 0),
(30012, 20006, 1001, 150, 0, 0, UNIX_TIMESTAMP(), 0),
(30013, 20006, 3002, 10, 1, 1, UNIX_TIMESTAMP(), 0),
(30014, 20007, 1001, 20, 0, 0, UNIX_TIMESTAMP(), 0),
(30015, 20008, 1001, 500, 0, 0, UNIX_TIMESTAMP(), 0),
(30016, 20008, 1002, 200, 1, 0, UNIX_TIMESTAMP(), 0),
(30017, 20008, 2001, 20, 2, 0, UNIX_TIMESTAMP(), 0),
(30018, 20008, 3001, 50, 3, 0, UNIX_TIMESTAMP(), 0),
(30019, 20009, 1001, 100, 0, 0, UNIX_TIMESTAMP(), 0),
(30020, 20010, 1001, 999, 0, 0, UNIX_TIMESTAMP(), 0),
(30021, 20010, 2001, 50, 1, 0, UNIX_TIMESTAMP(), 0),
(30022, 20010, 3001, 100, 2, 0, UNIX_TIMESTAMP(), 0);

-- ==================== 装备信息数据 ====================
INSERT INTO equip_info (item_uid, role_id, item_id, strengthen_level, star_level, gem_slot1, gem_slot2, gem_slot3, gem_slot4, extra_attrs, is_wear, wear_pos, update_time) VALUES
(40001, 20001, 5001, 3, 1, 0, 0, 0, 0, '{"atk":5}', 1, 1, UNIX_TIMESTAMP()),
(40002, 20001, 5002, 2, 0, 0, 0, 0, 0, '{}', 1, 2, UNIX_TIMESTAMP()),
(40003, 20002, 5003, 5, 2, 1, 0, 0, 0, '{"magic_atk":10}', 1, 1, UNIX_TIMESTAMP()),
(40004, 20004, 5001, 8, 3, 2, 3, 0, 0, '{"atk":15,"crit":5}', 1, 1, UNIX_TIMESTAMP()),
(40005, 20004, 5004, 5, 2, 0, 0, 0, 0, '{"def":10}', 1, 3, UNIX_TIMESTAMP()),
(40006, 20006, 5005, 10, 5, 4, 5, 6, 0, '{"atk":25,"crit":10,"atk_speed":5}', 1, 1, UNIX_TIMESTAMP()),
(40007, 20008, 5003, 12, 4, 1, 2, 3, 4, '{"magic_atk":30,"magic_def":15}', 1, 1, UNIX_TIMESTAMP()),
(40008, 20010, 5001, 15, 5, 5, 6, 7, 8, '{"atk":50,"crit":20,"crit_def":10,"hp":100}', 1, 1, UNIX_TIMESTAMP()),
(40009, 20010, 5002, 10, 3, 0, 0, 0, 0, '{"def":20}', 1, 2, UNIX_TIMESTAMP()),
(40010, 20010, 5004, 8, 2, 3, 4, 0, 0, '{"max_hp":200}', 1, 3, UNIX_TIMESTAMP());

-- ==================== 角色技能数据 ====================
INSERT INTO role_skill (role_id, skill_id, skill_level, key_pos, update_time) VALUES
(20001, 1, 3, 1, UNIX_TIMESTAMP()),
(20001, 2, 2, 2, UNIX_TIMESTAMP()),
(20001, 3, 1, 3, UNIX_TIMESTAMP()),
(20002, 1, 5, 1, UNIX_TIMESTAMP()),
(20002, 4, 3, 2, UNIX_TIMESTAMP()),
(20002, 5, 2, 3, UNIX_TIMESTAMP()),
(20003, 1, 2, 1, UNIX_TIMESTAMP()),
(20003, 6, 1, 2, UNIX_TIMESTAMP()),
(20004, 1, 8, 1, UNIX_TIMESTAMP()),
(20004, 2, 5, 2, UNIX_TIMESTAMP()),
(20004, 3, 3, 3, UNIX_TIMESTAMP()),
(20004, 7, 2, 4, UNIX_TIMESTAMP()),
(20005, 1, 4, 1, UNIX_TIMESTAMP()),
(20005, 4, 2, 2, UNIX_TIMESTAMP()),
(20006, 1, 10, 1, UNIX_TIMESTAMP()),
(20006, 6, 5, 2, UNIX_TIMESTAMP()),
(20006, 8, 3, 3, UNIX_TIMESTAMP()),
(20007, 1, 1, 1, UNIX_TIMESTAMP()),
(20008, 1, 12, 1, UNIX_TIMESTAMP()),
(20008, 4, 8, 2, UNIX_TIMESTAMP()),
(20008, 5, 5, 3, UNIX_TIMESTAMP()),
(20008, 9, 3, 4, UNIX_TIMESTAMP()),
(20009, 1, 6, 1, UNIX_TIMESTAMP()),
(20009, 2, 3, 2, UNIX_TIMESTAMP()),
(20010, 1, 15, 1, UNIX_TIMESTAMP()),
(20010, 2, 10, 2, UNIX_TIMESTAMP()),
(20010, 3, 8, 3, UNIX_TIMESTAMP()),
(20010, 7, 5, 4, UNIX_TIMESTAMP());

-- ==================== 角色任务数据 ====================
INSERT INTO role_task (role_id, task_id, task_status, progress, accept_time, complete_time, submit_time) VALUES
(20001, 1, 3, 100, UNIX_TIMESTAMP()-86400, UNIX_TIMESTAMP()-43200, UNIX_TIMESTAMP()-36000),
(20001, 2, 2, 50, UNIX_TIMESTAMP()-3600, 0, 0),
(20001, 3, 1, 0, UNIX_TIMESTAMP(), 0, 0),
(20002, 1, 3, 100, UNIX_TIMESTAMP()-172800, UNIX_TIMESTAMP()-86400, UNIX_TIMESTAMP()-72000),
(20002, 2, 3, 100, UNIX_TIMESTAMP()-86400, UNIX_TIMESTAMP()-43200, UNIX_TIMESTAMP()-36000),
(20002, 4, 2, 30, UNIX_TIMESTAMP()-7200, 0, 0),
(20003, 1, 2, 80, UNIX_TIMESTAMP()-1800, 0, 0),
(20004, 1, 3, 100, UNIX_TIMESTAMP()-259200, UNIX_TIMESTAMP()-172800, UNIX_TIMESTAMP()-144000),
(20004, 2, 3, 100, UNIX_TIMESTAMP()-172800, UNIX_TIMESTAMP()-86400, UNIX_TIMESTAMP()-72000),
(20004, 3, 3, 100, UNIX_TIMESTAMP()-86400, UNIX_TIMESTAMP()-43200, UNIX_TIMESTAMP()-36000),
(20004, 5, 2, 60, UNIX_TIMESTAMP()-3600, 0, 0),
(20005, 1, 3, 100, UNIX_TIMESTAMP()-86400, UNIX_TIMESTAMP()-43200, UNIX_TIMESTAMP()-36000),
(20005, 2, 1, 0, UNIX_TIMESTAMP(), 0, 0),
(20006, 1, 3, 100, UNIX_TIMESTAMP()-345600, UNIX_TIMESTAMP()-259200, UNIX_TIMESTAMP()-216000),
(20006, 2, 3, 100, UNIX_TIMESTAMP()-259200, UNIX_TIMESTAMP()-172800, UNIX_TIMESTAMP()-144000),
(20006, 3, 3, 100, UNIX_TIMESTAMP()-172800, UNIX_TIMESTAMP()-86400, UNIX_TIMESTAMP()-72000),
(20006, 4, 3, 100, UNIX_TIMESTAMP()-86400, UNIX_TIMESTAMP()-43200, UNIX_TIMESTAMP()-36000),
(20006, 6, 2, 40, UNIX_TIMESTAMP()-7200, 0, 0),
(20007, 1, 1, 0, UNIX_TIMESTAMP(), 0, 0),
(20008, 1, 3, 100, UNIX_TIMESTAMP()-432000, UNIX_TIMESTAMP()-345600, UNIX_TIMESTAMP()-302400),
(20008, 2, 3, 100, UNIX_TIMESTAMP()-345600, UNIX_TIMESTAMP()-259200, UNIX_TIMESTAMP()-216000),
(20008, 3, 3, 100, UNIX_TIMESTAMP()-259200, UNIX_TIMESTAMP()-172800, UNIX_TIMESTAMP()-144000),
(20008, 4, 3, 100, UNIX_TIMESTAMP()-172800, UNIX_TIMESTAMP()-86400, UNIX_TIMESTAMP()-72000),
(20008, 5, 3, 100, UNIX_TIMESTAMP()-86400, UNIX_TIMESTAMP()-43200, UNIX_TIMESTAMP()-36000),
(20008, 7, 2, 80, UNIX_TIMESTAMP()-3600, 0, 0),
(20009, 1, 3, 100, UNIX_TIMESTAMP()-172800, UNIX_TIMESTAMP()-86400, UNIX_TIMESTAMP()-72000),
(20009, 2, 2, 70, UNIX_TIMESTAMP()-7200, 0, 0),
(20010, 1, 3, 100, UNIX_TIMESTAMP()-518400, UNIX_TIMESTAMP()-432000, UNIX_TIMESTAMP()-388800),
(20010, 2, 3, 100, UNIX_TIMESTAMP()-432000, UNIX_TIMESTAMP()-345600, UNIX_TIMESTAMP()-302400),
(20010, 3, 3, 100, UNIX_TIMESTAMP()-345600, UNIX_TIMESTAMP()-259200, UNIX_TIMESTAMP()-216000),
(20010, 4, 3, 100, UNIX_TIMESTAMP()-259200, UNIX_TIMESTAMP()-172800, UNIX_TIMESTAMP()-144000),
(20010, 5, 3, 100, UNIX_TIMESTAMP()-172800, UNIX_TIMESTAMP()-86400, UNIX_TIMESTAMP()-72000),
(20010, 6, 3, 100, UNIX_TIMESTAMP()-86400, UNIX_TIMESTAMP()-43200, UNIX_TIMESTAMP()-36000),
(20010, 8, 1, 0, UNIX_TIMESTAMP(), 0, 0);

-- ==================== 邮件数据 ====================
INSERT INTO mail (mail_id, role_id, sender_name, title, content, attachment, is_read, is_got_attachment, create_time, expire_time) VALUES
(50001, 20001, '系统', '欢迎来到游戏', '亲爱的玩家，欢迎加入我们的游戏世界！这是给你的新手礼包。', '[{"item_id":1001,"count":1000},{"item_id":1002,"count":500}]', 1, 1, UNIX_TIMESTAMP()-86400, UNIX_TIMESTAMP()+604800),
(50002, 20001, '系统', '每日登录奖励', '感谢您今日登录游戏，请领取您的登录奖励。', '[{"item_id":1001,"count":100}]', 0, 0, UNIX_TIMESTAMP(), UNIX_TIMESTAMP()+86400),
(50003, 20002, '系统', '欢迎来到游戏', '亲爱的玩家，欢迎加入我们的游戏世界！这是给你的新手礼包。', '[{"item_id":1001,"count":1000},{"item_id":1002,"count":500}]', 1, 1, UNIX_TIMESTAMP()-172800, UNIX_TIMESTAMP()+432000),
(50004, 20004, '系统', '等级奖励', '恭喜您达到20级，请领取等级奖励。', '[{"item_id":2001,"count":5},{"item_id":3001,"count":10}]', 0, 0, UNIX_TIMESTAMP()-3600, UNIX_TIMESTAMP()+172800),
(50005, 20006, '系统', '活动奖励', '恭喜您在活动中获得优异成绩！', '[{"item_id":1002,"count":1000},{"item_id":2001,"count":10}]', 0, 0, UNIX_TIMESTAMP()-7200, UNIX_TIMESTAMP()+86400),
(50006, 20008, '系统', 'VIP特权礼包', '感谢您成为VIP玩家，请领取专属礼包。', '[{"item_id":1001,"count":5000},{"item_id":1002,"count":2000},{"item_id":3001,"count":50}]', 0, 0, UNIX_TIMESTAMP()-1800, UNIX_TIMESTAMP()+259200),
(50007, 20010, '系统', '排行榜奖励', '恭喜您在排行榜中获得第一名！', '[{"item_id":1001,"count":10000},{"item_id":2001,"count":50},{"item_id":5001,"count":1}]', 0, 0, UNIX_TIMESTAMP()-3600, UNIX_TIMESTAMP()+604800);

-- ==================== 好友关系数据 ====================
INSERT INTO friend_relation (role_id, friend_id, friend_name, friend_level, friend_career, intimacy, create_time) VALUES
(20001, 20002, '法师002', 15, 2, 100, UNIX_TIMESTAMP()-86400),
(20001, 20003, '射手003', 8, 3, 50, UNIX_TIMESTAMP()-43200),
(20002, 20001, '勇士001', 10, 1, 100, UNIX_TIMESTAMP()-86400),
(20002, 20004, '战士004', 20, 1, 200, UNIX_TIMESTAMP()-172800),
(20003, 20001, '勇士001', 10, 1, 50, UNIX_TIMESTAMP()-43200),
(20004, 20002, '法师002', 15, 2, 200, UNIX_TIMESTAMP()-172800),
(20004, 20005, '法师005', 12, 2, 150, UNIX_TIMESTAMP()-86400),
(20005, 20004, '战士004', 20, 1, 150, UNIX_TIMESTAMP()-86400),
(20006, 20008, '法师008', 30, 2, 300, UNIX_TIMESTAMP()-259200),
(20008, 20006, '射手006', 25, 3, 300, UNIX_TIMESTAMP()-259200),
(20008, 20010, '勇士010', 35, 1, 500, UNIX_TIMESTAMP()-345600),
(20009, 20010, '勇士010', 35, 1, 100, UNIX_TIMESTAMP()-86400),
(20010, 20008, '法师008', 30, 2, 500, UNIX_TIMESTAMP()-345600),
(20010, 20009, '射手009', 18, 3, 100, UNIX_TIMESTAMP()-86400);

-- 插入完成
SELECT '测试数据插入完成！' AS result;
SELECT CONCAT('账号数量: ', COUNT(*)) AS account_count FROM account WHERE account_id BETWEEN 10001 AND 10010;
SELECT CONCAT('角色数量: ', COUNT(*)) AS role_count FROM role_base WHERE role_id BETWEEN 20001 AND 20010;
