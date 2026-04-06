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

    // 保存到缓存
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        RoleData& data = role_cache_[role_id];
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
        online_status_[role_id] = false;
    }

    // 计算角色属性
    RoleProperty property;
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        CalculateRoleProperty(role_cache_[role_id], property);
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
        // 逐个字段复制而不是整个对象复制
        info.role_id = it->second.role_id;
        info.account_id = it->second.account_id;
        info.role_name = it->second.role_name;
        info.level = it->second.level;
        info.exp = it->second.exp;
        info.gold = it->second.gold;
        info.diamond = it->second.diamond;
        info.job = it->second.job;
        info.gender = it->second.gender;
        info.create_time = it->second.create_time;
        info.last_login_time = it->second.last_login_time;
        info.last_logout_time = it->second.last_logout_time;
        info.online_time = it->second.online_time;
        info.vip_level = it->second.vip_level;
        info.vip_exp = it->second.vip_exp;
        info.stamina = it->second.stamina;
        info.energy = it->second.energy;
        info.reputation = it->second.reputation;
        info.honor = it->second.honor;
        info.war_credit = it->second.war_credit;
        info.achievement = it->second.achievement;
        info.fight_power = it->second.fight_power;
        info.current_scene = it->second.current_scene;
        info.position_x = it->second.position_x;
        info.position_y = it->second.position_y;
        info.position_z = it->second.position_z;
        info.rotation_y = it->second.rotation_y;
        return true;
    }
    return false;
}

bool RoleModule::UpdateRoleInfo(const RoleData& info) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    RoleData& data = role_cache_[info.role_id];
    // 逐个字段复制而不是整个对象复制
    data.role_id = info.role_id;
    data.account_id = info.account_id;
    data.role_name = info.role_name;
    data.level = info.level;
    data.exp = info.exp;
    data.gold = info.gold;
    data.diamond = info.diamond;
    data.job = info.job;
    data.gender = info.gender;
    data.create_time = info.create_time;
    data.last_login_time = info.last_login_time;
    data.last_logout_time = info.last_logout_time;
    data.online_time = info.online_time;
    data.vip_level = info.vip_level;
    data.vip_exp = info.vip_exp;
    data.stamina = info.stamina;
    data.energy = info.energy;
    data.reputation = info.reputation;
    data.honor = info.honor;
    data.war_credit = info.war_credit;
    data.achievement = info.achievement;
    data.fight_power = info.fight_power;
    data.current_scene = info.current_scene;
    data.position_x = info.position_x;
    data.position_y = info.position_y;
    data.position_z = info.position_z;
    data.rotation_y = info.rotation_y;

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
