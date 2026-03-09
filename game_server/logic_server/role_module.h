#ifndef __ROLE_MODULE_H__
#define __ROLE_MODULE_H__

#include "logic_service.h"
#include <unordered_map>

namespace game_server {

// 角色属性
struct RoleProperty {
    int32_t hp;
    int32_t max_hp;
    int32_t mp;
    int32_t max_mp;
    int32_t attack;
    int32_t defense;
    int32_t speed;
    int32_t critical;
    int32_t critical_damage;
    int32_t dodge;
    int32_t hit;
    int32_t armor;
    int32_t magic_resist;
};

// 角色位置信息
struct RolePosition {
    int32_t scene_id;
    float x;
    float y;
    float z;
    float rotation_y;
};

// 角色模块类
class RoleModule {
public:
    RoleModule(LogicService* service);
    ~RoleModule();
    
    // 角色管理
    bool CreateRole(uint64_t account_id, const std::string& role_name, int32_t职业, int32_t gender, uint64_t& role_id);
    bool DeleteRole(uint64_t role_id);
    bool GetRoleInfo(uint64_t role_id, RoleData& info);
    bool UpdateRoleInfo(const RoleData& info);
    
    // 角色属性管理
    bool GetRoleProperty(uint64_t role_id, RoleProperty& property);
    bool UpdateRoleProperty(uint64_t role_id, const RoleProperty& property);
    bool AddRoleExp(uint64_t role_id, int32_t exp);
    bool AddRoleLevel(uint64_t role_id, int32_t level);
    
    // 角色位置管理
    bool GetRolePosition(uint64_t role_id, RolePosition& position);
    bool UpdateRolePosition(uint64_t role_id, const RolePosition& position);
    
    // 角色状态管理
    bool SetRoleOnline(uint64_t role_id, bool online);
    bool IsRoleOnline(uint64_t role_id);
    
    // 角色数据加载
    bool LoadRoleData(uint64_t role_id);
    bool SaveRoleData(uint64_t role_id);
    
    // 角色列表
    bool GetRoleList(uint64_t account_id, std::vector<RoleData>& roles);
    
private:
    // 计算角色属性
    void CalculateRoleProperty(RoleData& data, RoleProperty& property);
    
    // 角色数据缓存
    std::unordered_map<uint64_t, RoleData> role_cache_;
    std::unordered_map<uint64_t, RoleProperty> property_cache_;
    std::unordered_map<uint64_t, RolePosition> position_cache_;
    std::unordered_map<uint64_t, bool> online_status_;
    
    // 逻辑服务指针
    LogicService* service_;
    
    // 互斥锁
    std::mutex cache_mutex_;
};

} // namespace game_server

#endif // __ROLE_MODULE_H__
