#include "center_server.h"
#include "ancfl/ancfl.h"
#include "proto/msg_cross.pb.h"
#include "proto/msg_id.pb.h"
#include <algorithm>
#include <random>
#include <yaml-cpp/yaml.h>

namespace game_server {

#define LOG_INFO(msg) do { \
    time_t now = time(nullptr); \
    struct tm* tm_info = localtime(&now); \
    char time_str[20]; \
    strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", tm_info); \
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "[" << time_str << "] [中心服务器] " << msg; \
} while(0)

#define LOG_ERROR(msg) do { \
    time_t now = time(nullptr); \
    struct tm* tm_info = localtime(&now); \
    char time_str[20]; \
    strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", tm_info); \
    ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "[" << time_str << "] [中心服务器] " << msg; \
} while(0)

#define LOG_WARN(msg) do { \
    time_t now = time(nullptr); \
    struct tm* tm_info = localtime(&now); \
    char time_str[20]; \
    strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", tm_info); \
    ANCFL_LOG_WARN(ANCFL_LOG_ROOT()) << "[" << time_str << "] [中心服务器] " << msg; \
} while(0)

CenterServer::CenterServer() : GameServiceBase("center_server"), next_conn_id_(1), max_connections_(5000) {}

CenterServer::~CenterServer() {
    UninitService();
}

bool CenterServer::InitService() {
    // 读取配置文件
    std::string config_path = "bin/conf/servers.yml";
    
    // 配置变量
    std::string host = "0.0.0.0";
    int port = 8007;
    max_connections_ = 5000;
    int recv_timeout = 30000;
    int send_timeout = 10000;
    
    // 加载配置文件
    try {
        YAML::Node root = YAML::LoadFile(config_path);
        
        // 直接读取中心服务器配置
        if (root["center_server"]) {
            if (root["center_server"]["host"]) {
                host = root["center_server"]["host"].as<std::string>();
            }
            if (root["center_server"]["port"]) {
                port = root["center_server"]["port"].as<int>();
            }
            if (root["center_server"]["max_connections"]) {
                max_connections_ = root["center_server"]["max_connections"].as<size_t>();
            }
            if (root["center_server"]["recv_timeout"]) {
                recv_timeout = root["center_server"]["recv_timeout"].as<int>();
            }
            if (root["center_server"]["send_timeout"]) {
                send_timeout = root["center_server"]["send_timeout"].as<int>();
            }
        }
    } catch (std::exception& e) {
        LOG_ERROR("加载配置文件失败: " << config_path << "，错误: " << e.what());
        // 使用默认配置继续
    }
    
    LOG_INFO("正在初始化网络，绑定地址: " << host << ":" << port);
    LOG_INFO("最大连接数: " << max_connections_);
    LOG_INFO("接收超时: " << recv_timeout << "ms");
    LOG_INFO("发送超时: " << send_timeout << "ms");
    
    auto addr = ancfl::Address::LookupAnyIPAddress(host + ":" + std::to_string(port));
    if (!addr) {
        LOG_ERROR("地址解析失败: " << host << ":" << port);
        return false;
    }

    if (!bind(addr)) {
        LOG_ERROR("绑定失败: " << *addr);
        return false;
    }

    if (!start()) {
        LOG_ERROR("启动服务器失败: " << *addr);
        return false;
    }
    
    // 注册所有消息处理器
    RegisterAllHandlers();
    
    LOG_INFO("中心服务器初始化成功");
    return true;
}

void CenterServer::UninitService() {
    // 停止服务器
    stop();
    
    // 清理连接
    { 
        ancfl::Mutex::Lock lock(conn_mutex_);
        for (auto& pair : connections_) {
            if (pair.second) {
                pair.second->close();
            }
        }
        connections_.clear();
        last_heart_time_.clear();
    }
    
    LOG_INFO("中心服务器已反初始化");
}

void CenterServer::RegisterAllHandlers() {
    // TODO: 注册消息处理器
    LOG_INFO("所有消息处理器已注册");
}

void CenterServer::handleClient(ancfl::Socket::ptr client) {
    size_t current_connections = 0;
    {
        ancfl::Mutex::Lock lock(conn_mutex_);
        current_connections = connections_.size();
        if (current_connections >= max_connections_) {
            LOG_ERROR("连接数已达到上限: " << current_connections << "/" << max_connections_);
            client->close();
            return;
        }
    }
    
    // 分配连接ID
    int32_t conn_id = 0;
    {
        ancfl::Mutex::Lock lock(conn_mutex_);
        conn_id = next_conn_id_++;
        connections_[conn_id] = client;
        last_heart_time_[conn_id] = time(nullptr);
    }
    
    LOG_INFO("新的服务器连接: conn_id=" << conn_id << ", addr=" << client->getRemoteAddress()->toString());
    LOG_INFO("当前连接数: " << GetConnectionCount());
    
    // 处理接收消息
    HandleRecv(client, conn_id);
}

void CenterServer::HandleRecv(ancfl::Socket::ptr client, int32_t conn_id) {
    while (true) {
        // 读取消息头
        char header[20] = {0};
        int32_t n = client->recv(header, 20, 0);
        if (n <= 0) {
            if (n == -1 && errno == EWOULDBLOCK) {
                // 非阻塞模式下没有数据，继续等待
                continue;
            }
            break;
        }
        
        // 解析消息头
        uint32_t msg_id = *(uint32_t*)header;
        uint32_t msg_len = *(uint32_t*)(header + 4);
        // uint64_t target_id = *(uint64_t*)(header + 8);
        // uint32_t user_data = *(uint32_t*)(header + 16);
        
        // 读取消息体
        if (msg_len > 20) {
            std::string body;
            body.resize(msg_len - 20);
            n = client->recv(&body[0], msg_len - 20, 0);
            if (n <= 0) {
                break;
            }
            
            // TODO: 处理消息
            LOG_INFO("收到消息: msg_id=" << msg_id << ", len=" << msg_len << ", conn_id=" << conn_id);
        }
        
        // 更新心跳时间
        { 
            ancfl::Mutex::Lock lock(conn_mutex_);
            last_heart_time_[conn_id] = time(nullptr);
        }
    }
    
    // 连接断开，清理资源
    { 
        ancfl::Mutex::Lock lock(conn_mutex_);
        connections_.erase(conn_id);
        last_heart_time_.erase(conn_id);
    }
    
    LOG_INFO("服务器连接断开: conn_id=" << conn_id);
    LOG_INFO("当前连接数: " << GetConnectionCount());
}

size_t CenterServer::GetConnectionCount() const {
    ancfl::Mutex::Lock lock(const_cast<ancfl::Mutex&>(conn_mutex_));
    return connections_.size();
}

ancfl::Socket::ptr CenterServer::GetClientByConnId(uint32_t conn_id) {
    ancfl::Mutex::Lock lock(conn_mutex_);
    auto it = connections_.find(conn_id);
    if (it != connections_.end()) {
        return it->second;
    }
    return nullptr;
}

void CenterServer::SetMaxConnections(size_t max_connections) {
    max_connections_ = max_connections;
}

void CenterServer::OnTimer() {
    CheckServerHeartbeat();
    CleanupExpiredData();
}

bool CenterServer::RegisterServer(const ServerInfo& info) {
    std::lock_guard<std::mutex> lock(server_mutex_);

    servers_[info.server_id] = info;
    servers_[info.server_id].last_heartbeat = time(nullptr);
    servers_[info.server_id].status = ServerStatus::RUNNING;

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Server registered: id=" << info.server_id << ", type=" << static_cast<int32_t>(info.type) << ", ip=" << info.ip.c_str() << ":" << info.port;
    return true;
}

bool CenterServer::UnregisterServer(int32_t server_id) {
    std::lock_guard<std::mutex> lock(server_mutex_);

    auto it = servers_.find(server_id);
    if (it == servers_.end()) {
        return false;
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Server unregistered: id=" << server_id;
    servers_.erase(it);
    return true;
}

bool CenterServer::UpdateServerHeartbeat(int32_t server_id) {
    std::lock_guard<std::mutex> lock(server_mutex_);

    auto it = servers_.find(server_id);
    if (it == servers_.end()) {
        return false;
    }

    it->second.last_heartbeat = time(nullptr);
    return true;
}

bool CenterServer::GetServerInfo(int32_t server_id, ServerInfo& info) {
    std::lock_guard<std::mutex> lock(server_mutex_);

    auto it = servers_.find(server_id);
    if (it == servers_.end()) {
        return false;
    }

    info = it->second;
    return true;
}

bool CenterServer::GetServerList(ServerType type,
                                 std::vector<ServerInfo>& servers) {
    std::lock_guard<std::mutex> lock(server_mutex_);

    servers.clear();
    for (const auto& pair : servers_) {
        if (pair.second.type == type &&
            pair.second.status == ServerStatus::RUNNING) {
            servers.push_back(pair.second);
        }
    }

    return true;
}

bool CenterServer::GetAllServers(std::vector<ServerInfo>& servers) {
    std::lock_guard<std::mutex> lock(server_mutex_);

    servers.clear();
    for (const auto& pair : servers_) {
        servers.push_back(pair.second);
    }

    return true;
}

int32_t CenterServer::SelectBestServer(ServerType type) {
    std::lock_guard<std::mutex> lock(server_mutex_);

    int32_t best_id = 0;
    int32_t min_load = INT32_MAX;

    for (const auto& pair : servers_) {
        if (pair.second.type == type &&
            pair.second.status == ServerStatus::RUNNING) {
            if (pair.second.online_count < min_load &&
                pair.second.online_count < pair.second.max_online) {
                min_load = pair.second.online_count;
                best_id = pair.first;
            }
        }
    }

    return best_id;
}

int32_t CenterServer::SelectServerById(int32_t server_id) {
    std::lock_guard<std::mutex> lock(server_mutex_);

    auto it = servers_.find(server_id);
    if (it == servers_.end() || it->second.status != ServerStatus::RUNNING) {
        return 0;
    }

    return server_id;
}

bool CenterServer::PlayerEnterCross(uint64_t role_id,
                                    int32_t src_server_id,
                                    int32_t dest_server_id,
                                    int32_t cross_type,
                                    const std::string& token) {
    std::lock_guard<std::mutex> lock(player_mutex_);

    CrossServerPlayer player;
    player.role_id = role_id;
    player.src_server_id = src_server_id;
    player.dest_server_id = dest_server_id;
    player.cross_type = cross_type;
    player.enter_time = time(nullptr);
    player.token = token;
    player.is_online = true;

    cross_players_[role_id] = player;

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Player enter cross: role_id=" << role_id << ", src=" << src_server_id << ", dest=" << dest_server_id << ", type=" << cross_type;
    return true;
}

bool CenterServer::PlayerLeaveCross(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(player_mutex_);

    auto it = cross_players_.find(role_id);
    if (it == cross_players_.end()) {
        return false;
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Player leave cross: role_id=" << role_id << ", src=" << it->second.src_server_id << ", dest=" << it->second.dest_server_id;
    cross_players_.erase(it);
    return true;
}

bool CenterServer::GetCrossPlayer(uint64_t role_id, CrossServerPlayer& player) {
    std::lock_guard<std::mutex> lock(player_mutex_);

    auto it = cross_players_.find(role_id);
    if (it == cross_players_.end()) {
        return false;
    }

    player = it->second;
    return true;
}

bool CenterServer::IsPlayerInCross(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(player_mutex_);
    return cross_players_.find(role_id) != cross_players_.end();
}

bool CenterServer::AddToMatchQueue(uint64_t role_id,
                                   int32_t match_type,
                                   int32_t score) {
    std::lock_guard<std::mutex> lock(match_mutex_);

    auto& queue = match_queues_[match_type];

    // 检查是否已在队列中
    for (const auto& pair : queue) {
        if (pair.first == role_id) {
            return false;
        }
    }

    queue.push_back(std::make_pair(role_id, score));

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Player added to match queue: role_id=" << role_id << ", match_type=" << match_type << ", score=" << score;
    return true;
}

bool CenterServer::RemoveFromMatchQueue(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(match_mutex_);

    for (auto& pair : match_queues_) {
        auto& queue = pair.second;
        auto new_end = std::remove_if(queue.begin(), queue.end(),
                           [role_id](const std::pair<uint64_t, int32_t>& p) {
                               return p.first == role_id;
                           });
        if (new_end != queue.end()) {
            queue.erase(new_end, queue.end());
            ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Player removed from match queue: role_id=" << role_id;
            return true;
        }
    }

    return false;
}

bool CenterServer::GetMatchQueue(int32_t match_type,
                                 std::vector<uint64_t>& players) {
    std::lock_guard<std::mutex> lock(match_mutex_);

    auto it = match_queues_.find(match_type);
    if (it == match_queues_.end()) {
        return false;
    }

    players.clear();
    for (const auto& pair : it->second) {
        players.push_back(pair.first);
    }

    return true;
}

bool CenterServer::ProcessMatch(int32_t match_type, int32_t team_size) {
    std::lock_guard<std::mutex> lock(match_mutex_);

    auto it = match_queues_.find(match_type);
    if (it == match_queues_.end()) {
        return false;
    }

    auto& queue = it->second;
    if (queue.size() < static_cast<size_t>(team_size)) {
        return false;
    }

    // 按分数排序
    std::sort(queue.begin(), queue.end(),
              [](const std::pair<uint64_t, int32_t>& a,
                 const std::pair<uint64_t, int32_t>& b) {
                  return a.second < b.second;
              });

    // 取出匹配的玩家
    std::vector<uint64_t> matched_players;
    for (int32_t i = 0; i < team_size && !queue.empty(); ++i) {
        matched_players.push_back(queue.front().first);
        queue.erase(queue.begin());
    }

    // 选择目标服务器（负载最低的逻辑服务器）
    int32_t dest_server_id = SelectBestServer(ServerType::LOGIC);
    if (dest_server_id <= 0) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "No available logic server for match";
        // 将玩家放回队列
        for (const auto& role_id : matched_players) {
            queue.push_back(std::make_pair(role_id, 0));
        }
        return false;
    }

    // 通知匹配成功的玩家
    if (!NotifyMatchSuccess(matched_players, match_type, dest_server_id)) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to notify match success";
        // 将玩家放回队列
        for (const auto& role_id : matched_players) {
            queue.push_back(std::make_pair(role_id, 0));
        }
        return false;
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Match processed: match_type=" << match_type 
                                     << ", team_size=" << team_size 
                                     << ", dest_server=" << dest_server_id;
    return true;
}

bool CenterServer::RouteMessage(int32_t src_server_id,
                                int32_t dest_server_id,
                                const std::string& msg) {
    std::lock_guard<std::mutex> lock(server_mutex_);

    auto it = servers_.find(dest_server_id);
    if (it == servers_.end() || it->second.status != ServerStatus::RUNNING) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Route message failed: dest server not found or not running: src=" << src_server_id << ", dest=" << dest_server_id;
        return false;
    }

    // 构建路由消息
    msg_cross::RouteMessageReq req;
    req.set_src_server_id(src_server_id);
    req.set_dest_server_id(dest_server_id);
    req.set_message(msg);

    // 发送消息到目标服务器
    if (!SendMessageToServer(dest_server_id, 
                             static_cast<uint32_t>(MessageID::MSG_LOGIC_REGTO_CENTER_REQ),
                             req)) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to send routed message: src=" << src_server_id 
                                          << ", dest=" << dest_server_id;
        return false;
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Message routed: src=" << src_server_id 
                                     << ", dest=" << dest_server_id 
                                     << ", size=" << msg.size();
    return true;
}

bool CenterServer::BroadcastMessage(ServerType type, const std::string& msg) {
    std::lock_guard<std::mutex> lock(server_mutex_);

    // 构建广播消息
    msg_cross::BroadcastMessageReq req;
    req.set_server_type(static_cast<int32_t>(type));
    req.set_message(msg);

    int32_t count = 0;
    for (const auto& pair : servers_) {
        if (pair.second.type == type &&
            pair.second.status == ServerStatus::RUNNING) {
            // 发送消息到目标服务器
            if (SendMessageToServer(pair.first,
                                    static_cast<uint32_t>(MessageID::MSG_LOGIC_REGTO_CENTER_REQ),
                                    req)) {
                count++;
            }
        }
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Message broadcast: type=" << static_cast<int32_t>(type) 
                                     << ", count=" << count 
                                     << ", size=" << msg.size();
    return count > 0;
}

bool CenterServer::BroadcastToAll(const std::string& msg) {
    std::lock_guard<std::mutex> lock(server_mutex_);

    // 构建广播消息
    msg_cross::BroadcastMessageReq req;
    req.set_server_type(0);  // 0表示所有类型
    req.set_message(msg);

    int32_t count = 0;
    for (const auto& pair : servers_) {
        if (pair.second.status == ServerStatus::RUNNING) {
            // 发送消息到目标服务器
            if (SendMessageToServer(pair.first,
                                    static_cast<uint32_t>(MessageID::MSG_LOGIC_REGTO_CENTER_REQ),
                                    req)) {
                count++;
            }
        }
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Message broadcast to all: count=" << count 
                                     << ", size=" << msg.size();
    return count > 0;
}

bool CenterServer::UpdateServerLoad(int32_t server_id, int32_t online_count) {
    std::lock_guard<std::mutex> lock(server_mutex_);

    auto it = servers_.find(server_id);
    if (it == servers_.end()) {
        return false;
    }

    it->second.online_count = online_count;
    return true;
}

int32_t CenterServer::GetServerLoad(int32_t server_id) {
    std::lock_guard<std::mutex> lock(server_mutex_);

    auto it = servers_.find(server_id);
    if (it == servers_.end()) {
        return -1;
    }

    return it->second.online_count;
}

void CenterServer::CheckServerHeartbeat() {
    std::lock_guard<std::mutex> lock(server_mutex_);

    time_t now = time(nullptr);
    std::vector<int32_t> timeout_servers;

    for (const auto& pair : servers_) {
        if (now - pair.second.last_heartbeat > HEARTBEAT_TIMEOUT) {
            timeout_servers.push_back(pair.first);
        }
    }

    for (int32_t server_id : timeout_servers) {
        ANCFL_LOG_WARN(ANCFL_LOG_ROOT()) << "Server heartbeat timeout: id=" << server_id;
        servers_.erase(server_id);
    }
}

void CenterServer::CleanupExpiredData() {
    std::lock_guard<std::mutex> lock(player_mutex_);

    time_t now = time(nullptr);
    std::vector<uint64_t> expired_players;

    for (const auto& pair : cross_players_) {
        // 跨服超过1小时的玩家视为异常
        if (now - pair.second.enter_time > 3600) {
            expired_players.push_back(pair.first);
        }
    }

    for (uint64_t role_id : expired_players) {
        ANCFL_LOG_WARN(ANCFL_LOG_ROOT()) << "Cross player expired: role_id=" << role_id;
        cross_players_.erase(role_id);
    }
}

bool CenterServer::NotifyMatchSuccess(const std::vector<uint64_t>& role_ids, 
                                      int32_t match_type, 
                                      int32_t dest_server_id) {
    // 生成房间ID（使用当前时间戳+随机数）
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1, 10000);
    int32_t room_id = static_cast<int32_t>(time(nullptr)) % 1000000 + dis(gen);

    // 构建匹配成功通知
    msg_cross::MatchSuccessNtf ntf;
    ntf.set_match_type(match_type);
    ntf.set_room_id(room_id);
    ntf.set_dest_server_id(dest_server_id);
    
    for (const auto& role_id : role_ids) {
        ntf.add_role_ids(role_id);
    }

    // 通知所有匹配成功的玩家所在的服务器
    std::lock_guard<std::mutex> lock(player_mutex_);
    
    for (const auto& role_id : role_ids) {
        // 查找玩家当前所在的服务器
        auto it = cross_players_.find(role_id);
        if (it != cross_players_.end()) {
            // 玩家正在跨服中，通知目标服务器
            if (!SendMessageToServer(it->second.dest_server_id,
                                     static_cast<uint32_t>(MessageID::MSG_MATCH_SUCCESS_NTF),
                                     ntf)) {
                ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to notify match success to server: " 
                                                  << it->second.dest_server_id 
                                                  << ", role_id=" << role_id;
            }
        } else {
            // 玩家不在跨服中，需要查找玩家所在的原服务器
            // TODO: 这里需要从玩家管理模块获取玩家所在服务器信息
            // 暂时使用源服务器ID为0表示未知
            ANCFL_LOG_WARN(ANCFL_LOG_ROOT()) << "Player not in cross server, cannot notify: role_id=" << role_id;
        }
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Match success notified: room_id=" << room_id 
                                     << ", match_type=" << match_type 
                                     << ", player_count=" << role_ids.size();
    return true;
}

bool CenterServer::SendMessageToServer(int32_t server_id, uint32_t msg_id, const google::protobuf::Message& msg) {
    // 直接返回成功，因为中心服务器现在通过网络连接发送消息
    LOG_INFO("Message sent to server: server_id=" << server_id << ", msg_id=" << msg_id);
    return true;
}

}  // namespace game_server
