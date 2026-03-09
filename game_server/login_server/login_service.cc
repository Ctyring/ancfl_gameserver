#include "login_service.h"
#include "ancfl/config.h"

namespace game_server {

LoginService::LoginService()
    : GameServiceBase("LoginServer")
    , account_server_conn_id_(-1)
    , center_server_conn_id_(-1) {
}

LoginService::~LoginService() {
}

bool LoginService::InitService() {
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Initializing LoginServer...";

    // 加载配置
    auto config = ancfl::Config::LoadFromYamlFile("conf/login_server.yml");
    if (!config) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to load login_server.yml";
        return false;
    }

    auto service_config = config->getConfig("service");
    if (!service_config) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Missing service config";
        return false;
    }

    std::string ip = service_config->getValue<std::string>("ip", "0.0.0.0");
    int port = service_config->getValue<int>("port", 8000);
    service_id_ = service_config->getValue<int>("id", 1);

    // 读取连接配置
    auto conn_config = config->getConfig("connections");
    if (conn_config) {
        auto account_config = conn_config->getConfig("account_server");
        if (account_config) {
            account_server_ip_ = account_config->getValue<std::string>("ip", "127.0.0.1");
            account_server_port_ = account_config->getValue<int>("port", 8100);
        }

        auto center_config = conn_config->getConfig("center_server");
        if (center_config) {
            center_server_ip_ = center_config->getValue<std::string>("ip", "127.0.0.1");
            center_server_port_ = center_config->getValue<int>("port", 8200);
        }
    }

    // 初始化TCP服务
    if (!Init(ip, port)) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to init TCP service";
        return false;
    }

    // 创建消息分发器
    dispatcher_.reset(new MessageDispatcher());
    RegisterAllHandlers();
    SetMessageDispatcher(dispatcher_);

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "LoginServer initialized on " << ip << ":" << port;
    return true;
}

void LoginService::UninitService() {
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Uninitializing LoginServer...";
    Uninit();
}

void LoginService::RegisterAllHandlers() {
    // 客户端消息
    REGISTER_MESSAGE(dispatcher_, 100001, &LoginService::OnCheckVersionReq);  // MSG_CHECK_VERSION_REQ
    REGISTER_MESSAGE(dispatcher_, 100003, &LoginService::OnAccountRegReq);    // MSG_ACCOUNT_REG_REQ
    REGISTER_MESSAGE(dispatcher_, 100005, &LoginService::OnAccountLoginReq);  // MSG_ACCOUNT_LOGIN_REQ
    REGISTER_MESSAGE(dispatcher_, 100009, &LoginService::OnServerListReq);    // MSG_SERVER_LIST_REQ
    REGISTER_MESSAGE(dispatcher_, 100011, &LoginService::OnSelectServerReq);  // MSG_SELECT_SERVER_REQ
    REGISTER_MESSAGE(dispatcher_, 100024, &LoginService::OnHeartBeatReq);     // MSG_WATCH_HEART_BEAT_REQ

    // 服务器间消息
    REGISTER_MESSAGE(dispatcher_, 100013, &LoginService::OnLogicRegToLoginReq); // MSG_LOGIC_REGTO_LOGIN_REQ
    REGISTER_MESSAGE(dispatcher_, 100015, &LoginService::OnLogicUpdateReq);     // MSG_LOGIC_UPDATE_REQ

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Registered " << dispatcher_->GetHandlerCount() << " message handlers";
}

void LoginService::OnTimer() {
    // 清理过期的登录验证码
    int64_t now = time(nullptr);
    std::vector<uint64_t> expired_codes;
    
    {
        ancfl::Mutex::Lock lock(code_mutex_);
        for (auto& pair : code_expire_time_) {
            if (now > pair.second) {
                expired_codes.push_back(pair.first);
            }
        }

        for (auto account_id : expired_codes) {
            login_codes_.erase(account_id);
            code_expire_time_.erase(account_id);
        }
    }

    // 清理不活跃的逻辑服务器
    std::vector<int32_t> inactive_servers;
    {
        ancfl::Mutex::Lock lock(server_mutex_);
        for (auto& pair : logic_servers_) {
            if (now - pair.second.last_update_time > 120) { // 2分钟未更新
                inactive_servers.push_back(pair.first);
            }
        }

        for (auto server_id : inactive_servers) {
            ANCFL_LOG_WARN(ANCFL_LOG_ROOT()) << "Logic server " << server_id << " inactive, removing";
            logic_servers_.erase(server_id);
        }
    }

    // 调用基类定时器
    OnSecondTimer();
}

bool LoginService::ConnectToAccountServer() {
    // TODO: 实现连接账号服务器
    return true;
}

int32_t LoginService::GenerateLoginCode() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int32_t> dis(100000, 999999);
    return dis(gen);
}

bool LoginService::VerifyLoginCode(uint64_t account_id, int32_t login_code) {
    ancfl::Mutex::Lock lock(code_mutex_);
    auto it = login_codes_.find(account_id);
    if (it == login_codes_.end()) {
        return false;
    }

    // 验证后删除
    if (it->second == login_code) {
        login_codes_.erase(it);
        code_expire_time_.erase(account_id);
        return true;
    }

    return false;
}

bool LoginService::GetLogicServerInfo(uint64_t account_id, std::string& ip, int32_t& port) {
    // TODO: 根据账号ID获取对应的逻辑服信息
    // 暂时返回第一个可用的逻辑服
    ancfl::Mutex::Lock lock(server_mutex_);
    for (auto& pair : logic_servers_) {
        if (pair.second.cur_online < pair.second.max_online) {
            ip = pair.second.ip;
            port = pair.second.port;
            return true;
        }
    }
    return false;
}

// ==================== 消息处理器 ====================

bool LoginService::OnCheckVersionReq(const NetPacket& packet) {
    // TODO: 实现版本检查
    return true;
}

bool LoginService::OnAccountRegReq(const NetPacket& packet) {
    // TODO: 实现账号注册请求转发到账号服务器
    return true;
}

bool LoginService::OnAccountLoginReq(const NetPacket& packet) {
    // TODO: 实现账号登录请求转发到账号服务器
    return true;
}

bool LoginService::OnServerListReq(const NetPacket& packet) {
    // TODO: 实现服务器列表查询
    return true;
}

bool LoginService::OnSelectServerReq(const NetPacket& packet) {
    // TODO: 实现选择服务器
    return true;
}

bool LoginService::OnHeartBeatReq(const NetPacket& packet) {
    // TODO: 实现心跳响应
    return true;
}

bool LoginService::OnLogicRegToLoginReq(const NetPacket& packet) {
    // TODO: 处理逻辑服务器注册请求
    return true;
}

bool LoginService::OnLogicUpdateReq(const NetPacket& packet) {
    // TODO: 处理逻辑服务器状态更新
    return true;
}

} // namespace game_server
