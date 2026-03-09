#ifndef __PROXY_SERVICE_H__
#define __PROXY_SERVICE_H__

#include "common/game_service_base.h"

namespace game_server {

// 客户端会话信息
struct ClientSession {
    uint32_t conn_id;
    uint64_t account_id;
    uint64_t role_id;
    std::string session_id;
    int32_t logic_server_id;
    time_t last_active_time;
    time_t login_time;
    std::string ip;
    int32_t port;
};

// 服务器连接信息
struct ServerConnection {
    uint32_t conn_id;
    std::string server_name;
    std::string ip;
    int32_t port;
    time_t last_active_time;
    int32_t player_count;
};

// 网关服务类
class ProxyService : public GameServiceBase {
public:
    SERVICE_SINGLETON(ProxyService);
    
    virtual bool InitService() override;
    virtual void UninitService() override;
    virtual void RegisterAllHandlers() override;
    virtual void OnTimer() override;
    
    // 连接逻辑服务器
    bool ConnectToLogicServers();
    
    // 客户端连接管理
    void OnClientConnect(uint32_t conn_id, const std::string& ip, int32_t port);
    void OnClientDisconnect(uint32_t conn_id);
    
    // 消息转发
    bool ForwardToLogicServer(uint32_t client_conn_id, uint32_t msg_id, const std::string& data);
    bool ForwardToClient(uint32_t client_conn_id, uint32_t msg_id, const std::string& data);
    
    // 会话管理
    bool CreateSession(uint32_t conn_id, uint64_t account_id, uint64_t role_id, const std::string& session_id);
    bool RemoveSession(uint32_t conn_id);
    ClientSession* GetSession(uint32_t conn_id);
    ClientSession* GetSessionByAccount(uint64_t account_id);
    
    // 负载均衡
    uint32_t SelectLogicServer();
    
    // 服务器管理
    void AddLogicServer(const ServerConnection& server);
    void RemoveLogicServer(uint32_t conn_id);
    ServerConnection* GetLogicServer(uint32_t conn_id);
    
private:
    // 消息处理器
    bool OnCheckVersionReq(const NetPacket& packet);
    bool OnAccountRegReq(const NetPacket& packet);
    bool OnAccountLoginReq(const NetPacket& packet);
    bool OnServerListReq(const NetPacket& packet);
    bool OnSelectServerReq(const NetPacket& packet);
    bool OnRoleCreateReq(const NetPacket& packet);
    bool OnRoleLoginReq(const NetPacket& packet);
    bool OnRoleLogoutReq(const NetPacket& packet);
    bool OnHeartBeatReq(const NetPacket& packet);
    bool OnLogicRegToProxyReq(const NetPacket& packet);
    bool OnLogicUpdateReq(const NetPacket& packet);
    bool OnLogicDataAck(const NetPacket& packet);
    
    // 客户端会话
    std::unordered_map<uint32_t, ClientSession> client_sessions_;
    std::unordered_map<uint64_t, uint32_t> account_to_conn_;
    std::mutex session_mutex_;
    
    // 逻辑服务器连接
    std::vector<ServerConnection> logic_servers_;
    std::mutex server_mutex_;
    
    // 会话超时时间（秒）
    int32_t session_timeout_;
    
    // 心跳超时时间（秒）
    int32_t heartbeat_timeout_;
    
    // 清理定时器
    int32_t cleanup_timer_;
};

} // namespace game_server

#endif // __PROXY_SERVICE_H__
