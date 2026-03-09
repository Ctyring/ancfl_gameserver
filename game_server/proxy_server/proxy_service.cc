#include "proxy_service.h"
#include "proto/msg_base.pb.h"
#include "proto/msg_account.pb.h"
#include "proto/msg_role.pb.h"
#include "proto/msg_id.pb.h"
#include "ancfl/crypto.h"

namespace game_server {

ProxyService::ProxyService() : GameServiceBase("ProxyService"),
    session_timeout_(300),
    heartbeat_timeout_(60),
    cleanup_timer_(0) {
}

ProxyService::~ProxyService() {
}

bool ProxyService::InitService() {
    // 初始化服务
    if (!GameServiceBase::InitService()) {
        return false;
    }
    
    // 加载配置
    auto config = GetConfig();
    if (config) {
        session_timeout_ = config->GetInt32("session.timeout", 300);
        heartbeat_timeout_ = config->GetInt32("heartbeat.timeout", 60);
    }
    
    // 连接逻辑服务器
    if (!ConnectToLogicServers()) {
        LOG_ERROR("Failed to connect to logic servers");
        return false;
    }
    
    // 注册消息处理器
    RegisterAllHandlers();
    
    // 设置清理定时器
    cleanup_timer_ = GetTimerMgr()->AddTimer(30000, std::bind(&ProxyService::OnTimer, this));
    
    LOG_INFO("ProxyService initialized successfully");
    return true;
}

void ProxyService::UninitService() {
    // 清理客户端会话
    client_sessions_.clear();
    account_to_conn_.clear();
    
    // 清理逻辑服务器连接
    logic_servers_.clear();
    
    // 清理定时器
    if (cleanup_timer_ > 0) {
        GetTimerMgr()->CancelTimer(cleanup_timer_);
        cleanup_timer_ = 0;
    }
    
    GameServiceBase::UninitService();
    LOG_INFO("ProxyService uninitialized");
}

void ProxyService::RegisterAllHandlers() {
    // 注册客户端消息处理器
    RegisterHandler(static_cast<uint32_t>(MessageID::MSG_CHECK_VERSION_REQ), std::bind(&ProxyService::OnCheckVersionReq, this, std::placeholders::_1));
    RegisterHandler(static_cast<uint32_t>(MessageID::MSG_ACCOUNT_REG_REQ), std::bind(&ProxyService::OnAccountRegReq, this, std::placeholders::_1));
    RegisterHandler(static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_REQ), std::bind(&ProxyService::OnAccountLoginReq, this, std::placeholders::_1));
    RegisterHandler(static_cast<uint32_t>(MessageID::MSG_SERVER_LIST_REQ), std::bind(&ProxyService::OnServerListReq, this, std::placeholders::_1));
    RegisterHandler(static_cast<uint32_t>(MessageID::MSG_SELECT_SERVER_REQ), std::bind(&ProxyService::OnSelectServerReq, this, std::placeholders::_1));
    RegisterHandler(static_cast<uint32_t>(MessageID::MSG_ROLE_CREATE_REQ), std::bind(&ProxyService::OnRoleCreateReq, this, std::placeholders::_1));
    RegisterHandler(static_cast<uint32_t>(MessageID::MSG_ROLE_LOGIN_REQ), std::bind(&ProxyService::OnRoleLoginReq, this, std::placeholders::_1));
    RegisterHandler(static_cast<uint32_t>(MessageID::MSG_ROLE_LOGOUT_REQ), std::bind(&ProxyService::OnRoleLogoutReq, this, std::placeholders::_1));
    RegisterHandler(static_cast<uint32_t>(MessageID::MSG_HEART_BEAT_REQ), std::bind(&ProxyService::OnHeartBeatReq, this, std::placeholders::_1));
    
    // 注册服务器间消息处理器
    RegisterHandler(static_cast<uint32_t>(MessageID::MSG_LOGIC_REG_TO_PROXY_REQ), std::bind(&ProxyService::OnLogicRegToProxyReq, this, std::placeholders::_1));
    RegisterHandler(static_cast<uint32_t>(MessageID::MSG_LOGIC_UPDATE_REQ), std::bind(&ProxyService::OnLogicUpdateReq, this, std::placeholders::_1));
    RegisterHandler(static_cast<uint32_t>(MessageID::MSG_LOGIC_DATA_ACK), std::bind(&ProxyService::OnLogicDataAck, this, std::placeholders::_1));
}

void ProxyService::OnTimer() {
    time_t now = time(nullptr);
    
    // 清理超时的客户端会话
    std::lock_guard<std::mutex> lock(session_mutex_);
    auto it = client_sessions_.begin();
    while (it != client_sessions_.end()) {
        if (now - it->second.last_active_time > session_timeout_) {
            LOG_INFO("Session timeout: conn_id=%u, account_id=%llu", it->first, it->second.account_id);
            
            // 断开客户端连接
            Disconnect(it->first);
            
            // 从账号映射中删除
            account_to_conn_.erase(it->second.account_id);
            
            // 删除会话
            it = client_sessions_.erase(it);
        } else {
            ++it;
        }
    }
    
    // 检查逻辑服务器连接状态
    std::lock_guard<std::mutex> server_lock(server_mutex_);
    for (auto& server : logic_servers_) {
        if (now - server.last_active_time > heartbeat_timeout_) {
            LOG_WARN("Logic server timeout: conn_id=%u, name=%s", server.conn_id, server.server_name.c_str());
            // 重新连接逻辑服务器
            // TODO: 实现重连逻辑
        }
    }
}

bool ProxyService::ConnectToLogicServers() {
    // 从配置文件读取逻辑服务器列表
    auto config = GetConfig();
    if (!config) {
        LOG_ERROR("Failed to get config");
        return false;
    }
    
    // 连接所有逻辑服务器
    auto servers = config->GetArray("logic_servers");
    for (auto& server : servers) {
        std::string ip = server.GetString("ip", "127.0.0.1");
        int32_t port = server.GetInt32("port", 8002);
        std::string name = server.GetString("name", "LogicServer");
        
        uint32_t conn_id = Connect(ip, port);
        if (conn_id == 0) {
            LOG_ERROR("Failed to connect to logic server: %s:%d", ip.c_str(), port);
            continue;
        }
        
        ServerConnection server_conn;
        server_conn.conn_id = conn_id;
        server_conn.server_name = name;
        server_conn.ip = ip;
        server_conn.port = port;
        server_conn.last_active_time = time(nullptr);
        server_conn.player_count = 0;
        
        AddLogicServer(server_conn);
        LOG_INFO("Connected to logic server: %s:%d, conn_id=%u", ip.c_str(), port, conn_id);
    }
    
    return !logic_servers_.empty();
}

void ProxyService::OnClientConnect(uint32_t conn_id, const std::string& ip, int32_t port) {
    LOG_INFO("Client connected: conn_id=%u, ip=%s, port=%d", conn_id, ip.c_str(), port);
    
    // 创建客户端会话
    ClientSession session;
    session.conn_id = conn_id;
    session.account_id = 0;
    session.role_id = 0;
    session.session_id = GenerateSessionId();
    session.logic_server_id = 0;
    session.last_active_time = time(nullptr);
    session.login_time = time(nullptr);
    session.ip = ip;
    session.port = port;
    
    std::lock_guard<std::mutex> lock(session_mutex_);
    client_sessions_[conn_id] = session;
}

void ProxyService::OnClientDisconnect(uint32_t conn_id) {
    LOG_INFO("Client disconnected: conn_id=%u", conn_id);
    
    std::lock_guard<std::mutex> lock(session_mutex_);
    auto it = client_sessions_.find(conn_id);
    if (it != client_sessions_.end()) {
        // 从账号映射中删除
        if (it->second.account_id > 0) {
            account_to_conn_.erase(it->second.account_id);
        }
        
        // 删除会话
        client_sessions_.erase(it);
    }
}

bool ProxyService::ForwardToLogicServer(uint32_t client_conn_id, uint32_t msg_id, const std::string& data) {
    // 获取客户端会话
    ClientSession* session = GetSession(client_conn_id);
    if (!session) {
        LOG_ERROR("Session not found: conn_id=%u", client_conn_id);
        return false;
    }
    
    // 选择逻辑服务器
    uint32_t logic_server_id = session->logic_server_id;
    if (logic_server_id == 0) {
        logic_server_id = SelectLogicServer();
        if (logic_server_id == 0) {
            LOG_ERROR("No available logic server");
            return false;
        }
        session->logic_server_id = logic_server_id;
    }
    
    // 转发消息到逻辑服务器
    SendMsgToServer(logic_server_id, msg_id, data);
    
    // 更新活动时间
    session->last_active_time = time(nullptr);
    
    return true;
}

bool ProxyService::ForwardToClient(uint32_t client_conn_id, uint32_t msg_id, const std::string& data) {
    // 转发消息到客户端
    SendMsgToClient(client_conn_id, msg_id, data);
    
    // 更新活动时间
    ClientSession* session = GetSession(client_conn_id);
    if (session) {
        session->last_active_time = time(nullptr);
    }
    
    return true;
}

bool ProxyService::CreateSession(uint32_t conn_id, uint64_t account_id, uint64_t role_id, const std::string& session_id) {
    std::lock_guard<std::mutex> lock(session_mutex_);
    
    auto it = client_sessions_.find(conn_id);
    if (it == client_sessions_.end()) {
        LOG_ERROR("Session not found: conn_id=%u", conn_id);
        return false;
    }
    
    it->second.account_id = account_id;
    it->second.role_id = role_id;
    it->second.session_id = session_id;
    
    account_to_conn_[account_id] = conn_id;
    
    LOG_INFO("Session created: conn_id=%u, account_id=%llu, role_id=%llu", conn_id, account_id, role_id);
    return true;
}

bool ProxyService::RemoveSession(uint32_t conn_id) {
    std::lock_guard<std::mutex> lock(session_mutex_);
    
    auto it = client_sessions_.find(conn_id);
    if (it == client_sessions_.end()) {
        return false;
    }
    
    if (it->second.account_id > 0) {
        account_to_conn_.erase(it->second.account_id);
    }
    
    client_sessions_.erase(it);
    return true;
}

ClientSession* ProxyService::GetSession(uint32_t conn_id) {
    std::lock_guard<std::mutex> lock(session_mutex_);
    auto it = client_sessions_.find(conn_id);
    if (it != client_sessions_.end()) {
        return &it->second;
    }
    return nullptr;
}

ClientSession* ProxyService::GetSessionByAccount(uint64_t account_id) {
    std::lock_guard<std::mutex> lock(session_mutex_);
    auto it = account_to_conn_.find(account_id);
    if (it != account_to_conn_.end()) {
        auto session_it = client_sessions_.find(it->second);
        if (session_it != client_sessions_.end()) {
            return &session_it->second;
        }
    }
    return nullptr;
}

uint32_t ProxyService::SelectLogicServer() {
    std::lock_guard<std::mutex> lock(server_mutex_);
    
    if (logic_servers_.empty()) {
        return 0;
    }
    
    // 选择玩家数量最少的逻辑服务器
    uint32_t best_server = 0;
    int32_t min_players = INT32_MAX;
    
    for (auto& server : logic_servers_) {
        if (server.player_count < min_players) {
            min_players = server.player_count;
            best_server = server.conn_id;
        }
    }
    
    return best_server;
}

void ProxyService::AddLogicServer(const ServerConnection& server) {
    std::lock_guard<std::mutex> lock(server_mutex_);
    logic_servers_.push_back(server);
}

void ProxyService::RemoveLogicServer(uint32_t conn_id) {
    std::lock_guard<std::mutex> lock(server_mutex_);
    auto it = std::remove_if(logic_servers_.begin(), logic_servers_.end(),
        [conn_id](const ServerConnection& server) {
            return server.conn_id == conn_id;
        });
    logic_servers_.erase(it, logic_servers_.end());
}

ServerConnection* ProxyService::GetLogicServer(uint32_t conn_id) {
    std::lock_guard<std::mutex> lock(server_mutex_);
    for (auto& server : logic_servers_) {
        if (server.conn_id == conn_id) {
            return &server;
        }
    }
    return nullptr;
}

bool ProxyService::OnCheckVersionReq(const NetPacket& packet) {
    msg_base::CheckVersionReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }
    
    // 检查版本
    bool version_ok = true;
    // TODO: 实现版本检查逻辑
    
    // 发送响应
    msg_base::CheckVersionAck ack;
    ack.set_result(version_ok ? 0 : 1);
    ack.set_latest_version("1.0.0");
    ack.set_download_url("http://example.com/download");
    SendMsgToClient(packet.conn_id, static_cast<uint32_t>(MessageID::MSG_CHECK_VERSION_ACK), ack);
    
    return true;
}

bool ProxyService::OnAccountRegReq(const NetPacket& packet) {
    msg_account::AccountRegReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }
    
    // 转发到逻辑服务器
    ForwardToLogicServer(packet.conn_id, static_cast<uint32_t>(MessageID::MSG_ACCOUNT_REG_REQ), EncodePacket(req));
    
    return true;
}

bool ProxyService::OnAccountLoginReq(const NetPacket& packet) {
    msg_account::AccountLoginReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }
    
    // 转发到逻辑服务器
    ForwardToLogicServer(packet.conn_id, static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_REQ), EncodePacket(req));
    
    return true;
}

bool ProxyService::OnServerListReq(const NetPacket& packet) {
    msg_account::ServerListReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }
    
    // 转发到逻辑服务器
    ForwardToLogicServer(packet.conn_id, static_cast<uint32_t>(MessageID::MSG_SERVER_LIST_REQ), EncodePacket(req));
    
    return true;
}

bool ProxyService::OnSelectServerReq(const NetPacket& packet) {
    msg_account::SelectServerReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }
    
    // 转发到逻辑服务器
    ForwardToLogicServer(packet.conn_id, static_cast<uint32_t>(MessageID::MSG_SELECT_SERVER_REQ), EncodePacket(req));
    
    return true;
}

bool ProxyService::OnRoleCreateReq(const NetPacket& packet) {
    msg_role::RoleCreateReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }
    
    // 转发到逻辑服务器
    ForwardToLogicServer(packet.conn_id, static_cast<uint32_t>(MessageID::MSG_ROLE_CREATE_REQ), EncodePacket(req));
    
    return true;
}

bool ProxyService::OnRoleLoginReq(const NetPacket& packet) {
    msg_role::RoleLoginReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }
    
    // 转发到逻辑服务器
    ForwardToLogicServer(packet.conn_id, static_cast<uint32_t>(MessageID::MSG_ROLE_LOGIN_REQ), EncodePacket(req));
    
    return true;
}

bool ProxyService::OnRoleLogoutReq(const NetPacket& packet) {
    msg_role::RoleLogoutReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }
    
    // 转发到逻辑服务器
    ForwardToLogicServer(packet.conn_id, static_cast<uint32_t>(MessageID::MSG_ROLE_LOGOUT_REQ), EncodePacket(req));
    
    return true;
}

bool ProxyService::OnHeartBeatReq(const NetPacket& packet) {
    msg_base::HeartBeatAck ack;
    ack.set_timestamp(time(nullptr));
    SendMsgToClient(packet.conn_id, static_cast<uint32_t>(MessageID::MSG_HEART_BEAT_ACK), ack);
    return true;
}

bool ProxyService::OnLogicRegToProxyReq(const NetPacket& packet) {
    msg_base::ServerRegReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }
    
    // 添加逻辑服务器
    ServerConnection server;
    server.conn_id = packet.conn_id;
    server.server_name = req.server_name();
    server.ip = req.ip();
    server.port = req.port();
    server.last_active_time = time(nullptr);
    server.player_count = 0;
    
    AddLogicServer(server);
    LOG_INFO("Logic server registered: conn_id=%u, name=%s", packet.conn_id, req.server_name().c_str());
    
    // 发送注册响应
    msg_base::ServerRegAck ack;
    ack.set_result(0);
    SendMsgToServer(packet.conn_id, static_cast<uint32_t>(MessageID::MSG_SERVER_REG_ACK), ack);
    
    return true;
}

bool ProxyService::OnLogicUpdateReq(const NetPacket& packet) {
    msg_base::ServerUpdateReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }
    
    // 更新逻辑服务器信息
    ServerConnection* server = GetLogicServer(packet.conn_id);
    if (server) {
        server->last_active_time = time(nullptr);
        server->player_count = req.player_count();
    }
    
    return true;
}

bool ProxyService::OnLogicDataAck(const NetPacket& packet) {
    // 转发数据到客户端
    // 从数据包中获取客户端连接ID
    // TODO: 实现客户端连接ID的传递机制
    
    return true;
}

std::string ProxyService::GenerateSessionId() {
    // 生成会话ID
    static uint64_t next_id = time(nullptr) * 10000 + rand() % 10000;
    return std::to_string(next_id++);
}

} // namespace game_server
