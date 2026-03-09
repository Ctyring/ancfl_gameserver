#include "skill_module.h"
#include "proto/msg_battle.pb.h"

namespace game_server {

SkillModule::SkillModule(LogicService* service) : service_(service) {
    // 初始化技能配置
    // TODO: 从配置文件加载技能配置
}

SkillModule::~SkillModule() {
}

bool SkillModule::InitSkills(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 检查技能是否已初始化
    auto it = skill_cache_.find(role_id);
    if (it != skill_cache_.end()) {
        return true;
    }
    
    // 初始化技能列表
    skill_cache_[role_id] = std::vector<SkillInfo>();
    
    LOG_INFO("Skills initialized: role_id=%llu", role_id);
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

bool SkillModule::GetSkillInfo(uint64_t role_id, int32_t skill_config_id, SkillInfo& info) {
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
            LOG_ERROR("Skill already learned: role_id=%llu, skill_config_id=%d", role_id, skill_config_id);
            return false;
        }
    }
    
    // 获取技能配置
    SkillConfig config;
    if (!GetSkillConfig(skill_config_id, config)) {
        LOG_ERROR("Skill config not found: skill_config_id=%d", skill_config_id);
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
    
    LOG_INFO("Skill learned: role_id=%llu, skill_config_id=%d, skill_id=%llu", role_id, skill_config_id, info.skill_id);
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
                LOG_ERROR("Skill level max: role_id=%llu, skill_config_id=%d, level=%d", role_id, skill_config_id, skill.level);
                return false;
            }
            
            // TODO: 检查升级材料和金币
            
            // 升级技能
            skill.level++;
            
            LOG_INFO("Skill upgraded: role_id=%llu, skill_config_id=%d, level=%d", role_id, skill_config_id, skill.level);
            return true;
        }
    }
    
    LOG_ERROR("Skill not learned: role_id=%llu, skill_config_id=%d", role_id, skill_config_id);
    return false;
}

bool SkillModule::ForgetSkill(uint64_t role_id, int32_t skill_config_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = skill_cache_.find(role_id);
    if (it == skill_cache_.end()) {
        return false;
    }
    
    // 查找并删除技能
    auto skill_it = std::remove_if(it->second.begin(), it->second.end(),
        [skill_config_id](const SkillInfo& skill) {
            return skill.skill_config_id == skill_config_id;
        });
    
    if (skill_it == it->second.end()) {
        LOG_ERROR("Skill not found: role_id=%llu, skill_config_id=%d", role_id, skill_config_id);
        return false;
    }
    
    it->second.erase(skill_it, it->second.end());
    
    LOG_INFO("Skill forgotten: role_id=%llu, skill_config_id=%d", role_id, skill_config_id);
    return true;
}

bool SkillModule::UseSkill(uint64_t role_id, int32_t skill_config_id, uint64_t target_id, float target_x, float target_z) {
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
    if (target_id > 0 && !IsTargetInRange(role_id, target_id, config.cast_range)) {
        LOG_ERROR("Target out of range: role_id=%llu, target_id=%llu, range=%d", role_id, target_id, config.cast_range);
        return false;
    }
    
    // 获取技能目标
    std::vector<uint64_t> targets;
    if (config.target_type == SkillTargetType::SINGLE) {
        if (target_id > 0) {
            targets.push_back(target_id);
        }
    } else {
        GetSkillTargets(role_id, skill_config_id, target_x, target_z, targets);
    }
    
    // 应用技能效果
    for (uint64_t target : targets) {
        // 计算伤害
        if (config.damage > 0) {
            int32_t damage = 0;
            CalculateSkillDamage(role_id, skill_config_id, target, damage);
            // TODO: 应用伤害到目标
        }
        
        // 计算治疗
        if (config.heal > 0) {
            int32_t heal = 0;
            CalculateSkillHeal(role_id, skill_config_id, target, heal);
            // TODO: 应用治疗到目标
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
                    break;
                }
            }
        }
    }
    
    LOG_INFO("Skill used: role_id=%llu, skill_config_id=%d, target_count=%d", role_id, skill_config_id, targets.size());
    return true;
}

bool SkillModule::CanUseSkill(uint64_t role_id, int32_t skill_config_id) {
    // 检查技能是否存在
    SkillInfo info;
    if (!GetSkillInfo(role_id, skill_config_id, info)) {
        LOG_ERROR("Skill not learned: role_id=%llu, skill_config_id=%d", role_id, skill_config_id);
        return false;
    }
    
    // 检查技能是否解锁
    if (!info.is_unlocked) {
        LOG_ERROR("Skill not unlocked: role_id=%llu, skill_config_id=%d", role_id, skill_config_id);
        return false;
    }
    
    // 检查技能是否在冷却中
    if (IsSkillInCooldown(role_id, skill_config_id)) {
        LOG_ERROR("Skill in cooldown: role_id=%llu, skill_config_id=%d", role_id, skill_config_id);
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

int32_t SkillModule::GetSkillCooldown(uint64_t role_id, int32_t skill_config_id) {
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

bool SkillModule::CalculateSkillDamage(uint64_t role_id, int32_t skill_config_id, uint64_t target_id, int32_t& damage) {
    // 获取技能配置
    SkillConfig config;
    if (!GetSkillConfig(skill_config_id, config)) {
        return false;
    }
    
    // 基础伤害
    int32_t base_damage = config.damage;
    
    // TODO: 根据角色属性计算最终伤害
    // TODO: 根据目标防御计算实际伤害
    
    damage = base_damage;
    
    return true;
}

bool SkillModule::CalculateSkillHeal(uint64_t role_id, int32_t skill_config_id, uint64_t target_id, int32_t& heal) {
    // 获取技能配置
    SkillConfig config;
    if (!GetSkillConfig(skill_config_id, config)) {
        return false;
    }
    
    // 基础治疗
    int32_t base_heal = config.heal;
    
    // TODO: 根据角色属性计算最终治疗
    
    heal = base_heal;
    
    return true;
}

bool SkillModule::ApplySkillEffects(uint64_t role_id, int32_t skill_config_id, uint64_t target_id) {
    // 获取技能配置
    SkillConfig config;
    if (!GetSkillConfig(skill_config_id, config)) {
        return false;
    }
    
    // 应用技能效果
    for (const auto& effect : config.effects) {
        // TODO: 应用效果到目标
        // 例如：添加Buff、Debuff等
    }
    
    return true;
}

bool SkillModule::GetSkillTargets(uint64_t role_id, int32_t skill_config_id, float target_x, float target_z, std::vector<uint64_t>& targets) {
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
        break;
        
    case SkillTargetType::AOE:
        // TODO: 获取范围内的所有目标
        break;
        
    case SkillTargetType::LINE:
        // TODO: 获取直线上的所有目标
        break;
        
    case SkillTargetType::SECTOR:
        // TODO: 获取扇形区域内的所有目标
        break;
        
    default:
        break;
    }
    
    return true;
}

bool SkillModule::IsTargetInRange(uint64_t role_id, uint64_t target_id, int32_t range) {
    // TODO: 检查目标是否在范围内
    // 需要获取角色和目标的位置信息
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
    
    service_->SendMsgToDBServer(static_cast<uint32_t>(MessageID::MSG_SKILL_DATA_SYNC_REQ), req);
    
    return true;
}

bool SkillModule::GetSkillConfig(int32_t skill_config_id, SkillConfig& config) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = skill_configs_.find(skill_config_id);
    if (it == skill_configs_.end()) {
        // TODO: 从配置表加载技能配置
        return false;
    }
    
    config = it->second;
    return true;
}

uint64_t SkillModule::GenerateSkillId() {
    static uint64_t next_id = time(nullptr) * 10000 + rand() % 10000;
    return next_id++;
}

} // namespace game_server
