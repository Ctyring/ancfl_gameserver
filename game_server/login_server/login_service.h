#ifndef __LOGIN_SERVICE_H__
#define __LOGIN_SERVICE_H__

#include "../common/game_service_base.h"

namespace game_server {

// 登录服务器服务类
class LoginService : public GameServiceBase {
public:
    SERVICE_SINGLETON(LoginService);

    // 初始化服务
    virtual bool InitService() override;

    // 反初始化服务
    virtual void UninitService() override;

    // 注册所有消息处理器
    virtual void RegisterAllHandlers() override;

    // 每秒定时器
    virtual void OnTimer() override;

    // 连接账号服务器
    bool ConnectToAccountServer();

    // 生成登录验证码
    int32_t GenerateLoginCode();

    // 验证登录码
    bool VerifyLoginCode(uint64_t account_id, int32_t login_code);

    // 获取账号对应的逻辑服
    bool GetLogicServerInfo(uint64_t account_id, std::string& ip, int32_t& port);

private:
    // 消息处理器
    bool OnCheckVersionReq(const NetPacket& packet);
    bool OnAccountRegReq(const NetPacket& packet);
    bool OnAccountLoginReq(const NetPacket& packet);
    bool OnServerListReq(const NetPacket& packet);
    bool OnSelectServerReq(const NetPacket& packet);
    bool OnHeartBeatReq(const NetPacket& packet);

    // 服务器间消息
    bool OnLogicRegToLoginReq(const NetPacket& packet);
    bool OnLogicUpdateReq(const NetPacket& packet);

private:
    // 配置
    std::string account_server_ip_;
    int32_t account_server_port_;
    std::string center_server_ip_;
    int32_t center_server_port_;

    // 连接ID
    int32_t account_server_conn_id_;
    int32_t center_server_conn_id_;

    // 登录验证码管理
    std::unordered_map<uint64_t, int32_t> login_codes_;
    std::unordered_map<uint64_t, int64_t> code_expire_time_;
    ancfl::Mutex code_mutex_;

    // 已注册的逻辑服务器
    struct LogicServerInfo {
        int32_t server_id;
        std::string server_name;
        std::string ip;
        int32_t port;
        int32_t conn_id;
        int32_t cur_online;
        int32_t max_online;
        int64_t last_update_time;
    };
    std::unordered_map<int32_t, LogicServerInfo> logic_servers_;
    ancfl::Mutex server_mutex_;
};

} // namespace game_server

#endif // __LOGIN_SERVICE_H__
