#include "buff_module.h"
#include "proto/msg_battle.pb.h"

namespace game_server {

BuffModule::BuffModule(LogicService* service) : service_(service) {
    // 初始化Buff配置
    // TODO: 从配置文件加载Buff配置
}

BuffModule::~BuffModule() {
}

bool BuffModule::AddBuff(uint64_t target_id, uint64_t caster_id, int32_t buff_config_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 获取Buff配置
    BuffConfig config;
    if (!GetBuffConfig(buff_config_id, config)) {
        LOG_ERROR("Buff config not found: buff_config_id=%d", buff_config_id);
        return false;
    }
    
    // 检查是否已有相同Buff
    auto it = buff_cache_.find(target_id);
    if (it != buff_cache_.end()) {
        for (auto& buff : it->second) {
            if (buff.buff_config_id == buff_config_id && buff.is_active) {
                // 堆叠Buff
                if (buff.stack_count < buff.max_stack) {
                    StackBuff(target_id, buff.buff_id);
                } else {
                    // 刷新Buff时间
                    RefreshBuff(target_id, buff.buff_id);
                }
                return true;
            }
        }
    } else {
        buff_cache_[target_id] = std::vector<BuffInfo>();
        it = buff_cache_.find(target_id);
    }
    
    // 创建新Buff
    BuffInfo buff;
    buff.buff_id = GenerateBuffId();
    buff.buff_config_id = buff_config_id;
    buff.caster_id = caster_id;
    buff.target_id = target_id;
    buff.type = config.type;
    buff.effect_type = config.effect_type;
    buff.effect_value = config.effect_value;
    buff.duration = config.duration;
    buff.interval = config.interval;
    buff.stack_count = 1;
    buff.max_stack = config.max_stack;
    buff.start_time = time(nullptr);
    buff.end_time = buff.start_time + config.duration;
    buff.is_active = true;
    
    it->second.push_back(buff);
    
    // 应用Buff效果
    ApplySingleBuffEffect(target_id, buff);
    
    LOG_INFO("Buff added: target_id=%llu, buff_id=%llu, buff_config_id=%d", target_id, buff.buff_id, buff_config_id);
    return true;
}

bool BuffModule::RemoveBuff(uint64_t target_id, uint64_t buff_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = buff_cache_.find(target_id);
    if (it == buff_cache_.end()) {
        return false;
    }
    
    // 查找并移除Buff
    auto buff_it = std::find_if(it->second.begin(), it->second.end(),
        [buff_id](const BuffInfo& buff) {
            return buff.buff_id == buff_id;
        });
    
    if (buff_it == it->second.end()) {
        LOG_ERROR("Buff not found: target_id=%llu, buff_id=%llu", target_id, buff_id);
        return false;
    }
    
    // 移除Buff效果
    RemoveSingleBuffEffect(target_id, *buff_it);
    
    // 移除Buff
    it->second.erase(buff_it);
    
    LOG_INFO("Buff removed: target_id=%llu, buff_id=%llu", target_id, buff_id);
    return true;
}

bool BuffModule::RemoveBuffByConfigId(uint64_t target_id, int32_t buff_config_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = buff_cache_.find(target_id);
    if (it == buff_cache_.end()) {
        return false;
    }
    
    // 查找并移除Buff
    auto buff_it = std::find_if(it->second.begin(), it->second.end(),
        [buff_config_id](const BuffInfo& buff) {
            return buff.buff_config_id == buff_config_id;
        });
    
    if (buff_it == it->second.end()) {
        return false;
    }
    
    // 移除Buff效果
    RemoveSingleBuffEffect(target_id, *buff_it);
    
    // 移除Buff
    it->second.erase(buff_it);
    
    LOG_INFO("Buff removed by config id: target_id=%llu, buff_config_id=%d", target_id, buff_config_id);
    return true;
}

bool BuffModule::RemoveAllBuffs(uint64_t target_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = buff_cache_.find(target_id);
    if (it == buff_cache_.end()) {
        return false;
    }
    
    // 移除所有Buff效果
    for (const auto& buff : it->second) {
        RemoveSingleBuffEffect(target_id, buff);
    }
    
    // 清空Buff列表
    it->second.clear();
    
    LOG_INFO("All buffs removed: target_id=%llu", target_id);
    return true;
}

bool BuffModule::RemoveDebuffs(uint64_t target_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = buff_cache_.find(target_id);
    if (it == buff_cache_.end()) {
        return false;
    }
    
    // 移除所有Debuff
    auto buff_it = it->second.begin();
    while (buff_it != it->second.end()) {
        if (buff_it->type == BuffType::DEBUFF) {
            // 移除Buff效果
            RemoveSingleBuffEffect(target_id, *buff_it);
            buff_it = it->second.erase(buff_it);
        } else {
            ++buff_it;
        }
    }
    
    LOG_INFO("All debuffs removed: target_id=%llu", target_id);
    return true;
}

bool BuffModule::GetBuffs(uint64_t target_id, std::vector<BuffInfo>& buffs) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = buff_cache_.find(target_id);
    if (it == buff_cache_.end()) {
        return false;
    }
    
    buffs.clear();
    for (const auto& buff : it->second) {
        if (buff.is_active) {
            buffs.push_back(buff);
        }
    }
    
    return true;
}

bool BuffModule::GetBuffInfo(uint64_t target_id, uint64_t buff_id, BuffInfo& info) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = buff_cache_.find(target_id);
    if (it == buff_cache_.end()) {
        return false;
    }
    
    for (const auto& buff : it->second) {
        if (buff.buff_id == buff_id) {
            info = buff;
            return true;
        }
    }
    
    return false;
}

bool BuffModule::HasBuff(uint64_t target_id, int32_t buff_config_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = buff_cache_.find(target_id);
    if (it == buff_cache_.end()) {
        return false;
    }
    
    for (const auto& buff : it->second) {
        if (buff.buff_config_id == buff_config_id && buff.is_active) {
            return true;
        }
    }
    
    return false;
}

bool BuffModule::HasDebuff(uint64_t target_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = buff_cache_.find(target_id);
    if (it == buff_cache_.end()) {
        return false;
    }
    
    for (const auto& buff : it->second) {
        if (buff.type == BuffType::DEBUFF && buff.is_active) {
            return true;
        }
    }
    
    return false;
}

bool BuffModule::CalculateBuffEffects(uint64_t target_id, int32_t& attack_bonus, int32_t& defense_bonus, int32_t& speed_bonus) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    attack_bonus = 0;
    defense_bonus = 0;
    speed_bonus = 0;
    
    auto it = buff_cache_.find(target_id);
    if (it == buff_cache_.end()) {
        return true;
    }
    
    for (const auto& buff : it->second) {
        if (!buff.is_active) {
            continue;
        }
        
        int32_t value = buff.effect_value * buff.stack_count;
        
        switch (buff.effect_type) {
        case BuffEffectType::ATTACK_UP:
            attack_bonus += value;
            break;
        case BuffEffectType::ATTACK_DOWN:
            attack_bonus -= value;
            break;
        case BuffEffectType::DEFENSE_UP:
            defense_bonus += value;
            break;
        case BuffEffectType::DEFENSE_DOWN:
            defense_bonus -= value;
            break;
        case BuffEffectType::SPEED_UP:
            speed_bonus += value;
            break;
        case BuffEffectType::SPEED_DOWN:
            speed_bonus -= value;
            break;
        default:
            break;
        }
    }
    
    return true;
}

bool BuffModule::ApplyBuffEffects(uint64_t target_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = buff_cache_.find(target_id);
    if (it == buff_cache_.end()) {
        return false;
    }
    
    for (const auto& buff : it->second) {
        if (buff.is_active) {
            ApplySingleBuffEffect(target_id, buff);
        }
    }
    
    return true;
}

bool BuffModule::ProcessDotHot(uint64_t target_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = buff_cache_.find(target_id);
    if (it == buff_cache_.end()) {
        return false;
    }
    
    time_t now = time(nullptr);
    
    for (const auto& buff : it->second) {
        if (!buff.is_active) {
            continue;
        }
        
        // 检查是否是DOT或HOT
        if (buff.type == BuffType::DOT || buff.type == BuffType::HOT) {
            // 检查是否到了触发时间
            int32_t elapsed = now - buff.start_time;
            if (elapsed % buff.interval == 0) {
                if (buff.type == BuffType::DOT) {
                    // 应用DOT伤害
                    // TODO: 应用伤害到目标
                    LOG_INFO("DOT damage: target_id=%llu, buff_id=%llu, damage=%d", target_id, buff.buff_id, buff.effect_value);
                } else if (buff.type == BuffType::HOT) {
                    // 应用HOT治疗
                    // TODO: 应用治疗到目标
                    LOG_INFO("HOT heal: target_id=%llu, buff_id=%llu, heal=%d", target_id, buff.buff_id, buff.effect_value);
                }
            }
        }
    }
    
    return true;
}

bool BuffModule::StackBuff(uint64_t target_id, uint64_t buff_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = buff_cache_.find(target_id);
    if (it == buff_cache_.end()) {
        return false;
    }
    
    for (auto& buff : it->second) {
        if (buff.buff_id == buff_id) {
            if (buff.stack_count < buff.max_stack) {
                buff.stack_count++;
                LOG_INFO("Buff stacked: target_id=%llu, buff_id=%llu, stack_count=%d", target_id, buff_id, buff.stack_count);
            }
            return true;
        }
    }
    
    return false;
}

bool BuffModule::RefreshBuff(uint64_t target_id, uint64_t buff_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = buff_cache_.find(target_id);
    if (it == buff_cache_.end()) {
        return false;
    }
    
    for (auto& buff : it->second) {
        if (buff.buff_id == buff_id) {
            buff.start_time = time(nullptr);
            buff.end_time = buff.start_time + buff.duration;
            LOG_INFO("Buff refreshed: target_id=%llu, buff_id=%llu", target_id, buff_id);
            return true;
        }
    }
    
    return false;
}

void BuffModule::OnUpdate() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    time_t now = time(nullptr);
    
    // 检查所有目标的Buff
    for (auto& pair : buff_cache_) {
        uint64_t target_id = pair.first;
        auto& buffs = pair.second;
        
        // 检查每个Buff是否过期
        auto buff_it = buffs.begin();
        while (buff_it != buffs.end()) {
            if (buff_it->is_active && now >= buff_it->end_time) {
                // Buff过期，移除效果
                RemoveSingleBuffEffect(target_id, *buff_it);
                buff_it->is_active = false;
                
                LOG_INFO("Buff expired: target_id=%llu, buff_id=%llu", target_id, buff_it->buff_id);
                
                // 移除过期的Buff
                buff_it = buffs.erase(buff_it);
            } else {
                ++buff_it;
            }
        }
    }
}

void BuffModule::OnTimer() {
    // 处理DOT和HOT
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    for (auto& pair : buff_cache_) {
        uint64_t target_id = pair.first;
        ProcessDotHot(target_id);
    }
}

bool BuffModule::LoadBuffData(uint64_t target_id) {
    // 从数据库加载Buff数据
    // TODO: 实现从数据库加载Buff数据
    return true;
}

bool BuffModule::SaveBuffData(uint64_t target_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = buff_cache_.find(target_id);
    if (it == buff_cache_.end()) {
        return false;
    }
    
    // 保存Buff数据到数据库
    msg_battle::BuffDataSyncReq req;
    req.set_target_id(target_id);
    
    for (const auto& buff : it->second) {
        if (buff.is_active) {
            auto buff_data = req.add_buffs();
            buff_data->set_buff_id(buff.buff_id);
            buff_data->set_buff_config_id(buff.buff_config_id);
            buff_data->set_caster_id(buff.caster_id);
            buff_data->set_stack_count(buff.stack_count);
            buff_data->set_end_time(buff.end_time);
        }
    }
    
    service_->SendMsgToDBServer(static_cast<uint32_t>(MessageID::MSG_BUFF_DATA_SYNC_REQ), req);
    
    return true;
}

bool BuffModule::GetBuffConfig(int32_t buff_config_id, BuffConfig& config) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = buff_configs_.find(buff_config_id);
    if (it == buff_configs_.end()) {
        // TODO: 从配置表加载Buff配置
        return false;
    }
    
    config = it->second;
    return true;
}

uint64_t BuffModule::GenerateBuffId() {
    static uint64_t next_id = time(nullptr) * 10000 + rand() % 10000;
    return next_id++;
}

bool BuffModule::ApplySingleBuffEffect(uint64_t target_id, const BuffInfo& buff) {
    // TODO: 应用Buff效果到目标
    // 根据Buff类型和效果类型应用不同的效果
    
    switch (buff.effect_type) {
    case BuffEffectType::STUN:
        // TODO: 眩晕效果
        break;
    case BuffEffectType::SILENCE:
        // TODO: 沉默效果
        break;
    case BuffEffectType::SHIELD:
        // TODO: 护盾效果
        break;
    default:
        break;
    }
    
    return true;
}

bool BuffModule::RemoveSingleBuffEffect(uint64_t target_id, const BuffInfo& buff) {
    // TODO: 移除Buff效果
    // 根据Buff类型和效果类型移除不同的效果
    
    switch (buff.effect_type) {
    case BuffEffectType::STUN:
        // TODO: 移除眩晕效果
        break;
    case BuffEffectType::SILENCE:
        // TODO: 移除沉默效果
        break;
    case BuffEffectType::SHIELD:
        // TODO: 移除护盾效果
        break;
    default:
        break;
    }
    
    return true;
}

} // namespace game_server
