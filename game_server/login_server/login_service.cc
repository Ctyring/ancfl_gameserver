#include "login_service.h"
#include "ancfl/config.h"
#include "proto/msg_account.pb.h"
#include "proto/msg_id.pb.h"
#include <random>
#include <ctime>
#include <yaml-cpp/yaml.h>

namespace game_server {

// 日志宏，添加时间戳和统一前缀
#define LOG_INFO(msg) do { \
    time_t now = time(nullptr); \
    struct tm* tm_info = localtime(&now); \
    char time_str[20]; \
    strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", tm_info); \
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "[" << time_str << "] [登录服务器] " << msg; \
} while(0)

#define LOG_ERROR(msg) do { \
    time_t now = time(nullptr); \
    struct tm* tm_info = localtime(&now); \
    char time_str[20]; \
    strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", tm_info); \
    ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "[" << time_str << "] [登录服务器] 错误: " << msg; \
} while(0)

LoginService::LoginService()
    : GameServiceBase("login_server")
    , account_server_conn_id_(-1)
    , center_server_conn_id_(-1)
    , center_server_conn_(nullptr) {}

LoginService::~LoginService() {}

bool LoginService::InitService() {
    LOG_INFO("正在初始化登录服务器...");

    // 读取配置文件
    std::string config_path = "bin/conf/servers.yml";
    
    std::string ip = "0.0.0.0";
    int port = 8000;
    std::string center_host = "127.0.0.1";
    int center_port = 8007;
    
    // 加载配置文件
    try {
        YAML::Node root = YAML::LoadFile(config_path);
        
        // 读取登录服务器配置
        if (root["login_server"]) {
            if (root["login_server"]["host"]) {
                ip = root["login_server"]["host"].as<std::string>();
            }
            if (root["login_server"]["port"]) {
                port = root["login_server"]["port"].as<int>();
            }
        }
        
        // 读取中心服务器配置
        if (root["center_server"]) {
            if (root["center_server"]["host"]) {
                center_host = root["center_server"]["host"].as<std::string>();
            }
            if (root["center_server"]["port"]) {
                center_port = root["center_server"]["port"].as<int>();
            }
        }
        
        // 读取账号服务器配置
        if (root["account_server"]) {
            if (root["account_server"]["host"]) {
                account_server_ip_ = root["account_server"]["host"].as<std::string>();
            }
            if (root["account_server"]["port"]) {
                account_server_port_ = root["account_server"]["port"].as<int>();
            }
        }
        
        center_server_ip_ = center_host;
        center_server_port_ = center_port;
        
        LOG_INFO("绑定地址: " << ip << ":" << port);
        LOG_INFO("中心服务器地址: " << center_host << ":" << center_port);
        LOG_INFO("账号服务器地址: " << account_server_ip_ << ":" << account_server_port_);
    } catch (std::exception& e) {
        LOG_ERROR("加载配置文件失败: " << config_path << "，错误: " << e.what());
        return false;
    }

    // 绑定并监听端口
    auto addr = ancfl::IPv4Address::Create(ip.c_str(), port);
    if (!addr) {
        LOG_ERROR("创建地址失败: " << ip << ":" << port);
        return false;
    }
    
    if (!bind(addr)) {
        LOG_ERROR("绑定地址失败");
        return false;
    }
    
    if (!start()) {
        LOG_ERROR("启动服务器失败");
        return false;
    }
    
    LOG_INFO("成功绑定地址并启动服务: " << ip << ":" << port);
    
    // 连接中心服务器
    if (!ConnectToCenterServer()) {
        LOG_ERROR("连接中心服务器失败");
    } else {
        // 向中心服务器注册
        if (!RegisterToCenterServer()) {
            LOG_ERROR("向中心服务器注册失败");
        }
    }
    
    // 连接账号服务器
    if (!ConnectToAccountServer()) {
        LOG_ERROR("连接账号服务器失败");
    }

    LOG_INFO("登录服务器初始化成功");
    return true;
}

void LoginService::UninitService() {
    LOG_INFO("正在反初始化登录服务器...");
    LOG_INFO("登录服务器已反初始化");
}

void LoginService::RegisterAllHandlers() {
    // 注册消息处理器（暂时不实现）
    LOG_INFO("消息处理器注册完成");
}

void LoginService::handleClient(ancfl::Socket::ptr client) {
    LOG_INFO("新的客户端连接: " << client->getRemoteAddress()->toString());
    
    try {
        std::string recv_buffer;
        while (true) {
            // 接收消息
            char buffer[4096] = {0};
            int n = client->recv(buffer, sizeof(buffer), 0);
            if (n <= 0) {
                LOG_INFO("客户端断开连接: " << client->getRemoteAddress()->toString());
                break;
            }
            
            // 将接收到的数据添加到缓冲区
            recv_buffer.append(buffer, n);
            
            // 循环处理缓冲区中的消息
            while (true) {
                // 检查缓冲区是否包含完整的消息头
                if (recv_buffer.size() < sizeof(MessageHeader)) {
                    break; // 消息头不完整，等待更多数据
                }
                
                // 解析消息头
                MessageHeader header;
                memcpy(&header, recv_buffer.data(), sizeof(MessageHeader));
                
                // 字节序转换
                header.msg_id = ancfl::byteswapOnLittleEndian(header.msg_id);
                header.msg_len = ancfl::byteswapOnLittleEndian(header.msg_len);
                header.target_id = ancfl::byteswapOnLittleEndian(header.target_id);
                header.user_data = ancfl::byteswapOnLittleEndian(header.user_data);
                
                // 检查消息长度是否合法
                if (header.msg_len < sizeof(MessageHeader) || header.msg_len > 1024 * 1024) {
                    LOG_ERROR("消息长度异常: " << header.msg_len);
                    recv_buffer.clear(); // 清空缓冲区，重新开始
                    break;
                }
                
                // 检查缓冲区是否包含完整的消息
                if (recv_buffer.size() < header.msg_len) {
                    break; // 消息不完整，等待更多数据
                }
                
                // 提取完整消息
                std::string data = recv_buffer.substr(0, header.msg_len);
                // 从缓冲区中移除已处理的消息
                recv_buffer = recv_buffer.substr(header.msg_len);
                
                // 提取消息体
                std::string msg_data = data.substr(sizeof(MessageHeader));
                
                // 根据消息ID处理
                switch (header.msg_id) {
                    case static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_REQ):
                        HandleLoginRequest(client, msg_data);
                        break;
                    case static_cast<uint32_t>(MessageID::MSG_ACCOUNT_REG_REQ):
                        HandleRegisterRequest(client, msg_data);
                        break;
                    case static_cast<uint32_t>(MessageID::MSG_SERVER_LIST_REQ):
                        HandleServerListRequest(client, msg_data);
                        break;
                    case static_cast<uint32_t>(MessageID::MSG_SELECT_SERVER_REQ):
                        HandleSelectServerRequest(client, msg_data);
                        break;
                    default:
                        LOG_ERROR("未知消息ID: " << header.msg_id);
                        break;
                }
            }
        }
    } catch (std::exception& e) {
        LOG_ERROR("处理客户端连接异常: " << e.what());
    }
}

void LoginService::OnTimer() {
    // 每秒定时器（暂时不实现）
}

void LoginService::OnTimer5s() {
    LOG_INFO("执行5秒定时器回调");
    SendHeartbeatToCenterServer();
}

bool LoginService::ConnectToAccountServer() {
    LOG_INFO("正在连接账号服务器: " << account_server_ip_ << ":" << account_server_port_);
    
    // 创建账号服务器地址
    auto addr = ancfl::IPv4Address::Create(account_server_ip_.c_str(), account_server_port_);
    if (!addr) {
        LOG_ERROR("创建账号服务器地址失败");
        return false;
    }
    
    // 创建socket并连接
    account_server_conn_ = ancfl::Socket::CreateTCP(addr);
    if (!account_server_conn_) {
        LOG_ERROR("创建账号服务器socket失败");
        return false;
    }
    
    // 连接账号服务器
    if (!account_server_conn_->connect(addr)) {
        LOG_ERROR("连接账号服务器失败: " << account_server_ip_ << ":" << account_server_port_);
        account_server_conn_ = nullptr;
        return false;
    }
    
    // 设置超时（必须在connect之后，因为connect会创建socket文件描述符）
    account_server_conn_->setRecvTimeout(30000);
    account_server_conn_->setSendTimeout(10000);
    
    // 启动一个协程来处理账号服务器的响应
    m_worker->schedule([this]() {
        std::string recv_buffer;
        while (true) {
            if (!account_server_conn_) {
                // 暂停一段时间后继续
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            
            // 设置非阻塞模式
            int flags = fcntl(account_server_conn_->getSocket(), F_GETFL, 0);
            fcntl(account_server_conn_->getSocket(), F_SETFL, flags | O_NONBLOCK);
            
            char buffer[4096];
            int n = account_server_conn_->recv(buffer, sizeof(buffer), 0);
            if (n <= 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // 没有数据可读，暂停一段时间后继续
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                LOG_ERROR("接收账号服务器响应失败");
                break;
            }
            
            // 将接收到的数据添加到缓冲区
            recv_buffer.append(buffer, n);
            
            // 循环处理缓冲区中的消息
            while (true) {
                // 检查缓冲区是否包含完整的消息头
                if (recv_buffer.size() < sizeof(MessageHeader)) {
                    break; // 消息头不完整，等待更多数据
                }
                
                // 解析消息头
                MessageHeader header;
                memcpy(&header, recv_buffer.data(), sizeof(MessageHeader));
                
                // 字节序转换
                header.msg_id = ancfl::byteswapOnLittleEndian(header.msg_id);
                header.msg_len = ancfl::byteswapOnLittleEndian(header.msg_len);
                header.target_id = ancfl::byteswapOnLittleEndian(header.target_id);
                header.user_data = ancfl::byteswapOnLittleEndian(header.user_data);
                
                // 检查消息长度是否合法
                if (header.msg_len < sizeof(MessageHeader) || header.msg_len > 1024 * 1024) {
                    LOG_ERROR("消息长度异常: " << header.msg_len);
                    recv_buffer.clear(); // 清空缓冲区，重新开始
                    break;
                }
                
                // 检查缓冲区是否包含完整的消息
                if (recv_buffer.size() < header.msg_len) {
                    break; // 消息不完整，等待更多数据
                }
                
                // 提取完整消息
                std::string data = recv_buffer.substr(0, header.msg_len);
                // 从缓冲区中移除已处理的消息
                recv_buffer = recv_buffer.substr(header.msg_len);
                
                // 转发响应给对应的客户端
                auto it = client_map_.find(account_server_conn_.get());
                if (it != client_map_.end()) {
                    ancfl::Socket::ptr client = it->second;
                    if (client && client->isConnected()) {
                        // 转发响应给客户端
                        int send_n = client->send(data.data(), data.size(), 0);
                        if (send_n > 0) {
                            LOG_INFO("响应已成功转发给客户端");
                        } else {
                            LOG_ERROR("发送响应给客户端失败");
                        }
                    } else {
                        LOG_ERROR("客户端连接已断开");
                    }
                    // 移除已处理的客户端连接
                    client_map_.erase(it);
                } else {
                    LOG_ERROR("找不到对应的客户端连接");
                }
            }
        }
    });
    
    LOG_INFO("成功连接到账号服务器: " << account_server_ip_ << ":" << account_server_port_);
    return true;
}

int32_t LoginService::GenerateLoginCode() {
    // 使用简单的随机数生成方式
    static bool initialized = false;
    if (!initialized) {
        srand(time(nullptr));
        initialized = true;
    }
    return 100000 + (rand() % 900000);
}

bool LoginService::VerifyLoginCode(uint64_t account_id, int32_t login_code) {
    ancfl::Mutex::Lock lock(code_mutex_);
    auto it = login_codes_.find(account_id);
    if (it == login_codes_.end()) {
        return false;
    }

    // 检查过期时间
    auto expire_it = code_expire_time_.find(account_id);
    if (expire_it == code_expire_time_.end() || expire_it->second < time(nullptr)) {
        // 登录码已过期，删除
        login_codes_.erase(it);
        code_expire_time_.erase(expire_it);
        return false;
    }

    // 验证后删除
    if (it->second == login_code) {
        login_codes_.erase(it);
        code_expire_time_.erase(expire_it);
        return true;
    }

    return false;
}

bool LoginService::GetLogicServerInfo(uint64_t account_id,
                                      std::string& ip,
                                      int32_t& port) {
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
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "LoginService::OnCheckVersionReq";
    // 实现版本检查
    // 这里简单返回成功，实际应该检查客户端版本是否与服务器版本匹配
    return true;
}

bool LoginService::OnAccountRegReq(const NetPacket& packet) {
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "LoginService::OnAccountRegReq";
    // 实现账号注册请求转发到账号服务器
    // 实际应该将请求转发到账号服务器处理
    return true;
}

bool LoginService::OnAccountLoginReq(const NetPacket& packet) {
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "LoginService::OnAccountLoginReq";
    // 实现账号登录请求转发到账号服务器
    // 实际应该将请求转发到账号服务器处理
    // 登录成功后生成登录验证码
    int32_t login_code = GenerateLoginCode();
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Generated login code: " << login_code;
    return true;
}

bool LoginService::OnServerListReq(const NetPacket& packet) {
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "LoginService::OnServerListReq";
    // 实现服务器列表查询
    // 实际应该返回可用的逻辑服务器列表
    return true;
}

bool LoginService::OnSelectServerReq(const NetPacket& packet) {
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "LoginService::OnSelectServerReq";
    // 实现选择服务器
    // 实际应该验证登录码并返回逻辑服务器信息
    std::string ip;
    int32_t port = 0;
    if (GetLogicServerInfo(12345, ip, port)) {
        ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Selected server: " << ip << ":" << port;
    } else {
        ANCFL_LOG_WARN(ANCFL_LOG_ROOT()) << "No available logic server";
    }
    return true;
}

bool LoginService::OnHeartBeatReq(const NetPacket& packet) {
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "LoginService::OnHeartBeatReq";
    // 实现心跳响应
    // 实际应该更新客户端的心跳时间
    return true;
}

bool LoginService::OnLogicRegToLoginReq(const NetPacket& packet) {
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "LoginService::OnLogicRegToLoginReq";
    // 处理逻辑服务器注册请求
    // 实际应该将逻辑服务器信息添加到服务器列表
    return true;
}

bool LoginService::OnLogicUpdateReq(const NetPacket& packet) {
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "LoginService::OnLogicUpdateReq";
    // 处理逻辑服务器状态更新
    // 实际应该更新逻辑服务器的状态信息
    return true;
}

// ==================== 中心服务器连接管理 ====================

bool LoginService::ConnectToCenterServer() {
    LOG_INFO("正在连接中心服务器: " << center_server_ip_ << ":" << center_server_port_);
    
    // 创建中心服务器地址
    auto addr = ancfl::IPv4Address::Create(center_server_ip_.c_str(), center_server_port_);
    if (!addr) {
        LOG_ERROR("创建中心服务器地址失败");
        return false;
    }
    
    // 创建socket并连接
    center_server_conn_ = ancfl::Socket::CreateTCP(addr);
    if (!center_server_conn_) {
        LOG_ERROR("创建中心服务器socket失败");
        return false;
    }
    
    // 连接中心服务器
    if (!center_server_conn_->connect(addr)) {
        LOG_ERROR("连接中心服务器失败: " << center_server_ip_ << ":" << center_server_port_);
        center_server_conn_ = nullptr;
        return false;
    }
    
    // 设置超时（必须在connect之后，因为connect会创建socket文件描述符）
    center_server_conn_->setRecvTimeout(30000);
    center_server_conn_->setSendTimeout(10000);
    
    LOG_INFO("成功连接到中心服务器: " << center_server_ip_ << ":" << center_server_port_);
    return true;
}

bool LoginService::RegisterToCenterServer() {
    if (!center_server_conn_) {
        LOG_ERROR("中心服务器连接未建立");
        return false;
    }
    
    // 构建注册消息
    uint32_t msg_id = 1001; // 假设1001是注册消息ID
    std::string data = "login_server_register";
    
    // 构建消息头
    MessageHeader header;
    header.msg_id = msg_id;
    header.msg_len = sizeof(header) + data.size();
    header.target_id = 0;
    header.user_data = 0;
    
    // 字节序转换
    header.msg_id = ancfl::byteswapOnLittleEndian(header.msg_id);
    header.msg_len = ancfl::byteswapOnLittleEndian(header.msg_len);
    header.target_id = ancfl::byteswapOnLittleEndian(header.target_id);
    header.user_data = ancfl::byteswapOnLittleEndian(header.user_data);
    
    // 发送消息头
    if (center_server_conn_->send(&header, sizeof(header)) <= 0) {
        LOG_ERROR("向中心服务器发送注册消息头失败");
        return false;
    }
    
    // 发送消息体
    if (center_server_conn_->send(data.c_str(), data.size()) <= 0) {
        LOG_ERROR("向中心服务器发送注册消息体失败");
        return false;
    }
    
    LOG_INFO("向中心服务器注册成功");
    return true;
}

void LoginService::SendHeartbeatToCenterServer() {
    if (!center_server_conn_) {
        LOG_ERROR("中心服务器连接未建立");
        return;
    }
    
    // 构建心跳消息
    uint32_t msg_id = 1002; // 假设1002是心跳消息ID
    std::string data = "login_server_heartbeat";
    
    // 构建消息头
    MessageHeader header;
    header.msg_id = msg_id;
    header.msg_len = sizeof(header) + data.size();
    header.target_id = 0;
    header.user_data = 0;
    
    // 字节序转换
    header.msg_id = ancfl::byteswapOnLittleEndian(header.msg_id);
    header.msg_len = ancfl::byteswapOnLittleEndian(header.msg_len);
    header.target_id = ancfl::byteswapOnLittleEndian(header.target_id);
    header.user_data = ancfl::byteswapOnLittleEndian(header.user_data);
    
    // 发送消息头
    if (center_server_conn_->send(&header, sizeof(header)) <= 0) {
        LOG_ERROR("向中心服务器发送心跳消息头失败");
        return;
    }
    
    // 发送消息体
    if (center_server_conn_->send(data.c_str(), data.size()) <= 0) {
        LOG_ERROR("向中心服务器发送心跳消息体失败");
        return;
    }
    
    LOG_INFO("向中心服务器发送心跳成功");
}

void LoginService::HandleLoginRequest(ancfl::Socket::ptr client, const std::string& data) {
    LOG_INFO("接收到登录请求");
    
    // 检查账号服务器连接是否有效
    if (!account_server_conn_ || !account_server_conn_->isConnected()) {
        LOG_ERROR("账号服务器连接已断开，尝试重新连接");
        if (!ConnectToAccountServer()) {
            LOG_ERROR("重新连接账号服务器失败");
            return;
        }
    }
    
    // 解析登录请求
    AccountLoginReq req;
    if (!req.ParseFromString(data)) {
        LOG_ERROR("解析登录请求失败");
        return;
    }
    
    LOG_INFO("登录请求信息: 账号=" << req.account_name() << ", 密码=" << req.password());
    
    // 转发给账号服务器
    LOG_INFO("转发登录请求到账号服务器");
    
    // 保存客户端连接，用于后续转发响应
    client_map_[account_server_conn_.get()] = client;
    
    // 构建消息头
    MessageHeader header;
    header.msg_id = static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_REQ);
    header.msg_len = sizeof(MessageHeader) + data.size();
    header.target_id = 0;
    header.user_data = 0;
    
    // 字节序转换
    header.msg_id = ancfl::byteswapOnLittleEndian(header.msg_id);
    header.msg_len = ancfl::byteswapOnLittleEndian(header.msg_len);
    header.target_id = ancfl::byteswapOnLittleEndian(header.target_id);
    header.user_data = ancfl::byteswapOnLittleEndian(header.user_data);
    
    // 构建完整消息
    std::string msg_data;
    msg_data.append(reinterpret_cast<const char*>(&header), sizeof(MessageHeader));
    msg_data.append(data);
    
    // 发送消息到账号服务器
    int n = account_server_conn_->send(msg_data.data(), msg_data.size(), 0);
    if (n <= 0) {
        LOG_ERROR("发送登录请求到账号服务器失败");
        client_map_.erase(account_server_conn_.get());
        return;
    }
    
    LOG_INFO("登录请求已成功转发到账号服务器");
}

void LoginService::HandleRegisterRequest(ancfl::Socket::ptr client, const std::string& data) {
    LOG_INFO("接收到注册请求");
    
    // 检查账号服务器连接是否有效
    if (!account_server_conn_ || !account_server_conn_->isConnected()) {
        LOG_ERROR("账号服务器连接已断开，尝试重新连接");
        if (!ConnectToAccountServer()) {
            LOG_ERROR("重新连接账号服务器失败");
            return;
        }
    }
    
    // 解析注册请求
    AccountRegReq req;
    if (!req.ParseFromString(data)) {
        LOG_ERROR("解析注册请求失败");
        return;
    }
    
    LOG_INFO("注册请求信息: 账号=" << req.account_name() << ", 密码=" << req.password());
    
    // 转发给账号服务器
    LOG_INFO("转发注册请求到账号服务器");
    
    // 保存客户端连接，用于后续转发响应
    client_map_[account_server_conn_.get()] = client;
    
    // 构建消息头
    MessageHeader header;
    header.msg_id = static_cast<uint32_t>(MessageID::MSG_ACCOUNT_REG_REQ);
    header.msg_len = sizeof(MessageHeader) + data.size();
    header.target_id = 0;
    header.user_data = 0;
    
    // 字节序转换
    header.msg_id = ancfl::byteswapOnLittleEndian(header.msg_id);
    header.msg_len = ancfl::byteswapOnLittleEndian(header.msg_len);
    header.target_id = ancfl::byteswapOnLittleEndian(header.target_id);
    header.user_data = ancfl::byteswapOnLittleEndian(header.user_data);
    
    // 构建完整消息
    std::string msg_data;
    msg_data.append(reinterpret_cast<const char*>(&header), sizeof(MessageHeader));
    msg_data.append(data);
    
    // 发送消息到账号服务器
    int n = account_server_conn_->send(msg_data.data(), msg_data.size(), 0);
    if (n <= 0) {
        LOG_ERROR("发送注册请求到账号服务器失败");
        client_map_.erase(account_server_conn_.get());
        return;
    }
    
    LOG_INFO("注册请求已成功转发到账号服务器");
}

void LoginService::HandleServerListRequest(ancfl::Socket::ptr client, const std::string& data) {
    LOG_INFO("接收到服务器列表请求");
    
    // 构建服务器列表响应
    ServerListAck rsp;
    rsp.set_ret_code(0); // 成功
    
    // 填充服务器列表
    { 
        ancfl::Mutex::Lock lock(server_mutex_);
        // 直接遍历现有服务器，不添加默认服务器
        for (const auto& pair : logic_servers_) {
            const LogicServerInfo& server = pair.second;
            auto server_info = rsp.add_svr_nodes();
            server_info->set_svr_id(server.server_id);
            server_info->set_svr_name(server.server_name);
            server_info->set_svr_flag(server.cur_online < server.max_online * 0.7 ? 1 : (server.cur_online < server.max_online ? 2 : 3)); // 1:流畅, 2:拥挤, 3:爆满
            server_info->set_corner_mark(0); // 0:无
            server_info->set_svr_open_time(time(nullptr));
            server_info->set_svr_status(1); // 1:在线
            server_info->set_svr_addr(server.ip);
            server_info->set_svr_port(server.port);
        }
    }
    
    // 发送响应
    std::string rsp_data = rsp.SerializeAsString();
    MessageHeader header;
    header.msg_id = static_cast<uint32_t>(MessageID::MSG_SERVER_LIST_ACK);
    header.msg_len = sizeof(MessageHeader) + rsp_data.size();
    header.target_id = 0;
    header.user_data = 0;
    
    // 字节序转换
    header.msg_id = ancfl::byteswapOnLittleEndian(header.msg_id);
    header.msg_len = ancfl::byteswapOnLittleEndian(header.msg_len);
    header.target_id = ancfl::byteswapOnLittleEndian(header.target_id);
    header.user_data = ancfl::byteswapOnLittleEndian(header.user_data);
    
    // 构建完整消息
    std::string msg_data;
    msg_data.append(reinterpret_cast<const char*>(&header), sizeof(MessageHeader));
    msg_data.append(rsp_data);
    
    // 发送消息给客户端
    int n = client->send(msg_data.data(), msg_data.size(), 0);
    if (n > 0) {
        LOG_INFO("服务器列表响应发送成功，共" << rsp.svr_nodes_size() << "个服务器");
    } else {
        LOG_ERROR("发送服务器列表响应失败");
    }
}

void LoginService::HandleSelectServerRequest(ancfl::Socket::ptr client, const std::string& data) {
    LOG_INFO("接收到选择服务器请求");
    
    // 解析选择服务器请求
    SelectServerReq req;
    if (!req.ParseFromString(data)) {
        LOG_ERROR("解析选择服务器请求失败");
        return;
    }
    
    LOG_INFO("选择服务器请求信息: 账号ID=" << req.account_id() << ", 服务器ID=" << req.server_id() << ", 检查角色=" << req.check_role());
    
    // 分配逻辑服务器
    std::string ip;
    int32_t port = 0;
    int32_t server_id = req.server_id();
    
    { 
        ancfl::Mutex::Lock lock(server_mutex_);
        if (server_id > 0) {
            // 用户指定服务器
            auto it = logic_servers_.find(server_id);
            if (it != logic_servers_.end() && it->second.cur_online < it->second.max_online) {
                ip = it->second.ip;
                port = it->second.port;
            }
        } else {
            // 自动分配服务器
            for (const auto& pair : logic_servers_) {
                const LogicServerInfo& server = pair.second;
                if (server.cur_online < server.max_online) {
                    ip = server.ip;
                    port = server.port;
                    server_id = server.server_id;
                    break;
                }
            }
        }
    }
    
    if (ip.empty()) {
        LOG_ERROR("没有可用的逻辑服务器");
        // 返回错误响应
        SelectServerAck rsp;
        rsp.set_ret_code(1); // 没有可用服务器
        rsp.set_account_id(req.account_id());
        
        std::string rsp_data = rsp.SerializeAsString();
        MessageHeader header;
        header.msg_id = static_cast<uint32_t>(MessageID::MSG_SELECT_SERVER_ACK);
        header.msg_len = sizeof(MessageHeader) + rsp_data.size();
        header.target_id = 0;
        header.user_data = 0;
        
        // 字节序转换
        header.msg_id = ancfl::byteswapOnLittleEndian(header.msg_id);
        header.msg_len = ancfl::byteswapOnLittleEndian(header.msg_len);
        header.target_id = ancfl::byteswapOnLittleEndian(header.target_id);
        header.user_data = ancfl::byteswapOnLittleEndian(header.user_data);
        
        // 构建完整消息
        std::string msg_data;
        msg_data.append(reinterpret_cast<const char*>(&header), sizeof(MessageHeader));
        msg_data.append(rsp_data);
        
        // 发送消息给客户端
        client->send(msg_data.data(), msg_data.size(), 0);
        return;
    }
    
    // 生成登录码
    int32_t login_code = GenerateLoginCode();
    { 
        ancfl::Mutex::Lock lock(code_mutex_);
        login_codes_[req.account_id()] = login_code;
        code_expire_time_[req.account_id()] = time(nullptr) + 300; // 5分钟过期
    }
    
    // 返回逻辑服务器信息
    SelectServerAck rsp;
    rsp.set_ret_code(0); // 成功
    rsp.set_account_id(req.account_id());
    rsp.set_server_id(server_id);
    rsp.set_server_addr(ip);
    rsp.set_server_port(port);
    rsp.set_login_code(login_code);
    
    std::string rsp_data = rsp.SerializeAsString();
    MessageHeader header;
    header.msg_id = static_cast<uint32_t>(MessageID::MSG_SELECT_SERVER_ACK);
    header.msg_len = sizeof(MessageHeader) + rsp_data.size();
    header.target_id = 0;
    header.user_data = 0;
    
    // 字节序转换
    header.msg_id = ancfl::byteswapOnLittleEndian(header.msg_id);
    header.msg_len = ancfl::byteswapOnLittleEndian(header.msg_len);
    header.target_id = ancfl::byteswapOnLittleEndian(header.target_id);
    header.user_data = ancfl::byteswapOnLittleEndian(header.user_data);
    
    // 构建完整消息
    std::string msg_data;
    msg_data.append(reinterpret_cast<const char*>(&header), sizeof(MessageHeader));
    msg_data.append(rsp_data);
    
    // 发送消息给客户端
    int n = client->send(msg_data.data(), msg_data.size(), 0);
    if (n > 0) {
        LOG_INFO("选择服务器成功: 账号ID=" << req.account_id() << ", 服务器ID=" << server_id << ", 服务器=" << ip << ":" << port << ", 登录码=" << login_code);
    } else {
        LOG_ERROR("发送选择服务器响应失败");
    }
}

} // namespace game_server
