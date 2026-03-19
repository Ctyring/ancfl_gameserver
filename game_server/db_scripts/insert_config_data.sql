-- 游戏配置数据插入脚本
-- 使用前请先执行 create_config_db.sql 创建数据库和表

USE game_config;

-- ==================== Buff配置数据 ====================
INSERT INTO buff_config (id, name, type, effect_type, effect_value, duration, interval, max_stack, is_debuff, can_dispel, description, icon) VALUES
(1, '攻击提升', 1, 1, 10, 30, 0, 3, 0, 1, '提升10点攻击力', 'buff_attack_up.png'),
(2, '防御提升', 1, 3, 10, 30, 0, 3, 0, 1, '提升10点防御力', 'buff_defense_up.png'),
(3, '速度提升', 1, 5, 20, 15, 0, 2, 0, 1, '提升20点速度', 'buff_speed_up.png'),
(4, '攻击降低', 2, 2, -10, 20, 0, 2, 1, 1, '降低10点攻击力', 'buff_attack_down.png'),
(5, '眩晕', 5, 9, 0, 3, 0, 1, 1, 1, '无法行动', 'buff_stun.png'),
(6, '沉默', 5, 10, 0, 5, 0, 1, 1, 1, '无法使用技能', 'buff_silence.png'),
(7, '中毒', 3, 11, 5, 10, 2, 5, 1, 1, '每2秒造成5点伤害', 'buff_poison.png'),
(8, '持续治疗', 4, 7, 10, 15, 3, 3, 0, 1, '每3秒恢复10点生命', 'buff_hp_regen.png'),
(9, '燃烧', 3, 12, 8, 8, 2, 3, 1, 1, '每2秒造成8点伤害', 'buff_burn.png'),
(10, '冰冻', 5, 13, 0, 4, 0, 1, 1, 1, '无法行动', 'buff_freeze.png'),
(11, '护盾', 6, 14, 200, 10, 0, 1, 0, 1, '吸收200点伤害', 'buff_shield.png'),
(12, '法力恢复', 4, 8, 20, 12, 3, 3, 0, 1, '每3秒恢复20点法力', 'buff_mp_regen.png');

-- ==================== 任务配置数据 ====================
INSERT INTO task_config (id, name, type, level_requirement, pre_task_id, description, accept_npc, submit_npc) VALUES
(1, '新手引导', 1, 1, 0, '完成新手引导任务，熟悉游戏基本操作', 1001, 1001),
(2, '装备强化', 1, 5, 1, '强化一件装备到+3', 1002, 1002),
(3, '击杀野猪', 3, 1, 0, '击杀10只野猪', 0, 0),
(4, '收集草药', 3, 1, 0, '收集20株草药', 0, 0),
(5, '完成副本', 4, 10, 0, '完成5次副本', 0, 0),
(6, '技能升级', 1, 8, 2, '将一个技能升级到5级', 1003, 1003),
(7, '达到10级', 1, 1, 0, '角色达到10级', 0, 0),
(8, '击杀BOSS', 2, 15, 6, '击杀BOSS怪物', 1004, 1004),
(9, '日常登录', 3, 1, 0, '每日登录游戏', 0, 0),
(10, '周常挑战', 4, 20, 0, '完成周常挑战任务', 0, 0);

-- 任务条件配置数据
INSERT INTO task_condition_config (task_id, condition_type, target_id, target_count, description) VALUES
(1, 1, 1001, 5, '击杀5只史莱姆'),
(2, 7, 2001, 1, '强化装备到+3'),
(3, 1, 1002, 10, '击杀10只野猪'),
(4, 2, 3001, 20, '收集20株草药'),
(5, 6, 4001, 5, '完成5次副本'),
(6, 8, 5001, 5, '技能升级到5级'),
(7, 4, 10, 1, '达到10级'),
(8, 1, 1005, 1, '击杀BOSS'),
(9, 3, 1001, 1, '与村长对话'),
(10, 1, 1010, 50, '击杀50只怪物');

-- 任务奖励配置数据
INSERT INTO task_reward_config (task_id, reward_type, reward_id, reward_count) VALUES
(1, 1, 0, 100),
(1, 2, 0, 50),
(2, 1, 0, 200),
(2, 2, 0, 100),
(3, 1, 0, 50),
(3, 2, 0, 20),
(4, 1, 0, 80),
(4, 2, 0, 30),
(5, 1, 0, 500),
(5, 2, 0, 200),
(6, 1, 0, 300),
(6, 2, 0, 150),
(7, 1, 0, 1000),
(7, 2, 0, 500),
(8, 1, 0, 800),
(8, 2, 0, 400),
(9, 1, 0, 50),
(9, 2, 0, 100),
(10, 1, 0, 2000),
(10, 2, 0, 1000);

-- ==================== 技能配置数据 ====================
INSERT INTO skill_config (id, name, type, target_type, cast_range, aoe_radius, cooldown, mp_cost, damage, heal, description, icon) VALUES
(1, '普通攻击', 1, 2, 5, 0, 1, 0, 100, 0, '普通物理攻击', 'skill_attack.png'),
(2, '火球术', 1, 2, 10, 0, 5, 20, 200, 0, '发射火球攻击单个目标', 'skill_fireball.png'),
(3, '治疗术', 5, 2, 8, 0, 8, 30, 0, 150, '治疗单个目标', 'skill_heal.png'),
(4, '群体治疗', 5, 3, 10, 5, 15, 50, 0, 100, '治疗范围内所有友方', 'skill_aoe_heal.png'),
(5, '眩晕术', 6, 2, 8, 0, 12, 40, 50, 0, '眩晕目标3秒', 'skill_stun.png'),
(6, '攻击力提升', 3, 1, 0, 0, 30, 25, 0, 0, '提升自身攻击力', 'skill_buff_atk.png'),
(7, '烈焰风暴', 1, 3, 10, 6, 20, 60, 300, 0, '对范围内敌人造成火焰伤害', 'skill_firestorm.png'),
(8, '闪避', 2, 1, 0, 0, 0, 0, 0, 0, '被动提升闪避率', 'skill_dodge.png'),
(9, '冰冻术', 6, 2, 10, 0, 15, 45, 120, 0, '冰冻目标4秒', 'skill_freeze.png'),
(10, '神圣护盾', 3, 1, 0, 0, 45, 50, 0, 0, '获得护盾吸收伤害', 'skill_shield.png'),
(11, '雷电术', 1, 2, 12, 0, 8, 35, 250, 0, '召唤雷电攻击目标', 'skill_thunder.png'),
(12, '毒雾', 4, 3, 8, 4, 18, 40, 80, 0, '释放毒雾造成持续伤害', 'skill_poison.png');

-- 技能效果配置数据
INSERT INTO skill_effect_config (skill_id, effect_type, effect_value, duration, interval) VALUES
(2, 12, 5, 5, 1),
(5, 9, 0, 3, 0),
(6, 1, 10, 30, 0),
(7, 12, 5, 5, 2),
(8, 3, 15, 0, 0),
(9, 13, 0, 4, 0),
(10, 14, 500, 10, 0),
(11, 1, 0, 0, 0),
(12, 11, 10, 8, 2);

-- ==================== 商店配置数据 ====================
INSERT INTO shop_config (id, name, type, refresh_time, refresh_cost, is_open, description) VALUES
(1, '普通商店', 1, 0, 0, 1, '出售基础物品'),
(2, '神秘商店', 2, 86400, 50, 1, '限时折扣商品'),
(3, '公会商店', 3, 604800, 0, 1, '使用公会币购买'),
(4, 'PVP商店', 4, 604800, 0, 1, '使用荣誉点购买'),
(5, '活动商店', 5, 0, 0, 1, '活动期间开放');

-- 商店商品配置数据
INSERT INTO shop_item_config (shop_id, item_config_id, item_count, price_type, price, discount, limit_count, require_level, require_vip, sort_order) VALUES
(1, 1001, 1, 1, 100, 0, 999, 1, 0, 1),
(1, 1002, 1, 1, 200, 0, 999, 1, 0, 2),
(1, 1003, 1, 1, 500, 0, 999, 5, 0, 3),
(1, 1004, 1, 2, 50, 0, 999, 1, 0, 4),
(2, 2001, 1, 2, 500, 50, 1, 10, 1, 1),
(2, 2002, 1, 2, 800, 30, 1, 15, 2, 2),
(2, 2003, 1, 2, 1000, 20, 1, 20, 3, 3),
(3, 3001, 1, 3, 1000, 0, 10, 1, 0, 1),
(3, 3002, 1, 3, 2000, 0, 5, 20, 0, 2),
(4, 4001, 1, 4, 500, 0, 20, 1, 0, 1),
(4, 4002, 1, 4, 1000, 0, 10, 30, 1, 2),
(5, 5001, 1, 5, 100, 0, 999, 1, 0, 1);

-- ==================== 物品配置数据 ====================
INSERT INTO item_config (id, name, type, quality, max_stack, is_bind, description, icon) VALUES
(1001, '生命药水', 1, 1, 99, 0, '恢复100点生命值', 'item_hp_potion.png'),
(1002, '法力药水', 1, 1, 99, 0, '恢复50点法力值', 'item_mp_potion.png'),
(1003, '强化石', 3, 2, 999, 0, '用于强化装备', 'item_stone.png'),
(1004, '复活币', 1, 3, 99, 0, '死亡后复活', 'item_revive.png'),
(2001, '神秘宝箱', 1, 4, 99, 0, '开启获得随机奖励', 'item_box.png'),
(2002, '高级强化石', 3, 3, 999, 0, '高级装备强化材料', 'item_stone_high.png'),
(2003, '传说装备箱', 1, 5, 1, 1, '开启获得传说装备', 'item_legend_box.png'),
(3001, '公会贡献令', 3, 2, 999, 0, '增加公会贡献', 'item_guild.png'),
(3002, '公会技能书', 1, 3, 99, 0, '学习公会技能', 'item_guild_skill.png'),
(4001, '荣誉徽章', 3, 3, 999, 0, 'PVP荣誉象征', 'item_honor.png'),
(4002, '竞技场门票', 1, 2, 99, 0, '进入竞技场', 'item_arena.png'),
(5001, '活动代币', 3, 2, 9999, 0, '活动专用货币', 'item_event.png');

-- ==================== 怪物配置数据 ====================
INSERT INTO monster_config (id, name, level, type, hp, mp, attack, defense, speed, exp, drop_items) VALUES
(1001, '史莱姆', 1, 1, 50, 0, 5, 2, 80, 10, '[{"item_id":1001,"rate":0.5,"count":1}]'),
(1002, '野猪', 3, 1, 80, 0, 8, 3, 90, 15, '[{"item_id":1001,"rate":0.6,"count":1},{"item_id":1003,"rate":0.2,"count":1}]'),
(1003, '狼', 5, 1, 120, 0, 12, 4, 110, 25, '[{"item_id":1002,"rate":0.5,"count":1}]'),
(1004, '熊', 8, 2, 300, 0, 20, 10, 85, 50, '[{"item_id":1003,"rate":0.4,"count":2}]'),
(1005, 'BOSS-巨魔', 15, 3, 2000, 100, 80, 30, 70, 500, '[{"item_id":2001,"rate":1.0,"count":1},{"item_id":2002,"rate":0.5,"count":1}]'),
(1010, '骷髅兵', 10, 1, 200, 0, 25, 8, 95, 40, '[{"item_id":1001,"rate":0.7,"count":2}]');

-- ==================== 场景配置数据 ====================
INSERT INTO scene_config (id, name, type, max_player, pk_mode, enter_level, description) VALUES
(1, '新手村', 1, 100, 0, 1, '新手出生地点'),
(2, '野猪林', 2, 50, 1, 1, '野猪出没的森林'),
(3, '狼牙山', 2, 50, 1, 5, '狼群聚集地'),
(4, '副本-地下城', 3, 5, 0, 10, '10级副本'),
(5, '竞技场', 4, 20, 2, 20, 'PVP战场'),
(6, '主城', 1, 200, 0, 1, '玩家主城');

-- ==================== 宠物配置数据 ====================
INSERT INTO pet_config (id, name, type, quality, hp, attack, defense, speed, skill_slots, growth_rate, obtain_way, description, icon) VALUES
(1001, '小猫咪', 1, 1, 100, 10, 5, 120, 3, 1.0, '新手任务', '可爱的小猫咪，陪伴你冒险', 'pet_cat.png'),
(1002, '小狼', 1, 2, 150, 15, 8, 130, 3, 1.2, '野外捕捉', '忠诚的小狼，战斗力不错', 'pet_wolf.png'),
(1003, '独角兽', 2, 3, 200, 20, 12, 140, 4, 1.4, '活动奖励', '高贵的独角兽，拥有特殊能力', 'pet_unicorn.png'),
(1004, '凤凰', 3, 5, 300, 30, 20, 160, 5, 1.8, '传说任务', '传说中的凤凰，拥有强大的火属性能力', 'pet_phoenix.png'),
(1005, '龙宝宝', 3, 4, 250, 25, 15, 150, 4, 1.6, '副本掉落', '幼年的龙，潜力无限', 'pet_dragon.png');

-- 宠物技能配置数据
INSERT INTO pet_skill_config (id, name, type, effect_type, effect_value, cooldown, duration, description, icon) VALUES
(2001, '爪击', 1, 1, 20, 5, 0, '基础物理攻击', 'pet_skill_attack.png'),
(2002, '治愈术', 2, 7, 50, 10, 0, '恢复主人生命值', 'pet_skill_heal.png'),
(2003, '火焰吐息', 1, 12, 30, 8, 0, '释放火焰攻击', 'pet_skill_fire.png'),
(2004, '防御光环', 2, 3, 10, 20, 15, '提升主人防御力', 'pet_skill_defense.png'),
(2005, '速度光环', 2, 5, 15, 15, 10, '提升主人速度', 'pet_skill_speed.png'),
(2006, '雷电术', 1, 1, 40, 10, 0, '释放雷电攻击', 'pet_skill_thunder.png'),
(2007, '生命光环', 2, 7, 20, 15, 10, '持续恢复主人生命值', 'pet_skill_regen.png'),
(2008, '攻击光环', 2, 1, 10, 20, 15, '提升主人攻击力', 'pet_skill_attack_buff.png');

-- ==================== 坐骑配置数据 ====================
INSERT INTO mount_config (id, name, type, quality, speed_bonus, hp_bonus, attack_bonus, defense_bonus, skill_slots, obtain_way, description, icon, model) VALUES
(3001, '棕色马', 1, 1, 100, 50, 20, 20, 2, '新手任务', '普通的棕色马，适合新手', 'mount_horse.png', 'model/horse_brown.model'),
(3002, '黑马', 1, 2, 120, 80, 30, 30, 2, '商店购买', '黑色的战马，速度更快', 'mount_black_horse.png', 'model/horse_black.model'),
(3003, '狮鹫', 2, 3, 150, 100, 50, 40, 3, '副本掉落', '飞行坐骑，速度极快', 'mount_griffin.png', 'model/griffin.model'),
(3004, '独角兽坐骑', 2, 4, 180, 150, 80, 60, 3, '活动奖励', '稀有的独角兽坐骑', 'mount_unicorn.png', 'model/unicorn.model'),
(3005, '骨龙', 3, 5, 200, 200, 100, 80, 4, '传说任务', '传说中的骨龙坐骑', 'mount_bone_dragon.png', 'model/bone_dragon.model');

-- 坐骑技能配置数据
INSERT INTO mount_skill_config (id, name, type, effect_type, effect_value, cooldown, duration, description, icon) VALUES
(4001, '冲锋', 1, 5, 50, 30, 5, '短距离快速移动', 'mount_skill_charge.png'),
(4002, '生命恢复', 2, 7, 50, 45, 10, '恢复主人生命值', 'mount_skill_heal.png'),
(4003, '防御壁垒', 2, 3, 20, 60, 15, '提升主人防御力', 'mount_skill_defense.png'),
(4004, '攻击增幅', 2, 1, 20, 60, 15, '提升主人攻击力', 'mount_skill_attack.png'),
(4005, '速度爆发', 2, 5, 30, 40, 8, '短时间提升移动速度', 'mount_skill_speed.png');

-- ==================== 翅膀配置数据 ====================
INSERT INTO wing_config (id, name, type, quality, hp_bonus, attack_bonus, defense_bonus, speed_bonus, skill_slots, obtain_way, description, icon, model) VALUES
(5001, '新手翅膀', 1, 1, 100, 50, 30, 20, 2, '新手任务', '基础翅膀，提供少量属性', 'wing_basic.png', 'model/wing_basic.model'),
(5002, '天使翅膀', 1, 2, 150, 80, 50, 30, 2, '商店购买', '白色的天使翅膀', 'wing_angel.png', 'model/wing_angel.model'),
(5003, '恶魔翅膀', 2, 3, 200, 120, 80, 40, 3, '副本掉落', '黑色的恶魔翅膀', 'wing_demon.png', 'model/wing_demon.model'),
(5004, '凤凰之翼', 2, 4, 250, 150, 100, 50, 3, '活动奖励', '火焰凤凰翅膀', 'wing_phoenix.png', 'model/wing_phoenix.model'),
(5005, '圣灵之翼', 3, 5, 300, 200, 150, 60, 4, '传说任务', '传说中的圣灵翅膀', 'wing_holy.png', 'model/wing_holy.model');

-- 翅膀技能配置数据
INSERT INTO wing_skill_config (id, name, type, effect_type, effect_value, cooldown, duration, description, icon) VALUES
(6001, '飞行', 1, 5, 100, 0, 0, '允许飞行', 'wing_skill_fly.png'),
(6002, '生命守护', 2, 3, 20, 30, 10, '提升主人防御力', 'wing_skill_defense.png'),
(6003, '攻击祝福', 2, 1, 20, 30, 10, '提升主人攻击力', 'wing_skill_attack.png'),
(6004, '生命恢复', 2, 7, 50, 40, 15, '恢复主人生命值', 'wing_skill_heal.png'),
(6005, '速度提升', 2, 5, 30, 25, 8, '提升主人移动速度', 'wing_skill_speed.png');

-- ==================== 称号配置数据 ====================
INSERT INTO title_config (id, name, type, hp_bonus, attack_bonus, defense_bonus, speed_bonus, duration, obtain_condition, description, icon) VALUES
(7001, '新手 adventurer', 1, 50, 20, 20, 10, 0, '完成新手引导', '勇敢的新手 adventurer', 'title_beginner.png'),
(7002, '战场英雄', 2, 100, 50, 50, 20, 86400, '在竞技场获得10场胜利', '战场上的英雄', 'title_hero.png'),
(7003, '副本大师', 1, 80, 40, 40, 15, 0, '完成100次副本', '副本挑战大师', 'title_dungeon.png'),
(7004, '公会会长', 3, 150, 80, 80, 30, 0, '创建或成为公会会长', '公会的领导者', 'title_guild_leader.png'),
(7005, '节日使者', 4, 60, 30, 30, 15, 604800, '参与节日活动', '节日的使者', 'title_festival.png'),
(7006, '传说战士', 5, 200, 100, 100, 40, 0, '达到60级', '传说中的战士', 'title_legend.png');
