#ifndef __DB_SERVICE_H__
#define __DB_SERVICE_H__

#include "ancfl/ancfl.h"
#include "ancfl/db/mysql.h"
#include "common/game_service_base.h"
#include "common/shared_memory.h"
#include "common/message_dispatcher.h"
#include "proto/msg_db.pb.h"

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



// 数据服务类
class DBService : public GameServiceBase {
public:
    DBService();
    ~DBService();

    virtual bool InitService() override;
    virtual void UninitService() override;
    virtual void RegisterAllHandlers() override;
    virtual void handleClient(ancfl::Socket::ptr client) override;
    virtual void OnTimer5s() override;

    // 连接数据库
    bool ConnectToDatabase();

    // 数据操作
    bool CreateRole(const game_server::RoleInfo& role_info);
    bool UpdateRole(const game_server::RoleInfo& role_info);
    bool DeleteRole(uint64_t role_id);
    bool GetRoleList(uint64_t account_id, std::vector<game_server::RoleInfo>& roles);
    bool GetRoleData(uint64_t role_id, game_server::RoleInfo& data);

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

    // 获取当前连接数
    size_t GetConnectionCount() const;

    // 根据连接ID获取客户端socket
    ancfl::Socket::ptr GetClientByConnId(uint32_t conn_id);

    // 设置最大连接数
    void SetMaxConnections(size_t max_connections);

    // 获取最大连接数
    size_t GetMaxConnections() const { return max_connections_; }

protected:
    std::unordered_map<uint32_t, ancfl::Socket::ptr> connections_;  ///< 连接映射表：conn_id -> socket
    std::unordered_map<uint32_t, time_t> last_heart_time_;          ///< 心跳时间映射表：conn_id -> 最后心跳时间
    ancfl::Mutex conn_mutex_;                                        ///< 连接映射表的互斥锁
    int32_t next_conn_id_;                                           ///< 下一个连接ID
    size_t max_connections_;                                         ///< 最大连接数

private:
    // 消息处理器
    void HandleCreateAccount(const DBRequest& req, DBResponse& rsp);
    void HandleVerifyAccount(const DBRequest& req, DBResponse& rsp);
    void HandleGetAccountInfo(const DBRequest& req, DBResponse& rsp);
    void HandleSealAccount(const DBRequest& req, DBResponse& rsp);
    void HandleUnsealAccount(const DBRequest& req, DBResponse& rsp);
    void HandleRecordLoginLog(const DBRequest& req, DBResponse& rsp);
    
    // 缓存管理
    void UpdateAccountCache(const AccountInfo& info);
    bool GetAccountFromCache(uint64_t account_id, AccountInfo& info);
    bool GetAccountFromCache(const std::string& account_name, AccountInfo& info);
    void HandleCreateRole(const DBRequest& req, DBResponse& rsp);
    void HandleUpdateRole(const DBRequest& req, DBResponse& rsp);
    void HandleDeleteRole(const DBRequest& req, DBResponse& rsp);
    void HandleGetRoleList(const DBRequest& req, DBResponse& rsp);
    void HandleGetRoleData(const DBRequest& req, DBResponse& rsp);

private:
    // 处理接收客户端消息
    void HandleRecv(ancfl::Socket::ptr client, int32_t conn_id);

    // 中心服务器连接
    int32_t center_server_id_;
    std::string center_server_ip_;
    int32_t center_server_port_;
    ancfl::Socket::ptr center_server_conn_;
    
    // 连接中心服务器
    bool ConnectToCenterServer();
    // 向中心服务器注册
    bool RegisterToCenterServer();
    // 发送心跳到中心服务器
    void SendHeartbeatToCenterServer();

    // 数据库连接池
    std::vector<std::shared_ptr<ancfl::MySQL>> db_connections_;
    std::mutex db_mutex_;

    // 数据库配置
    std::string db_host_;
    int db_port_;
    std::string db_user_;
    std::string db_password_;
    std::string db_name_;
    
    // 账号缓存
    std::unordered_map<uint64_t, AccountInfo> account_cache_;         ///< 账号缓存，键为账号ID
    std::unordered_map<std::string, uint64_t> name_to_id_;            ///< 账号名称到ID的映射
    ancfl::Mutex cache_mutex_;                                        ///< 缓存的互斥锁
};

} // namespace game_server

#endif // __DB_SERVICE_H__
