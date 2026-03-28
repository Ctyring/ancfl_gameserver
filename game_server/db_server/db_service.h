#ifndef __DB_SERVICE_H__
#define __DB_SERVICE_H__

#include "ancfl/ancfl.h"
#include "ancfl/db/mysql.h"
#include "common/game_service_base.h"
#include "common/shared_memory.h"

namespace game_server {

// 账号信息结构
struct AccountInfo {
    uint64_t account_id;
    std::string account_name;
    std::string password;
    int32_t channel;
    int64_t create_time;
    int64_t last_login_time;
    int32_t last_login_ip;
    bool is_sealed;
    int64_t seal_end_time;
    bool review;
};

// 角色信息结构
struct RoleInfo {
    uint64_t role_id;
    uint64_t account_id;
    int32_t server_id;
    std::string role_name;
    int32_t career;
    int32_t level;
    int64_t exp;
    int32_t head_id;
    int32_t portrait_frame;
    int64_t create_time;
    int64_t last_login_time;
    int8_t is_deleted;
    int64_t delete_time;
};

// 数据服务类
class DBService : public GameServiceBase {
public:
    DBService();
    ~DBService();

    virtual bool InitService() override;
    virtual void UninitService() override;
    virtual void RegisterAllHandlers() override;
    virtual void OnTimer() override;

    // 连接数据库
    bool ConnectToDatabase();

    // 数据操作
    bool CreateRole(const RoleInfo& role_info);
    bool UpdateRole(const RoleInfo& role_info);
    bool DeleteRole(uint64_t role_id);
    bool GetRoleList(uint64_t account_id, std::vector<RoleInfo>& roles);
    bool GetRoleData(uint64_t role_id, RoleInfo& data);

    // 账号操作
    bool CreateAccount(const std::string& account_name,
                       const std::string& password,
                       int32_t channel,
                       uint64_t& account_id);
    bool VerifyAccount(const std::string& account_name,
                       const std::string& password,
                       uint64_t& account_id);
    bool GetAccountInfo(uint64_t account_id, AccountInfo& info);
    bool SealAccount(uint64_t account_id, int64_t seal_end_time);
    bool UnsealAccount(uint64_t account_id);
    bool IsAccountSealed(uint64_t account_id);

    // 日志操作
    bool RecordLoginLog(uint64_t account_id,
                        int32_t channel,
                        const std::string& version,
                        const std::string& uuid,
                        const std::string& idfa,
                        const std::string& imodel,
                        const std::string& imei,
                        int32_t ip);

    // 数据库连接池
    std::shared_ptr<ancfl::MySQL> GetDBConnection();
    void ReleaseDBConnection(std::shared_ptr<ancfl::MySQL> conn);

    // 设置主IOManager（用于网络IO）
    void SetIOManager(ancfl::IOManager* io_manager) {
        io_manager_ = io_manager;
    }

    // 设置工作线程池（用于后台任务）
    void SetWorkerPool(ancfl::IOManager* worker_pool) {
        worker_pool_ = worker_pool;
    }

private:
    // 消息处理器
    bool OnDBDataSyncReq(const NetPacket& packet);
    bool OnRoleListReq(const NetPacket& packet);
    bool OnRoleDeleteReq(const NetPacket& packet);
    bool OnAccountCreateReq(const NetPacket& packet);
    bool OnAccountVerifyReq(const NetPacket& packet);

private:
    // 数据库连接池
    std::vector<std::shared_ptr<ancfl::MySQL>> db_connections_;
    std::mutex db_mutex_;

    // IO管理器
    ancfl::IOManager* io_manager_;
    ancfl::IOManager* worker_pool_;

    // 数据库配置
    std::string db_host_;
    int db_port_;
    std::string db_user_;
    std::string db_password_;
    std::string db_name_;
};

} // namespace game_server

#endif // __DB_SERVICE_H__
