#ifndef __DB_SERVICE_H__
#define __DB_SERVICE_H__

#include "common/game_service_base.h"
#include "common/shared_memory.h"
#include "ancfl/db/mysql.h"

namespace game_server {

// 数据服务类
class DBService : public GameServiceBase {
public:
    SERVICE_SINGLETON(DBService);
    
    virtual bool InitService() override;
    virtual void UninitService() override;
    virtual void RegisterAllHandlers() override;
    virtual void OnTimer() override;
    
    // 连接数据库
    bool ConnectToDatabase();
    
    // 数据操作
    bool CreateRole(const msg_role::RoleDataSyncReq& req);
    bool UpdateRole(const msg_role::RoleDataSyncReq& req);
    bool DeleteRole(uint64_t role_id);
    bool GetRoleList(uint64_t account_id, std::vector<msg_role::RoleInfo>& roles);
    bool GetRoleData(uint64_t role_id, msg_role::RoleDataSyncReq& data);
    
    // 账号操作
    bool CreateAccount(const std::string& account_name, const std::string& password, int32_t channel, uint64_t& account_id);
    bool VerifyAccount(const std::string& account_name, const std::string& password, msg_account::AccountInfo& info);
    bool GetAccountInfo(uint64_t account_id, msg_account::AccountInfo& info);
    bool SealAccount(uint64_t account_id, int32_t seal_time);
    bool UnsealAccount(uint64_t account_id);
    bool IsAccountSealed(uint64_t account_id);
    
    // 日志操作
    bool RecordLoginLog(uint64_t account_id, int32_t channel, const std::string& version, const std::string& uuid, const std::string& idfa, const std::string& imodel, const std::string& imei, int32_t ip);
    
    // 数据库连接池
    std::shared_ptr<ancfl::MySQL> GetDBConnection();
    void ReleaseDBConnection(std::shared_ptr<ancfl::MySQL> conn);
    
private:
    // 消息处理器
    bool OnDBDataSyncReq(const NetPacket& packet);
    bool OnRoleListReq(const NetPacket& packet);
    bool OnRoleDeleteReq(const NetPacket& packet);
    bool OnAccountCreateReq(const NetPacket& packet);
    bool OnAccountVerifyReq(const NetPacket& packet);
    bool OnAccountGetInfoReq(const NetPacket& packet);
    bool OnAccountSealReq(const NetPacket& packet);
    bool OnAccountUnsealReq(const NetPacket& packet);
    bool OnAccountIsSealedReq(const NetPacket& packet);
    bool OnLoginLogReq(const NetPacket& packet);
    bool OnHeartBeatReq(const NetPacket& packet);
    bool OnLogicRegToDBReq(const NetPacket& packet);
    
    // 数据库连接池
    std::vector<std::shared_ptr<ancfl::MySQL>> db_pool_;
    std::mutex db_pool_mutex_;
    
    // 逻辑服务器连接
    std::unordered_map<uint32_t, std::string> logic_servers_;
    
    // 配置信息
    std::string db_host_;
    int32_t db_port_;
    std::string db_user_;
    std::string db_password_;
    std::string db_name_;
    int32_t db_pool_size_;
    
    // 同步定时器
    int32_t sync_timer_;
};

} // namespace game_server

#endif // __DB_SERVICE_H__
