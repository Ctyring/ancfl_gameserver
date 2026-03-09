#ifndef __LOGIC_SERVICE_H__
#define __LOGIC_SERVICE_H__

#include "common/game_service_base.h"
#include "common/shared_memory.h"

namespace game_server {

// 角色数据结构
struct RoleData : public ShareObject {
    uint64_t role_id;
    uint64_t account_id;
    std::string role_name;
    int32_t level;
    int32_t exp;
    int32_t gold;
    int32_t diamond;
    int32_t job;
    int32_t gender;
    int32_t create_time;
    int32_t last_login_time;
    int32_t last_logout_time;
    int32_t online_time;
    int32_t vip_level;
    int32_t vip_exp;
    int32_t stamina;
    int32_t energy;
    int32_t reputation;
    int32_t honor;
    int32_t war_credit;
    int32_t achievement;
    int32_t fight_power;
    int32_t current_scene;
    float position_x;
    float position_y;
    float position_z;
    float rotation_y;
};

// 逻辑服务类
class LogicService : public GameServiceBase {
   public:
    SERVICE_SINGLETON(LogicService);

    virtual bool InitService() override;
    virtual void UninitService() override;
    virtual void RegisterAllHandlers() override;
    virtual void OnTimer() override;

    // 连接数据服务器
    bool ConnectToDBServer();

    // 角色管理
    bool CreateRole(uint64_t account_id,
                    const std::string& role_name,
                    int32_t职业,
                    int32_t gender);
    bool LoadRoleData(uint64_t role_id, RoleData& data);
    bool SaveRoleData(const RoleData& data);
    bool DeleteRole(uint64_t role_id);

    // 获取角色列表
    bool GetRoleList(uint64_t account_id, std::vector<RoleData>& roles);

    // 共享内存管理
    bool InitSharedMemory();
    RoleData* AllocateRoleData();
    void FreeRoleData(RoleData* data);

   private:
    // 消息处理器
    bool OnRoleCreateReq(const NetPacket& packet);
    bool OnRoleLoginReq(const NetPacket& packet);
    bool OnRoleLogoutReq(const NetPacket& packet);
    bool OnRoleListReq(const NetPacket& packet);
    bool OnRoleDeleteReq(const NetPacket& packet);
    bool OnHeartBeatReq(const NetPacket& packet);
    bool OnDBRegToLogicReq(const NetPacket& packet);
    bool OnDBDataSyncAck(const NetPacket& packet);

    // 共享内存
    std::unique_ptr<SharedMemory<RoleData>> role_memory_;

    // 数据库服务器连接
    uint32_t db_server_id_;
    std::string db_server_ip_;
    int32_t db_server_port_;

    // 角色缓存
    std::unordered_map<uint64_t, RoleData*> role_cache_;

    // 同步定时器
    int32_t sync_timer_;
};

}  // namespace game_server

#endif  // __LOGIC_SERVICE_H__
