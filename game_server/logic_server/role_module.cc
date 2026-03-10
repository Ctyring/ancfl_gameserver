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
    data.体力 = 100;
    data.精力 = 100;
    data.声望 = 0;
    data.荣誉 = 0;
    data.战功 = 0;
    data.成就 = 0;
    data.战斗力 = 1000;
    data.当前场景 = 1001;
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

    // 保存到数据库
    msg_role::RoleDataSyncReq req;
    req.set_role_id(data.role_id);
    req.set_account_id(data.account_id);
    req.set_role_name(data.role_name);
    req.set_level(data.level);
    req.set_exp(data.exp);
    req.set_gold(data.gold);
    req.set_diamond(data.diamond);
    req.set_job(data.job);
    req.set_gender(data.gender);
    req.set_create_time(data.create_time);
    req.set_last_login_time(data.last_login_time);
    req.set_last_logout_time(data.last_logout_time);
    req.set_online_time(data.online_time);
    req.set_vip_level(data.vip_level);
    req.set_vip_exp(data.vip_exp);
    req.set_stamina(data.stamina);
    req.set_energy(data.energy);
    req.set_reputation(data.reputation);
    req.set_honor(data.honor);
    req.set_war_credit(data.war_credit);
    req.set_achievement(data.achievement);
    req.set_fight_power(data.fight_power);
    req.set_current_scene(data.current_scene);
    req.set_position_x(data.position_x);
    req.set_position_y(data.position_y);
    req.set_position_z(data.position_z);
    req.set_rotation_y(data.rotation_y);

    service_->SendMsgToDBServer(
        static_cast<uint32_t>(MessageID::MSG_DB_DATA_SYNC_REQ), req);

    LOG_INFO("Role created: id=%llu, name=%s, account_id=%llu", role_id,
             role_name.c_str(), account_id);
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

    // 发送删除请求到数据库
    msg_role::RoleDeleteReq req;
    req.set_role_id(role_id);
    service_->SendMsgToDBServer(
        static_cast<uint32_t>(MessageID::MSG_ROLE_DELETE_REQ), req);

    LOG_INFO("Role deleted: id=%llu", role_id);
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

    // 保存到数据库
    msg_role::RoleDataSyncReq req;
    req.set_role_id(info.role_id);
    req.set_account_id(info.account_id);
    req.set_role_name(info.role_name);
    req.set_level(info.level);
    req.set_exp(info.exp);
    req.set_gold(info.gold);
    req.set_diamond(info.diamond);
    req.set_job(info.job);
    req.set_gender(info.gender);
    req.set_create_time(info.create_time);
    req.set_last_login_time(info.last_login_time);
    req.set_last_logout_time(info.last_logout_time);
    req.set_online_time(info.online_time);
    req.set_vip_level(info.vip_level);
    req.set_vip_exp(info.vip_exp);
    req.set_stamina(info.stamina);
    req.set_energy(info.energy);
    req.set_reputation(info.reputation);
    req.set_honor(info.honor);
    req.set_war_credit(info.war_credit);
    req.set_achievement(info.achievement);
    req.set_fight_power(info.fight_power);
    req.set_current_scene(info.current_scene);
    req.set_position_x(info.position_x);
    req.set_position_y(info.position_y);
    req.set_position_z(info.position_z);
    req.set_rotation_y(info.rotation_y);

    service_->SendMsgToDBServer(
        static_cast<uint32_t>(MessageID::MSG_DB_DATA_SYNC_REQ), req);

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

        LOG_INFO("Role level up: id=%llu, level=%d", role_id, it->second.level);
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

    LOG_INFO("Role level added: id=%llu, level=%d", role_id, it->second.level);
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
        it->second.当前场景 = position.scene_id;
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
    // 从数据库加载角色数据
    msg_role::RoleDataSyncReq req;
    req.set_role_id(role_id);
    service_->SendMsgToDBServer(
        static_cast<uint32_t>(MessageID::MSG_DB_DATA_GET_REQ), req);

    // TODO: 实现异步加载数据的回调处理
    return true;
}

bool RoleModule::SaveRoleData(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto it = role_cache_.find(role_id);
    if (it == role_cache_.end()) {
        return false;
    }

    // 保存到数据库
    msg_role::RoleDataSyncReq req;
    req.set_role_id(it->second.role_id);
    req.set_account_id(it->second.account_id);
    req.set_role_name(it->second.role_name);
    req.set_level(it->second.level);
    req.set_exp(it->second.exp);
    req.set_gold(it->second.gold);
    req.set_diamond(it->second.diamond);
    req.set_job(it->second.job);
    req.set_gender(it->second.gender);
    req.set_create_time(it->second.create_time);
    req.set_last_login_time(it->second.last_login_time);
    req.set_last_logout_time(it->second.last_logout_time);
    req.set_online_time(it->second.online_time);
    req.set_vip_level(it->second.vip_level);
    req.set_vip_exp(it->second.vip_exp);
    req.set_stamina(it->second.stamina);
    req.set_energy(it->second.energy);
    req.set_reputation(it->second.reputation);
    req.set_honor(it->second.honor);
    req.set_war_credit(it->second.war_credit);
    req.set_achievement(it->second.achievement);
    req.set_fight_power(it->second.fight_power);
    req.set_current_scene(it->second.current_scene);
    req.set_position_x(it->second.position_x);
    req.set_position_y(it->second.position_y);
    req.set_position_z(it->second.position_z);
    req.set_rotation_y(it->second.rotation_y);

    service_->SendMsgToDBServer(
        static_cast<uint32_t>(MessageID::MSG_DB_DATA_SYNC_REQ), req);

    return true;
}

bool RoleModule::GetRoleList(uint64_t account_id,
                             std::vector<RoleData>& roles) {
    // 从数据库获取角色列表
    msg_role::RoleListReq req;
    req.set_account_id(account_id);
    service_->SendMsgToDBServer(
        static_cast<uint32_t>(MessageID::MSG_ROLE_LIST_REQ), req);

    // TODO: 实现异步获取角色列表的回调处理
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
