#include "center_server.h"
#include "ancfl/ancfl.h"
#include <algorithm>
#include <random>

namespace game_server {

CenterServer::CenterServer() : is_running_(false) {}

CenterServer::~CenterServer() {
    Stop();
}

bool CenterServer::Init(const std::string& config_file) {
    // TODO: 从配置文件加载配置
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Center server initializing: config=" << config_file.c_str();
    return true;
}

bool CenterServer::Start() {
    if (is_running_) {
        return true;
    }

    is_running_ = true;
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Center server started";
    return true;
}

void CenterServer::Stop() {
    is_running_ = false;
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Center server stopped";
}

bool CenterServer::IsRunning() {
    return is_running_;
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
        auto it =
            std::remove_if(queue.begin(), queue.end(),
                           [role_id](const std::pair<uint64_t, int32_t>& p) {
                               return p.first == role_id;
                           });
        if (it != queue.end()) {
            queue.erase(it, queue.end());
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

    // TODO: 通知匹配成功的玩家

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Match processed: match_type=" << match_type << ", team_size=" << team_size;
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

    // TODO: 实际发送消息到目标服务器
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Message routed: src=" << src_server_id << ", dest=" << dest_server_id << ", size=" << msg.size();
    return true;
}

bool CenterServer::BroadcastMessage(ServerType type, const std::string& msg) {
    std::lock_guard<std::mutex> lock(server_mutex_);

    int32_t count = 0;
    for (const auto& pair : servers_) {
        if (pair.second.type == type &&
            pair.second.status == ServerStatus::RUNNING) {
            // TODO: 实际发送消息
            count++;
        }
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Message broadcast: type=" << static_cast<int32_t>(type) << ", count=" << count << ", size=" << msg.size();
    return true;
}

bool CenterServer::BroadcastToAll(const std::string& msg) {
    std::lock_guard<std::mutex> lock(server_mutex_);

    int32_t count = 0;
    for (const auto& pair : servers_) {
        if (pair.second.status == ServerStatus::RUNNING) {
            // TODO: 实际发送消息
            count++;
        }
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Message broadcast to all: count=" << count << ", size=" << msg.size();
    return true;
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

void CenterServer::OnTimer() {
    CheckServerHeartbeat();
    CleanupExpiredData();
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

}  // namespace game_server
