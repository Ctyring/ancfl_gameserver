#include "role_module.h"
#include "proto/msg_role.pb.h"

namespace game_server {

RoleModule::RoleModule(LogicService* service) : service_(service) {}

RoleModule::~RoleModule() {}

bool RoleModule::CreateRole(uint64_t account_id,
                            const std::string& role_name,
                            int32_t job,
                            int32_t gender,
                            uint64_t& role_id) {
    // 检查角色名称是否重复
    // TODO: 实现角色名称检查

    // 生成角色ID
    role_id = time(nullptr) * 10000 + rand() % 10000;

    // 创建角色数据
    RoleData data;
    data.role_id = role_id;
    data.account_id = account_id;
    data.role_name = role_name;
    data.level = 1;
    data.exp = 0;
    data.gold = 10000;
    data.diamond = 1000;
    data.job = job;
    data.gender = gender;
    data.create_time = time(nullptr);
    data.last_login_time = data.create_time;
    data.last_logout_time = 0;
    data.online_time = 0;
    data.vip_level = 0;
    data.vip_exp = 0;
    data.stamina = 100;
    data.energy = 100;
    data.reputation = 0;
    data.honor = 0;
    data.war_credit = 0;
    data.achievement = 0;
    data.fight_power = 1000;
    data.current_scene = 1001;
    data.position_x = 0.0f;
    data.position_y = 0.0f;
    data.position_z = 0.0f;
    data.rotation_y = 0.0f;

    // 保存到缓存
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        role_cache_[role_id] = data;
        online_status_[role_id] = false;
    }

    // 计算角色属性
    RoleProperty property;
    CalculateRoleProperty(data, property);
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        property_cache_[role_id] = property;
    }

    // 暂时返回 true，实际实现需要保存数据到数据库
    return true;
}

bool RoleModule::DeleteRole(uint64_t role_id) {
    // 从缓存中删除
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        role_cache_.erase(role_id);
        property_cache_.erase(role_id);
        position_cache_.erase(role_id);
        online_status_.erase(role_id);
    }

    // 暂时返回 true，实际实现需要从数据库删除数据
    return true;
}

bool RoleModule::GetRoleInfo(uint64_t role_id, RoleData& info) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = role_cache_.find(role_id);
    if (it != role_cache_.end()) {
        info = it->second;
        return true;
    }
    return false;
}

bool RoleModule::UpdateRoleInfo(const RoleData& info) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    role_cache_[info.role_id] = info;

    // 暂时返回 true，实际实现需要保存数据到数据库
    return true;
}

bool RoleModule::GetRoleProperty(uint64_t role_id, RoleProperty& property) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = property_cache_.find(role_id);
    if (it != property_cache_.end()) {
        property = it->second;
        return true;
    }
    return false;
}

bool RoleModule::UpdateRoleProperty(uint64_t role_id,
                                    const RoleProperty& property) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    property_cache_[role_id] = property;
    return true;
}

bool RoleModule::AddRoleExp(uint64_t role_id, int32_t exp) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = role_cache_.find(role_id);
    if (it == role_cache_.end()) {
        return false;
    }

    it->second.exp += exp;

    // 检查升级
    int32_t max_exp = it->second.level * 1000;
    while (it->second.exp >= max_exp) {
        it->second.exp -= max_exp;
        it->second.level++;
        max_exp = it->second.level * 1000;

        // 重新计算属性
        RoleProperty property;
        CalculateRoleProperty(it->second, property);
        property_cache_[role_id] = property;

        // 暂时注释掉日志输出，实际实现需要添加日志
    }

    return true;
}

bool RoleModule::AddRoleLevel(uint64_t role_id, int32_t level) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = role_cache_.find(role_id);
    if (it == role_cache_.end()) {
        return false;
    }

    it->second.level += level;

    // 重新计算属性
    RoleProperty property;
    CalculateRoleProperty(it->second, property);
    property_cache_[role_id] = property;

    // 暂时注释掉日志输出，实际实现需要添加日志
    return true;
}

bool RoleModule::GetRolePosition(uint64_t role_id, RolePosition& position) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = position_cache_.find(role_id);
    if (it != position_cache_.end()) {
        position = it->second;
        return true;
    }
    return false;
}

bool RoleModule::UpdateRolePosition(uint64_t role_id,
                                    const RolePosition& position) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    position_cache_[role_id] = position;

    // 更新角色数据
    auto it = role_cache_.find(role_id);
    if (it != role_cache_.end()) {
        it->second.current_scene = position.scene_id;
        it->second.position_x = position.x;
        it->second.position_y = position.y;
        it->second.position_z = position.z;
        it->second.rotation_y = position.rotation_y;
    }

    return true;
}

bool RoleModule::SetRoleOnline(uint64_t role_id, bool online) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    online_status_[role_id] = online;

    if (online) {
        auto it = role_cache_.find(role_id);
        if (it != role_cache_.end()) {
            it->second.last_login_time = time(nullptr);
        }
    } else {
        auto it = role_cache_.find(role_id);
        if (it != role_cache_.end()) {
            it->second.last_logout_time = time(nullptr);
        }
    }

    return true;
}

bool RoleModule::IsRoleOnline(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = online_status_.find(role_id);
    if (it != online_status_.end()) {
        return it->second;
    }
    return false;
}

bool RoleModule::LoadRoleData(uint64_t role_id) {
    // 暂时返回 true，实际实现需要从数据库加载数据
    return true;
}

bool RoleModule::SaveRoleData(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = role_cache_.find(role_id);
    if (it == role_cache_.end()) {
        return false;
    }

    // 暂时返回 true，实际实现需要保存数据到数据库
    return true;
}

bool RoleModule::GetRoleList(uint64_t account_id,
                             std::vector<RoleData>& roles) {
    // 暂时返回 true，实际实现需要从数据库获取角色列表
    return true;
}

void RoleModule::CalculateRoleProperty(RoleData& data, RoleProperty& property) {
    // 基础属性
    property.hp = 1000 + data.level * 100;
    property.max_hp = property.hp;
    property.mp = 500 + data.level * 50;
    property.max_mp = property.mp;
    property.attack = 100 + data.level * 10;
    property.defense = 50 + data.level * 5;
    property.speed = 100;
    property.critical = 10;
    property.critical_damage = 150;
    property.dodge = 10;
    property.hit = 90;
    property.armor = 20 + data.level * 2;
    property.magic_resist = 20 + data.level * 2;

    // TODO: 根据装备、技能等计算最终属性
}

}  // namespace game_server
