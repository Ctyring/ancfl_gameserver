#ifndef __LOGIC_SERVICE_H__
#define __LOGIC_SERVICE_H__

#include "activity_module.h"
#include "bag_module.h"
#include "buff_module.h"
#include "common/game_service_base.h"
#include "common/shared_memory.h"
#include "equip_module.h"
#include "friend_module.h"
#include "guild_module.h"
#include "mail_module.h"
#include "role_data.h"
#include "role_module.h"
#include "scene_module.h"
#include "shop_module.h"
#include "skill_module.h"
#include "task_module.h"

namespace game_server {

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
                    int32_t job,
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

    // 获取各模块指针
    RoleModule* GetRoleModule() { return &role_module_; }
    BagModule* GetBagModule() { return &bag_module_; }
    EquipModule* GetEquipModule() { return &equip_module_; }
    TaskModule* GetTaskModule() { return &task_module_; }
    MailModule* GetMailModule() { return &mail_module_; }
    FriendModule* GetFriendModule() { return &friend_module_; }
    ShopModule* GetShopModule() { return &shop_module_; }
    GuildModule* GetGuildModule() { return &guild_module_; }
    BuffModule* GetBuffModule() { return &buff_module_; }
    SkillModule* GetSkillModule() { return &skill_module_; }
    SceneModule* GetSceneModule() { return &scene_module_; }
    ActivityModule* GetActivityModule() { return &activity_module_; }

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

    // 游戏功能模块
    RoleModule role_module_;
    BagModule bag_module_;
    EquipModule equip_module_;
    TaskModule task_module_;
    MailModule mail_module_;
    FriendModule friend_module_;
    ShopModule shop_module_;
    GuildModule guild_module_;
    BuffModule buff_module_;
    SkillModule skill_module_;
    SceneModule scene_module_;
    ActivityModule activity_module_;
};

}  // namespace game_server

#endif  // __LOGIC_SERVICE_H__
