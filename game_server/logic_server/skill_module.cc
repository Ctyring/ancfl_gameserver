#include "skill_module.h"
#include "ancfl/ancfl.h"
#include "buff_module.h"
#include "config_manager.h"
#include "proto/msg_battle.pb.h"

namespace game_server {

SkillModule::SkillModule(LogicService* service)
    : service_(service), buff_module_(service->GetBuffModule()) {
    LoadSkillConfigs();
}

SkillModule::~SkillModule() {}

bool SkillModule::InitSkills(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    // 检查技能是否已初始化
    auto it = skill_cache_.find(role_id);
    if (it != skill_cache_.end()) {
        return true;
    }

    // 初始化技能列表
    skill_cache_[role_id] = std::vector<SkillInfo>();

    ANCFL_ANCFL_LOG_INFO(ANCFL_LOG_ROOT())(ANCFL_LOG_ROOT())
        << "Skills initialized: role_id=" << role_id;
    return true;
}

bool SkillModule::GetSkills(uint64_t role_id, std::vector<SkillInfo>& skills) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = skill_cache_.find(role_id);
    if (it == skill_cache_.end()) {
        return false;
    }

    skills = it->second;
    return true;
}

bool SkillModule::GetSkillInfo(uint64_t role_id,
                               int32_t skill_config_id,
                               SkillInfo& info) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = skill_cache_.find(role_id);
    if (it == skill_cache_.end()) {
        return false;
    }

    for (const auto& skill : it->second) {
        if (skill.skill_config_id == skill_config_id) {
            info = skill;
            return true;
        }
    }

    return false;
}

bool SkillModule::LearnSkill(uint64_t role_id, int32_t skill_config_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = skill_cache_.find(role_id);
    if (it == skill_cache_.end()) {
        return false;
    }

    // 检查技能是否已学习
    for (const auto& skill : it->second) {
        if (skill.skill_config_id == skill_config_id) {
            ANCFL_ANCFL_LOG_ERROR(ANCFL_LOG_ROOT())(ANCFL_LOG_ROOT())
                << "Skill already learned: role_id=" << role_id
                << ", skill_config_id=" << skill_config_id;
            return false;
        }
    }

    // 获取技能配置
    SkillConfig config;
    if (!GetSkillConfig(skill_config_id, config)) {
        ANCFL_ANCFL_LOG_ERROR(ANCFL_LOG_ROOT())(ANCFL_LOG_ROOT())
            << "Skill config not found: skill_config_id=" << skill_config_id;
        return false;
    }

    // 创建技能信息
    SkillInfo info;
    info.skill_id = GenerateSkillId();
    info.skill_config_id = skill_config_id;
    info.level = 1;
    info.exp = 0;
    info.cooldown_time = config.cooldown;
    info.last_use_time = 0;
    info.is_unlocked = true;

    it->second.push_back(info);

    ANCFL_ANCFL_LOG_INFO(ANCFL_LOG_ROOT())(ANCFL_LOG_ROOT())
        << "Skill learned: role_id=" << role_id
        << ", skill_config_id=" << skill_config_id
        << ", skill_id=" << info.skill_id;
    return true;
}

bool SkillModule::UpgradeSkill(uint64_t role_id, int32_t skill_config_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = skill_cache_.find(role_id);
    if (it == skill_cache_.end()) {
        return false;
    }

    // 查找技能
    for (auto& skill : it->second) {
        if (skill.skill_config_id == skill_config_id) {
            // 检查技能等级上限
            if (skill.level >= 10) {
                ANCFL_ANCFL_LOG_ERROR(ANCFL_LOG_ROOT())(ANCFL_LOG_ROOT())
                    << "Skill level max: role_id=" << role_id
                    << ", skill_config_id=" << skill_config_id
                    << ", level=" << skill.level;
                return false;
            }

            // TODO: 检查升级材料和金币

            // 升级技能
            skill.level++;

            ANCFL_ANCFL_LOG_INFO(ANCFL_LOG_ROOT())(ANCFL_LOG_ROOT())
                << "Skill upgraded: role_id=" << role_id
                << ", skill_config_id=" << skill_config_id
                << ", level=" << skill.level;
            return true;
        }
    }

    ANCFL_ANCFL_LOG_ERROR(ANCFL_LOG_ROOT())(ANCFL_LOG_ROOT())
        << "Skill not learned: role_id=" << role_id
        << ", skill_config_id=" << skill_config_id;
    return false;
}

bool SkillModule::ForgetSkill(uint64_t role_id, int32_t skill_config_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = skill_cache_.find(role_id);
    if (it == skill_cache_.end()) {
        return false;
    }

    // 查找并删除技能
    auto skill_it =
        std::remove_if(it->second.begin(), it->second.end(),
                       [skill_config_id](const SkillInfo& skill) {
                           return skill.skill_config_id == skill_config_id;
                       });

    if (skill_it == it->second.end()) {
        ANCFL_ANCFL_LOG_ERROR(ANCFL_LOG_ROOT())(ANCFL_LOG_ROOT())
            << "Skill not found: role_id=" << role_id
            << ", skill_config_id=" << skill_config_id;
        return false;
    }

    it->second.erase(skill_it, it->second.end());

    ANCFL_ANCFL_LOG_INFO(ANCFL_LOG_ROOT())(ANCFL_LOG_ROOT())
        << "Skill forgotten: role_id=" << role_id
        << ", skill_config_id=" << skill_config_id;
    return true;
}

bool SkillModule::UseSkill(uint64_t role_id,
                           int32_t skill_config_id,
                           uint64_t target_id,
                           float target_x,
                           float target_z) {
    // 检查是否可以使用技能
    if (!CanUseSkill(role_id, skill_config_id)) {
        return false;
    }

    // 获取技能配置
    SkillConfig config;
    if (!GetSkillConfig(skill_config_id, config)) {
        return false;
    }

    // 检查目标是否在范围内
    if (target_id > 0 &&
        !IsTargetInRange(role_id, target_id, config.cast_range)) {
        ANCFL_ANCFL_LOG_ERROR(ANCFL_LOG_ROOT())(ANCFL_LOG_ROOT())
            << "Target out of range: role_id=" << role_id
            << ", target_id=" << target_id << ", range=" << config.cast_range;
        return false;
    }

    // 检查MP是否足够
    // TODO: 从角色模块获取MP并检查
    // if (GetRoleMP(role_id) < config.mp_cost) {
    //     ANCFL_ANCFL_LOG_ERROR(ANCFL_LOG_ROOT())(ANCFL_LOG_ROOT()) << "MP not
    //     enough: role_id=" << role_id << ", need=" << config.mp_cost; return
    //     false;
    // }

    // 消耗MP
    // TODO: 扣减角色MP
    // DeductRoleMP(role_id, config.mp_cost);

    // 获取技能目标
    std::vector<uint64_t> targets;
    if (config.target_type == SkillTargetType::SINGLE) {
        if (target_id > 0) {
            targets.push_back(target_id);
            ANCFL_LOG_DEBUG(ANCFL_LOG_ROOT())
                << "Skill target: single, target_id=" << target_id;
        }
    } else {
        GetSkillTargets(role_id, skill_config_id, target_x, target_z, targets);
    }

    // 应用技能效果
    for (uint64_t target : targets) {
        // 计算伤害
        if (config.damage > 0) {
            int32_t damage = 0;
            if (CalculateSkillDamage(role_id, skill_config_id, target,
                                     damage)) {
                // TODO: 应用伤害到目标
                ANCFL_LOG_DEBUG(ANCFL_LOG_ROOT())
                    << "Skill damage: role_id=" << role_id
                    << ", target_id=" << target << ", damage=" << damage;
            }
        }

        // 计算治疗
        if (config.heal > 0) {
            int32_t heal = 0;
            if (CalculateSkillHeal(role_id, skill_config_id, target, heal)) {
                // TODO: 应用治疗到目标
                ANCFL_LOG_DEBUG(ANCFL_LOG_ROOT())
                    << "Skill heal: role_id=" << role_id
                    << ", target_id=" << target << ", heal=" << heal;
            }
        }

        // 应用技能效果
        if (!config.effects.empty()) {
            ApplySkillEffects(role_id, skill_config_id, target);
        }
    }

    // 设置技能冷却
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = skill_cache_.find(role_id);
        if (it != skill_cache_.end()) {
            for (auto& skill : it->second) {
                if (skill.skill_config_id == skill_config_id) {
                    skill.last_use_time = time(nullptr);
                    skill.cooldown_time = config.cooldown;
                    ANCFL_LOG_DEBUG(ANCFL_LOG_ROOT())
                        << "Skill cooldown set: role_id=" << role_id
                        << ", skill_config_id=" << skill_config_id
                        << ", cooldown=" << config.cooldown;
                    break;
                }
            }
        }
    }

    ANCFL_ANCFL_LOG_INFO(ANCFL_LOG_ROOT())(ANCFL_LOG_ROOT())
        << "Skill used: role_id=" << role_id
        << ", skill_config_id=" << skill_config_id
        << ", target_count=" << targets.size();
    return true;
}

bool SkillModule::CanUseSkill(uint64_t role_id, int32_t skill_config_id) {
    // 检查技能是否存在
    SkillInfo info;
    if (!GetSkillInfo(role_id, skill_config_id, info)) {
        ANCFL_ANCFL_LOG_ERROR(ANCFL_LOG_ROOT())(ANCFL_LOG_ROOT())
            << "Skill not learned: role_id=" << role_id
            << ", skill_config_id=" << skill_config_id;
        return false;
    }

    // 检查技能是否解锁
    if (!info.is_unlocked) {
        ANCFL_ANCFL_LOG_ERROR(ANCFL_LOG_ROOT())(ANCFL_LOG_ROOT())
            << "Skill not unlocked: role_id=" << role_id
            << ", skill_config_id=" << skill_config_id;
        return false;
    }

    // 检查技能是否在冷却中
    if (IsSkillInCooldown(role_id, skill_config_id)) {
        ANCFL_ANCFL_LOG_ERROR(ANCFL_LOG_ROOT())(ANCFL_LOG_ROOT())
            << "Skill in cooldown: role_id=" << role_id
            << ", skill_config_id=" << skill_config_id;
        return false;
    }

    // TODO: 检查MP是否足够

    return true;
}

bool SkillModule::IsSkillInCooldown(uint64_t role_id, int32_t skill_config_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = skill_cache_.find(role_id);
    if (it == skill_cache_.end()) {
        return false;
    }

    for (const auto& skill : it->second) {
        if (skill.skill_config_id == skill_config_id) {
            if (skill.last_use_time == 0) {
                return false;
            }

            time_t now = time(nullptr);
            return (now - skill.last_use_time) < skill.cooldown_time;
        }
    }

    return false;
}

int32_t SkillModule::GetSkillCooldown(uint64_t role_id,
                                      int32_t skill_config_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = skill_cache_.find(role_id);
    if (it == skill_cache_.end()) {
        return 0;
    }

    for (const auto& skill : it->second) {
        if (skill.skill_config_id == skill_config_id) {
            if (skill.last_use_time == 0) {
                return 0;
            }

            time_t now = time(nullptr);
            int32_t elapsed = now - skill.last_use_time;
            if (elapsed >= skill.cooldown_time) {
                return 0;
            }

            return skill.cooldown_time - elapsed;
        }
    }

    return 0;
}

bool SkillModule::CalculateSkillDamage(uint64_t role_id,
                                       int32_t skill_config_id,
                                       uint64_t target_id,
                                       int32_t& damage) {
    // 获取技能配置
    SkillConfig config;
    if (!GetSkillConfig(skill_config_id, config)) {
        return false;
    }

    // 基础伤害
    int32_t base_damage = config.damage;

    // 计算最终伤害
    int32_t final_damage = base_damage;

    // TODO: 从角色模块获取攻击力属性
    // int32_t attack = GetRoleAttack(role_id);
    // final_damage = base_damage * (1 + attack / 1000.0f);

    // TODO: 从角色模块获取目标防御属性
    // int32_t defense = GetTargetDefense(target_id);
    // final_damage = final_damage * (1 - defense / 2000.0f);
    // final_damage = std::max(1, final_damage); // 确保至少造成1点伤害

    // 暴击计算
    // TODO: 从角色模块获取暴击率
    // float crit_rate = GetRoleCritRate(role_id);
    // if (rand() % 10000 < crit_rate * 10000) {
    //     // TODO: 从角色模块获取暴击伤害倍率
    //     float crit_damage = GetRoleCritDamage(role_id);
    //     final_damage = final_damage * crit_damage;
    //     ANCFL_LOG_DEBUG(ANCFL_LOG_ROOT()) << "Skill crit: role_id=" <<
    //     role_id << ", damage=" << final_damage;
    // }

    damage = final_damage;
    ANCFL_LOG_DEBUG(ANCFL_LOG_ROOT())
        << "Calculate skill damage: role_id=" << role_id
        << ", skill_config_id=" << skill_config_id
        << ", target_id=" << target_id << ", damage=" << damage;

    return true;
}

bool SkillModule::CalculateSkillHeal(uint64_t role_id,
                                     int32_t skill_config_id,
                                     uint64_t target_id,
                                     int32_t& heal) {
    // 获取技能配置
    SkillConfig config;
    if (!GetSkillConfig(skill_config_id, config)) {
        return false;
    }

    // 基础治疗
    int32_t base_heal = config.heal;

    // 计算最终治疗
    int32_t final_heal = base_heal;

    // TODO: 从角色模块获取治疗强度属性
    // int32_t heal_power = GetRoleHealPower(role_id);
    // final_heal = base_heal * (1 + heal_power / 1000.0f);

    // TODO: 检查是否有治疗加成效果
    // int32_t heal_bonus = GetRoleHealBonus(role_id);
    // final_heal = final_heal * (1 + heal_bonus / 1000.0f);

    heal = final_heal;
    ANCFL_LOG_DEBUG(ANCFL_LOG_ROOT())
        << "Calculate skill heal: role_id=" << role_id
        << ", skill_config_id=" << skill_config_id
        << ", target_id=" << target_id << ", heal=" << heal;

    return true;
}

bool SkillModule::ApplySkillEffects(uint64_t role_id,
                                    int32_t skill_config_id,
                                    uint64_t target_id) {
    // 获取技能配置
    SkillConfig config;
    if (!GetSkillConfig(skill_config_id, config)) {
        return false;
    }

    // 应用技能效果
    for (const auto& effect : config.effects) {
        ANCFL_LOG_DEBUG(ANCFL_LOG_ROOT())
            << "Apply skill effect: role_id=" << role_id
            << ", skill_config_id=" << skill_config_id
            << ", target_id=" << target_id
            << ", effect_type=" << effect.effect_type
            << ", effect_value=" << effect.effect_value
            << ", duration=" << effect.duration
            << ", interval=" << effect.interval;

        // 根据效果类型应用不同的效果
        int32_t buff_config_id = 0;
        switch (effect.effect_type) {
            case 1:                     // 伤害Buff
                buff_config_id = 1001;  // 假设伤害Buff的配置ID为1001
                break;
            case 2:                     // 防御Buff
                buff_config_id = 1002;  // 假设防御Buff的配置ID为1002
                break;
            case 3:                     // 速度Buff
                buff_config_id = 1003;  // 假设速度Buff的配置ID为1003
                break;
            case 4:                     // 减速Debuff
                buff_config_id = 2001;  // 假设减速Debuff的配置ID为2001
                break;
            case 5:                     // 眩晕Debuff
                buff_config_id = 2002;  // 假设眩晕Debuff的配置ID为2002
                break;
            default:
                ANCFL_LOG_WARN(ANCFL_LOG_ROOT())
                    << "Unknown effect type: " << effect.effect_type;
                continue;
        }

        // 添加Buff到目标
        if (buff_config_id > 0) {
            if (!buff_module_->AddBuff(target_id, role_id, buff_config_id)) {
                ANCFL_ANCFL_LOG_ERROR(ANCFL_LOG_ROOT())(ANCFL_LOG_ROOT())
                    << "Failed to add buff: target_id=" << target_id
                    << ", caster_id=" << role_id
                    << ", buff_config_id=" << buff_config_id;
            }
        }
    }

    return true;
}

bool SkillModule::GetSkillTargets(uint64_t role_id,
                                  int32_t skill_config_id,
                                  float target_x,
                                  float target_z,
                                  std::vector<uint64_t>& targets) {
    // 获取技能配置
    SkillConfig config;
    if (!GetSkillConfig(skill_config_id, config)) {
        return false;
    }

    targets.clear();

    // 根据目标类型获取目标
    switch (config.target_type) {
        case SkillTargetType::SELF:
            targets.push_back(role_id);
            ANCFL_LOG_DEBUG(ANCFL_LOG_ROOT())
                << "Skill target: self, role_id=" << role_id;
            break;

        case SkillTargetType::AOE:
            // TODO: 获取范围内的所有目标
            ANCFL_LOG_DEBUG(ANCFL_LOG_ROOT())
                << "Skill target: AOE, center=({}, {})" << target_x << target_z;
            break;

        case SkillTargetType::LINE:
            // TODO: 获取直线上的所有目标
            ANCFL_LOG_DEBUG(ANCFL_LOG_ROOT())
                << "Skill target: LINE, start=role, end=({}, {})" << target_x
                << target_z;
            break;

        case SkillTargetType::SECTOR:
            // TODO: 获取扇形区域内的所有目标
            ANCFL_LOG_DEBUG(ANCFL_LOG_ROOT())
                << "Skill target: SECTOR, center=role, direction=({}, {})"
                << target_x << target_z;
            break;

        default:
            break;
    }

    return true;
}

bool SkillModule::IsTargetInRange(uint64_t role_id,
                                  uint64_t target_id,
                                  int32_t range) {
    // TODO: 从场景模块获取角色和目标的位置信息
    // 暂时返回true，假设目标在范围内
    ANCFL_LOG_DEBUG(ANCFL_LOG_ROOT())
        << "Check target in range: role_id=" << role_id
        << ", target_id=" << target_id << ", range=" << range;
    return true;
}

bool SkillModule::LoadSkillData(uint64_t role_id) {
    // 从数据库加载技能数据
    // TODO: 实现从数据库加载技能数据

    // 初始化技能
    InitSkills(role_id);

    return true;
}

bool SkillModule::SaveSkillData(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = skill_cache_.find(role_id);
    if (it == skill_cache_.end()) {
        return false;
    }

    // 保存技能数据到数据库
    msg_battle::SkillDataSyncReq req;
    req.set_role_id(role_id);

    for (const auto& skill : it->second) {
        auto skill_data = req.add_skills();
        skill_data->set_skill_id(skill.skill_id);
        skill_data->set_skill_config_id(skill.skill_config_id);
        skill_data->set_level(skill.level);
        skill_data->set_exp(skill.exp);
        skill_data->set_is_unlocked(skill.is_unlocked);
    }

    service_->SendMsgToDBServer(
        static_cast<uint32_t>(MessageID::MSG_SKILL_DATA_SYNC_REQ), req);

    return true;
}

bool SkillModule::GetSkillConfig(int32_t skill_config_id, SkillConfig& config) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = skill_configs_.find(skill_config_id);
    if (it != skill_configs_.end()) {
        config = it->second;
        return true;
    }

    // TODO: 从配置表加载技能配置
    // 暂时返回默认配置
    config.skill_config_id = skill_config_id;
    config.skill_name = "Skill_" + std::to_string(skill_config_id);
    config.type = SkillType::ACTIVE;
    config.target_type = SkillTargetType::SINGLE;
    config.cast_range = 100;
    config.aoe_radius = 20;
    config.cooldown = 5;
    config.mp_cost = 10;
    config.damage = 100;
    config.heal = 0;

    // 保存到缓存
    skill_configs_[skill_config_id] = config;

    return true;
}

uint64_t SkillModule::GenerateSkillId() {
    static uint64_t next_id = time(nullptr) * 10000 + rand() % 10000;
    return next_id++;
}

bool SkillModule::LoadSkillConfigs() {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    if (!g_config_manager) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT())("Config manager not initialized");
        return false;
    }

    if (!g_config_manager->HasConfig("skill_config")) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT())(
            "Skill config table not found in database, please run "
            "insert_config_data.sql first");
        return false;
    }

    std::vector<ConfigRow> rows;
    if (!g_config_manager->GetConfigRows("skill_config", rows)) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT())("Failed to load skill config rows");
        return false;
    }

    skill_configs_.clear();
    for (const auto& row : rows) {
        SkillConfig config;

        int32_t config_id = 0;
        for (const auto& pair : row) {
            if (pair.first == "id") {
                config_id = pair.second.int_val;
                break;
            }
        }

        config.skill_config_id = config_id;
        config.skill_name =
            CONFIG_GET_STRING("skill_config", config_id, "name", "");
        config.type = static_cast<SkillType>(
            CONFIG_GET_INT("skill_config", config_id, "type", 1));
        config.target_type = static_cast<SkillTargetType>(
            CONFIG_GET_INT("skill_config", config_id, "target_type", 2));
        config.cast_range =
            CONFIG_GET_INT("skill_config", config_id, "cast_range", 5);
        config.aoe_radius =
            CONFIG_GET_INT("skill_config", config_id, "aoe_radius", 0);
        config.cooldown =
            CONFIG_GET_INT("skill_config", config_id, "cooldown", 1);
        config.mp_cost =
            CONFIG_GET_INT("skill_config", config_id, "mp_cost", 0);
        config.damage = CONFIG_GET_INT("skill_config", config_id, "damage", 0);
        config.heal = CONFIG_GET_INT("skill_config", config_id, "heal", 0);
        config.effects.clear();

        skill_configs_[config.skill_config_id] = config;
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT())("Loaded %d skill configs from database",
                                     skill_configs_.size());
    return true;
}

}  // namespace game_server
