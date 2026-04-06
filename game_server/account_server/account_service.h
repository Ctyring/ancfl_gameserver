#ifndef __ACCOUNT_SERVICE_H__
#define __ACCOUNT_SERVICE_H__

#include "../common/game_service_base.h"
#include "../common/message_dispatcher.h"
#include "../proto/msg_db.pb.h"

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

// 账号服务器服务类
class AccountService : public GameServiceBase {
public:
    AccountService();
    ~AccountService();

    // 初始化服务
    virtual bool InitService() override;

    // 反初始化服务
    virtual void UninitService() override;

    // 注册所有消息处理器
    virtual void RegisterAllHandlers() override;

    // 每秒定时器
    virtual void OnTimer() override;
    
    // 5秒定时器
    virtual void OnTimer5s() override;

    // 账号操作
    bool CreateAccount(const std::string& account_name, const std::string& password,
                       int32_t channel, uint64_t& account_id);
    
    bool VerifyAccount(const std::string& account_name, const std::string& password,
                       AccountInfo& info);
    
    bool GetAccountInfo(uint64_t account_id, AccountInfo& info);
    bool GetAccountInfo(const std::string& account_name, AccountInfo& info);
    
    bool SealAccount(uint64_t account_id, int32_t seal_time);
    bool UnsealAccount(uint64_t account_id);
    
    bool IsAccountSealed(uint64_t account_id);

    // 记录登录日志
    bool RecordLoginLog(uint64_t account_id, int32_t channel, const std::string& version,
                        const std::string& uuid, const std::string& idfa,
                        const std::string& imodel, const std::string& imei, int32_t ip);

    // 处理登录请求
    void HandleLoginRequest(ancfl::Socket::ptr client, const std::string& data);
    
    // 处理注册请求
    void HandleRegisterRequest(ancfl::Socket::ptr client, const std::string& data);

protected:
    // 处理客户端连接
    virtual void handleClient(ancfl::Socket::ptr client) override;

private:
    // 消息处理器
    bool OnAccountRegReq(const NetPacket& packet);
    bool OnAccountLoginReq(const NetPacket& packet);
    bool OnSealAccountReq(const NetPacket& packet);
    bool OnHeartBeatReq(const NetPacket& packet);

    // 连接中心服务器
    bool ConnectToCenterServer();
    
    // 注册到中心服务器
    bool RegisterToCenterServer();
    
    // 发送心跳到中心服务器
    void SendHeartbeatToCenterServer();
    
    // 连接数据库服务器
    bool ConnectToDBServer();
    
    // 向数据库服务器发送请求并获取响应
    bool SendDBRequest(const DBRequest& req, DBResponse& rsp);

    // MD5加密
    std::string MD5Encrypt(const std::string& input);

private:
    // 账号缓存
    std::unordered_map<uint64_t, AccountInfo> account_cache_;
    std::unordered_map<std::string, uint64_t> name_to_id_;
    ancfl::Mutex cache_mutex_;
    
    // 中心服务器连接
    uint32_t center_server_id_;
    std::string center_server_ip_;
    int32_t center_server_port_;
    ancfl::Socket::ptr center_server_conn_;
    
    // 数据库服务器连接
    std::string db_server_ip_;
    int32_t db_server_port_;
    ancfl::Socket::ptr db_server_conn_;
};

} // namespace game_server

#endif // __ACCOUNT_SERVICE_H__
