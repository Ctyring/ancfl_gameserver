#ifndef __CENTER_SERVER_H__
#define __CENTER_SERVER_H__

#include "ancfl/ancfl.h"
#include "common/game_service_base.h"
#include "common/message_dispatcher.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <memory>
#include <functional>

// 前向声明
namespace google {
namespace protobuf {
class Message;
}
}

namespace game_server {

// 服务器类型
enum class ServerType {
    LOGIN = 1,
    ACCOUNT = 2,
    LOGIC = 3,
    DB = 4,
    PROXY = 5,
    GATEWAY = 6,
    CENTER = 7
};

// 服务器状态
enum class ServerStatus {
    INIT = 0,
    RUNNING = 1,
    STOPPING = 2,
    STOPPED = 3
};

// 服务器信息
struct ServerInfo {
    int32_t server_id;
    ServerType type;
    std::string ip;
    int32_t port;
    ServerStatus status;
    int32_t online_count;
    int32_t max_online;
    time_t start_time;
    time_t last_heartbeat;
    std::string version;
    std::string extra_info;
};

// 玩家跨服信息
struct CrossServerPlayer {
    uint64_t role_id;
    std::string role_name;
    int32_t src_server_id;
    int32_t dest_server_id;
    int32_t cross_type;
    time_t enter_time;
    std::string token;
    bool is_online;
};

// 跨服类型
enum class CrossType {
    CROSS_BATTLE = 1,   // 跨服战斗
    CROSS_ARENA = 2,    // 跨服竞技场
    CROSS_BOSS = 3,     // 跨服Boss
    CROSS_GUILD = 4     // 跨服公会战
};

// 中心服务器类
class CenterServer : public GameServiceBase {
public:
    CenterServer();
    ~CenterServer();
    
    // 初始化和启动
    virtual bool InitService() override;
    virtual void UninitService() override;
    
    // 注册所有消息处理器
    virtual void RegisterAllHandlers() override;
    
    // 处理客户端连接（重写父类方法）
    virtual void handleClient(ancfl::Socket::ptr client) override;
    
    // 处理接收消息
    void HandleRecv(ancfl::Socket::ptr client, int32_t conn_id);
    
    // 服务器管理
    bool RegisterServer(const ServerInfo& info);
    bool UnregisterServer(int32_t server_id);
    bool UpdateServerHeartbeat(int32_t server_id);
    bool GetServerInfo(int32_t server_id, ServerInfo& info);
    bool GetServerList(ServerType type, std::vector<ServerInfo>& servers);
    bool GetAllServers(std::vector<ServerInfo>& servers);
    
    // 服务器选择
    int32_t SelectBestServer(ServerType type);
    int32_t SelectServerById(int32_t server_id);
    
    // 跨服管理
    bool PlayerEnterCross(uint64_t role_id, int32_t src_server_id, int32_t dest_server_id, int32_t cross_type, const std::string& token);
    bool PlayerLeaveCross(uint64_t role_id);
    bool GetCrossPlayer(uint64_t role_id, CrossServerPlayer& player);
    bool IsPlayerInCross(uint64_t role_id);
    
    // 跨服匹配
    bool AddToMatchQueue(uint64_t role_id, int32_t match_type, int32_t score);
    bool RemoveFromMatchQueue(uint64_t role_id);
    bool GetMatchQueue(int32_t match_type, std::vector<uint64_t>& players);
    bool ProcessMatch(int32_t match_type, int32_t team_size);
    
    // 消息路由
    bool RouteMessage(int32_t src_server_id, int32_t dest_server_id, const std::string& msg);
    bool BroadcastMessage(ServerType type, const std::string& msg);
    bool BroadcastToAll(const std::string& msg);
    
    // 负载均衡
    bool UpdateServerLoad(int32_t server_id, int32_t online_count);
    int32_t GetServerLoad(int32_t server_id);
    
    // 定时处理
    virtual void OnTimer() override;
    
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
    // 检查服务器心跳
    void CheckServerHeartbeat();
    
    // 清理过期数据
    void CleanupExpiredData();
    
    // 服务器缓存
    std::unordered_map<int32_t, ServerInfo> servers_;
    
    // 跨服玩家缓存
    std::unordered_map<uint64_t, CrossServerPlayer> cross_players_;
    
    // 匹配队列
    std::unordered_map<int32_t, std::vector<std::pair<uint64_t, int32_t>>> match_queues_;
    
    // 互斥锁
    std::mutex server_mutex_;
    std::mutex player_mutex_;
    std::mutex match_mutex_;
    
    // 心跳超时时间（秒）
    static const int32_t HEARTBEAT_TIMEOUT = 30;
    
    // 匹配超时时间（秒）
    static const int32_t MATCH_TIMEOUT = 60;
    
    // 内部方法：通知匹配成功的玩家
    bool NotifyMatchSuccess(const std::vector<uint64_t>& role_ids, int32_t match_type, int32_t dest_server_id);
    
    // 内部方法：发送消息到指定服务器
    bool SendMessageToServer(int32_t server_id, uint32_t msg_id, const google::protobuf::Message& msg);
};

} // namespace game_server

#endif // __CENTER_SERVER_H__
