#ifndef __ACCOUNT_SERVICE_H__
#define __ACCOUNT_SERVICE_H__

#include "../common/game_service_base.h"
#include "ancfl/db/mysql.h"

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
    SERVICE_SINGLETON(AccountService);

    // 初始化服务
    virtual bool InitService() override;

    // 反初始化服务
    virtual void UninitService() override;

    // 注册所有消息处理器
    virtual void RegisterAllHandlers() override;

    // 每秒定时器
    virtual void OnTimer() override;

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

private:
    // 消息处理器
    bool OnAccountRegReq(const NetPacket& packet);
    bool OnAccountLoginReq(const NetPacket& packet);
    bool OnSealAccountReq(const NetPacket& packet);
    bool OnHeartBeatReq(const NetPacket& packet);

    // 数据库操作
    bool InitDatabase();
    void CloseDatabase();

    // MD5加密
    std::string MD5Encrypt(const std::string& input);

private:
    // 数据库连接
    ancfl::MySQL::ptr mysql_;
    
    // 账号缓存
    std::unordered_map<uint64_t, AccountInfo> account_cache_;
    std::unordered_map<std::string, uint64_t> name_to_id_;
    ancfl::Mutex cache_mutex_;
    
    // 数据库配置
    std::string db_host_;
    int32_t db_port_;
    std::string db_user_;
    std::string db_password_;
    std::string db_name_;
};

} // namespace game_server

#endif // __ACCOUNT_SERVICE_H__
