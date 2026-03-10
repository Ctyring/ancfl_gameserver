#include "equip_module.h"
#include "proto/msg_equip.pb.h"

namespace game_server {

EquipModule::EquipModule(LogicService* service) : service_(service) {
}

EquipModule::~EquipModule() {
}

bool EquipModule::InitEquip(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 检查装备是否已初始化
    auto it = equip_cache_.find(role_id);
    if (it != equip_cache_.end()) {
        return true;
    }
    
    // 初始化装备
    equip_cache_[role_id] = std::vector<EquipInfo>();
    worn_equips_[role_id] = std::unordered_map<int32_t, EquipInfo>();
    
    LOG_INFO("Equip initialized: role_id=%llu", role_id);
    return true;
}

bool EquipModule::GetEquipList(uint64_t role_id, std::vector<EquipInfo>& equips) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = equip_cache_.find(role_id);
    if (it == equip_cache_.end()) {
        return false;
    }
    
    equips = it->second;
    return true;
}

bool EquipModule::GetEquipInfo(uint64_t role_id, uint64_t equip_id, EquipInfo& info) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = equip_cache_.find(role_id);
    if (it == equip_cache_.end()) {
        return false;
    }
    
    for (const auto& equip : it->second) {
        if (equip.equip_id == equip_id) {
            info = equip;
            return true;
        }
    }
    
    return false;
}

bool EquipModule::WearEquip(uint64_t role_id, uint64_t equip_id, int32_t position) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = equip_cache_.find(role_id);
    if (it == equip_cache_.end()) {
        return false;
    }
    
    // 查找装备
    EquipInfo* equip = nullptr;
    for (auto& e : it->second) {
        if (e.equip_id == equip_id) {
            equip = &e;
            break;
        }
    }
    
    if (!equip) {
        LOG_ERROR("Equip not found: role_id=%llu, equip_id=%llu", role_id, equip_id);
        return false;
    }
    
    // 检查装备位置是否匹配
    if (equip->position != position) {
        LOG_ERROR("Equip position mismatch: role_id=%llu, equip_id=%llu, equip_pos=%d, target_pos=%d", 
                  role_id, equip_id, equip->position, position);
        return false;
    }
    
    // 检查装备位置是否可用
    auto worn_it = worn_equips_.find(role_id);
    if (worn_it != worn_equips_.end() && worn_it->second.find(position) != worn_it->second.end()) {
        LOG_ERROR("Equip position occupied: role_id=%llu, position=%d", role_id, position);
        return false;
    }
    
    // 穿戴装备
    worn_equips_[role_id][position] = *equip;
    
    LOG_INFO("Equip worn: role_id=%llu, equip_id=%llu, position=%d", role_id, equip_id, position);
    return true;
}

bool EquipModule::TakeOffEquip(uint64_t role_id, int32_t position) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto worn_it = worn_equips_.find(role_id);
    if (worn_it == worn_equips_.end()) {
        return false;
    }
    
    auto pos_it = worn_it->second.find(position);
    if (pos_it == worn_it->second.end()) {
        LOG_ERROR("No equip at position: role_id=%llu, position=%d", role_id, position);
        return false;
    }
    
    // 卸下装备
    worn_it->second.erase(pos_it);
    
    LOG_INFO("Equip taken off: role_id=%llu, position=%d", role_id, position);
    return true;
}

bool EquipModule::GetWornEquips(uint64_t role_id, std::vector<EquipInfo>& equips) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto worn_it = worn_equips_.find(role_id);
    if (worn_it == worn_equips_.end()) {
        return false;
    }
    
    equips.clear();
    for (const auto& pair : worn_it->second) {
        equips.push_back(pair.second);
    }
    
    return true;
}

bool EquipModule::StrengthenEquip(uint64_t role_id, uint64_t equip_id, int32_t level) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = equip_cache_.find(role_id);
    if (it == equip_cache_.end()) {
        return false;
    }
    
    // 查找装备
    for (auto& equip : it->second) {
        if (equip.equip_id == equip_id) {
            // 检查强化等级
            if (level <= equip.强化等级) {
                LOG_ERROR("Invalid strengthen level: role_id=%llu, equip_id=%llu, current=%d, target=%d", 
                          role_id, equip_id, equip.强化等级, level);
                return false;
            }
            
            // TODO: 检查强化材料和金币
            
            // 强化装备
            equip.强化等级 = level;
            
            // 如果装备已穿戴，更新已穿戴的装备
            auto worn_it = worn_equips_.find(role_id);
            if (worn_it != worn_equips_.end()) {
                for (auto& pair : worn_it->second) {
                    if (pair.second.equip_id == equip_id) {
                        pair.second.强化等级 = level;
                        break;
                    }
                }
            }
            
            LOG_INFO("Equip strengthened: role_id=%llu, equip_id=%llu, level=%d", role_id, equip_id, level);
            return true;
        }
    }
    
    LOG_ERROR("Equip not found: role_id=%llu, equip_id=%llu", role_id, equip_id);
    return false;
}

bool EquipModule::GetStrengthenLevel(uint64_t role_id, uint64_t equip_id, int32_t& level) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = equip_cache_.find(role_id);
    if (it == equip_cache_.end()) {
        return false;
    }
    
    for (const auto& equip : it->second) {
        if (equip.equip_id == equip_id) {
            level = equip.强化等级;
            return true;
        }
    }
    
    return false;
}

bool EquipModule::UpgradeStar(uint64_t role_id, uint64_t equip_id, int32_t star_level) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = equip_cache_.find(role_id);
    if (it == equip_cache_.end()) {
        return false;
    }
    
    // 查找装备
    for (auto& equip : it->second) {
        if (equip.equip_id == equip_id) {
            // 检查星级
            if (star_level <= equip.star_level) {
                LOG_ERROR("Invalid star level: role_id=%llu, equip_id=%llu, current=%d, target=%d", 
                          role_id, equip_id, equip.star_level, star_level);
                return false;
            }
            
            // TODO: 检查升星材料和金币
            
            // 升星
            equip.star_level = star_level;
            
            // 如果装备已穿戴，更新已穿戴的装备
            auto worn_it = worn_equips_.find(role_id);
            if (worn_it != worn_equips_.end()) {
                for (auto& pair : worn_it->second) {
                    if (pair.second.equip_id == equip_id) {
                        pair.second.star_level = star_level;
                        break;
                    }
                }
            }
            
            LOG_INFO("Equip star upgraded: role_id=%llu, equip_id=%llu, star_level=%d", role_id, equip_id, star_level);
            return true;
        }
    }
    
    LOG_ERROR("Equip not found: role_id=%llu, equip_id=%llu", role_id, equip_id);
    return false;
}

bool EquipModule::GetStarLevel(uint64_t role_id, uint64_t equip_id, int32_t& star_level) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = equip_cache_.find(role_id);
    if (it == equip_cache_.end()) {
        return false;
    }
    
    for (const auto& equip : it->second) {
        if (equip.equip_id == equip_id) {
            star_level = equip.star_level;
            return true;
        }
    }
    
    return false;
}

bool EquipModule::InlayGem(uint64_t role_id, uint64_t equip_id, int32_t gem_config_id, int32_t gem_index) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = equip_cache_.find(role_id);
    if (it == equip_cache_.end()) {
        return false;
    }
    
    // 查找装备
    for (auto& equip : it->second) {
        if (equip.equip_id == equip_id) {
            // 检查宝石索引
            if (gem_index < 0 || gem_index >= equip.gems.size()) {
                LOG_ERROR("Invalid gem index: role_id=%llu, equip_id=%llu, index=%d, size=%d", 
                          role_id, equip_id, gem_index, equip.gems.size());
                return false;
            }
            
            // TODO: 检查宝石是否存在
            
            // 镶嵌宝石
            equip.gems[gem_index] = gem_config_id;
            
            // 如果装备已穿戴，更新已穿戴的装备
            auto worn_it = worn_equips_.find(role_id);
            if (worn_it != worn_equips_.end()) {
                for (auto& pair : worn_it->second) {
                    if (pair.second.equip_id == equip_id) {
                        pair.second.gems[gem_index] = gem_config_id;
                        break;
                    }
                }
            }
            
            LOG_INFO("Gem inlaid: role_id=%llu, equip_id=%llu, gem_id=%d, index=%d", role_id, equip_id, gem_config_id, gem_index);
            return true;
        }
    }
    
    LOG_ERROR("Equip not found: role_id=%llu, equip_id=%llu", role_id, equip_id);
    return false;
}

bool EquipModule::RemoveGem(uint64_t role_id, uint64_t equip_id, int32_t gem_index) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = equip_cache_.find(role_id);
    if (it == equip_cache_.end()) {
        return false;
    }
    
    // 查找装备
    for (auto& equip : it->second) {
        if (equip.equip_id == equip_id) {
            // 检查宝石索引
            if (gem_index < 0 || gem_index >= equip.gems.size()) {
                LOG_ERROR("Invalid gem index: role_id=%llu, equip_id=%llu, index=%d, size=%d", 
                          role_id, equip_id, gem_index, equip.gems.size());
                return false;
            }
            
            // 卸下宝石
            equip.gems[gem_index] = 0;
            
            // 如果装备已穿戴，更新已穿戴的装备
            auto worn_it = worn_equips_.find(role_id);
            if (worn_it != worn_equips_.end()) {
                for (auto& pair : worn_it->second) {
                    if (pair.second.equip_id == equip_id) {
                        pair.second.gems[gem_index] = 0;
                        break;
                    }
                }
            }
            
            LOG_INFO("Gem removed: role_id=%llu, equip_id=%llu, index=%d", role_id, equip_id, gem_index);
            return true;
        }
    }
    
    LOG_ERROR("Equip not found: role_id=%llu, equip_id=%llu", role_id, equip_id);
    return false;
}

bool EquipModule::GetGems(uint64_t role_id, uint64_t equip_id, std::vector<int32_t>& gems) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = equip_cache_.find(role_id);
    if (it == equip_cache_.end()) {
        return false;
    }
    
    for (const auto& equip : it->second) {
        if (equip.equip_id == equip_id) {
            gems = equip.gems;
            return true;
        }
    }
    
    return false;
}

bool EquipModule::CalculateEquipAttribute(uint64_t role_id, EquipAttribute& attribute) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 初始化属性
    memset(&attribute, 0, sizeof(EquipAttribute));
    
    auto worn_it = worn_equips_.find(role_id);
    if (worn_it == worn_equips_.end()) {
        return true;
    }
    
    // 计算所有已穿戴装备的属性
    for (const auto& pair : worn_it->second) {
        EquipAttribute single_attr;
        CalculateSingleEquipAttribute(pair.second, single_attr);
        
        attribute.hp += single_attr.hp;
        attribute.mp += single_attr.mp;
        attribute.attack += single_attr.attack;
        attribute.defense += single_attr.defense;
        attribute.speed += single_attr.speed;
        attribute.critical += single_attr.critical;
        attribute.critical_damage += single_attr.critical_damage;
        attribute.dodge += single_attr.dodge;
        attribute.hit += single_attr.hit;
        attribute.armor += single_attr.armor;
        attribute.magic_resist += single_attr.magic_resist;
    }
    
    return true;
}

bool EquipModule::CalculateSingleEquipAttribute(const EquipInfo& equip, EquipAttribute& attribute) {
    // TODO: 根据装备配置计算属性
    // 这里使用简单的计算公式
    
    // 基础属性
    int32_t base_attack = 100 + equip.level * 10;
    int32_t base_defense = 50 + equip.level * 5;
    
    // 强化加成
    float strengthen_bonus = 1.0f + equip.强化等级 * 0.1f;
    
    // 星级加成
    float star_bonus = 1.0f + equip.star_level * 0.2f;
    
    // 品质加成
    float quality_bonus = 1.0f + equip.quality * 0.1f;
    
    // 最终属性
    attribute.hp = 0;
    attribute.mp = 0;
    attribute.attack = base_attack * strengthen_bonus * star_bonus * quality_bonus;
    attribute.defense = base_defense * strengthen_bonus * star_bonus * quality_bonus;
    attribute.speed = 0;
    attribute.critical = 0;
    attribute.critical_damage = 0;
    attribute.dodge = 0;
    attribute.hit = 0;
    attribute.armor = 0;
    attribute.magic_resist = 0;
    
    // TODO: 根据宝石计算额外属性
    
    return true;
}

bool EquipModule::LoadEquipData(uint64_t role_id) {
    // 从数据库加载装备数据
    // TODO: 实现从数据库加载装备数据
    
    // 初始化装备
    InitEquip(role_id);
    
    return true;
}

bool EquipModule::SaveEquipData(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = equip_cache_.find(role_id);
    if (it == equip_cache_.end()) {
        return false;
    }
    
    // 保存装备数据到数据库
    msg_equip::EquipDataSyncReq req;
    req.set_role_id(role_id);
    
    for (const auto& equip : it->second) {
        auto equip_data = req.add_equips();
        equip_data->set_equip_id(equip.equip_id);
        equip_data->set_equip_config_id(equip.equip_config_id);
        equip_data->set_position(equip.position);
        equip_data->set_level(equip.level);
        equip_data->set_star_level(equip.star_level);
        equip_data->set_quality(equip.quality);
        equip_data->set_strengthen_level(equip.strengthen_level);
        
        for (int32_t gem : equip.gems) {
            equip_data->add_gems(gem);
        }
    }
    
    // 保存已穿戴的装备
    auto worn_it = worn_equips_.find(role_id);
    if (worn_it != worn_equips_.end()) {
        for (const auto& pair : worn_it->second) {
            auto worn_equip = req.add_worn_equips();
            worn_equip->set_equip_id(pair.second.equip_id);
            worn_equip->set_position(pair.second.position);
        }
    }
    
    service_->SendMsgToDBServer(static_cast<uint32_t>(MessageID::MSG_EQUIP_DATA_SYNC_REQ), req);
    
    return true;
}

uint64_t EquipModule::GenerateEquipId() {
    // 生成装备ID
    static uint64_t next_id = time(nullptr) * 10000 + rand() % 10000;
    return next_id++;
}

} // namespace game_server
