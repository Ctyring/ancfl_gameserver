#ifndef __LOGIN_SERVICE_H__
#define __LOGIN_SERVICE_H__

#include "../common/game_service_base.h"
#include "../common/message_dispatcher.h"

namespace game_server {

// 登录服务器服务类
class LoginService : public GameServiceBase {
public:
    LoginService();
    ~LoginService();

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

    // 连接账号服务器
    bool ConnectToAccountServer();
    
    // 连接中心服务器
    bool ConnectToCenterServer();
    
    // 注册到中心服务器
    bool RegisterToCenterServer();
    
    // 发送心跳到中心服务器
    void SendHeartbeatToCenterServer();

    // 处理登录请求
    void HandleLoginRequest(ancfl::Socket::ptr client, const std::string& data);
    
    // 处理注册请求
    void HandleRegisterRequest(ancfl::Socket::ptr client, const std::string& data);
    
    // 处理服务器列表请求
    void HandleServerListRequest(ancfl::Socket::ptr client, const std::string& data);
    
    // 处理选择服务器请求
    void HandleSelectServerRequest(ancfl::Socket::ptr client, const std::string& data);

protected:
    // 处理客户端连接
    virtual void handleClient(ancfl::Socket::ptr client) override;

private:
    // 生成登录验证码
    int32_t GenerateLoginCode();

    // 验证登录码
    bool VerifyLoginCode(uint64_t account_id, int32_t login_code);

    // 获取账号对应的逻辑服
    bool GetLogicServerInfo(uint64_t account_id, std::string& ip, int32_t& port);

private:
    // 消息处理器（暂时不实现）
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
    
    // 中心服务器连接
    ancfl::Socket::ptr center_server_conn_;
    
    // 账号服务器连接
    ancfl::Socket::ptr account_server_conn_;
    
    // 客户端连接映射，用于保存账号服务器连接到客户端连接的映射
    std::unordered_map<ancfl::Socket*, ancfl::Socket::ptr> client_map_;

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
