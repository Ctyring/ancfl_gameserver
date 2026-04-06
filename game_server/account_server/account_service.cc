#include "account_service.h"
#include "ancfl/config.h"
#include "ancfl/util/hash_util.h"
#include "proto/msg_account.pb.h"
#include "proto/msg_id.pb.h"
#include <ctime>
#include <yaml-cpp/yaml.h>

namespace game_server {

// 日志宏，添加时间戳和统一前缀
#define LOG_INFO(msg) do { \
    time_t now = time(nullptr); \
    struct tm* tm_info = localtime(&now); \
    char time_str[20]; \
    strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", tm_info); \
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "[" << time_str << "] [账号服务器] " << msg; \
} while(0)

#define LOG_ERROR(msg) do { \
    time_t now = time(nullptr); \
    struct tm* tm_info = localtime(&now); \
    char time_str[20]; \
    strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", tm_info); \
    ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "[" << time_str << "] [账号服务器] 错误: " << msg; \
} while(0)

AccountService::AccountService()
    : GameServiceBase("account_server")
    , center_server_id_(3)
    , center_server_ip_("127.0.0.1")
    , center_server_port_(8011)
    , center_server_conn_(nullptr)
    , db_server_ip_("127.0.0.1")
    , db_server_port_(8010)
    , db_server_conn_(nullptr) {}

AccountService::~AccountService() {}

bool AccountService::InitService() {
    LOG_INFO("正在初始化账号服务器...");

    // 读取配置文件
    std::string config_path = "bin/conf/servers.yml";
    
    std::string ip = "0.0.0.0";
    int port = 8200;
    std::string center_host = "127.0.0.1";
    int center_port = 8007;
    
    // 加载配置文件
    try {
        YAML::Node root = YAML::LoadFile(config_path);
        
        // 读取账号服务器配置
        if (root["account_server"]) {
            if (root["account_server"]["host"]) {
                ip = root["account_server"]["host"].as<std::string>();
            }
            if (root["account_server"]["port"]) {
                port = root["account_server"]["port"].as<int>();
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
        
        center_server_ip_ = center_host;
        center_server_port_ = center_port;
        
        LOG_INFO("绑定地址: " << ip << ":" << port);
        LOG_INFO("中心服务器地址: " << center_host << ":" << center_port);
    } catch (std::exception& e) {
        LOG_ERROR("加载配置文件失败: " << config_path << "，错误: " << e.what());
        return false;
    }
    
    // 数据库服务器配置
    db_server_ip_ = "127.0.0.1";
    db_server_port_ = 8009;
    
    // 连接数据库服务器
    if (!ConnectToDBServer()) {
        LOG_ERROR("连接数据库服务器失败");
        // 继续运行，不退出
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

    LOG_INFO("账号服务器初始化成功");
    return true;
}

void AccountService::UninitService() {
    LOG_INFO("正在反初始化账号服务器...");
    if (db_server_conn_) {
        db_server_conn_->close();
        db_server_conn_ = nullptr;
    }
    LOG_INFO("账号服务器已反初始化");
}

void AccountService::RegisterAllHandlers() {
    // 注册消息处理器（暂时不实现）
    LOG_INFO("消息处理器注册完成");
}

void AccountService::handleClient(ancfl::Socket::ptr client) {
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

void AccountService::OnTimer() {
    // 每秒定时器（暂时不实现）
}

void AccountService::OnTimer5s() {
    LOG_INFO("执行5秒定时器回调");
    SendHeartbeatToCenterServer();
    
    // 检查数据库服务器连接
    if (!db_server_conn_) {
        LOG_INFO("数据库服务器连接已断开，尝试重新连接");
        if (ConnectToDBServer()) {
            LOG_INFO("成功重新连接到数据库服务器");
        } else {
            LOG_ERROR("重新连接数据库服务器失败");
        }
    }
}

bool AccountService::ConnectToDBServer() {
    LOG_INFO("正在连接数据库服务器: " << db_server_ip_ << ":" << db_server_port_);
    
    // 创建数据库服务器地址
    auto addr = ancfl::IPv4Address::Create(db_server_ip_.c_str(), db_server_port_);
    if (!addr) {
        LOG_ERROR("创建数据库服务器地址失败");
        return false;
    }
    
    // 创建socket并连接
    db_server_conn_ = ancfl::Socket::CreateTCP(addr);
    if (!db_server_conn_) {
        LOG_ERROR("创建数据库服务器socket失败");
        return false;
    }
    
    // 连接数据库服务器
    if (!db_server_conn_->connect(addr)) {
        LOG_ERROR("连接数据库服务器失败: " << db_server_ip_ << ":" << db_server_port_);
        db_server_conn_ = nullptr;
        return false;
    }
    
    // 设置超时（必须在connect之后，因为connect会创建socket文件描述符）
    db_server_conn_->setRecvTimeout(30000);
    db_server_conn_->setSendTimeout(10000);
    
    LOG_INFO("成功连接到数据库服务器: " << db_server_ip_ << ":" << db_server_port_);
    return true;
}

// 向数据库服务器发送请求并获取响应
bool AccountService::SendDBRequest(const DBRequest& req, DBResponse& rsp) {
    if (!db_server_conn_) {
        LOG_ERROR("数据库服务器连接未建立");
        return false;
    }
    
    // 序列化请求
    std::string req_data;
    if (!req.SerializeToString(&req_data)) {
        LOG_ERROR("序列化数据库请求失败");
        return false;
    }
    
    // 构建消息头
    MessageHeader header;
    header.msg_id = static_cast<uint32_t>(MessageID::MSG_DB_REQ);
    header.msg_len = sizeof(header) + req_data.size();
    header.target_id = 0;
    header.user_data = 0;
    
    // 字节序转换
    header.msg_id = ancfl::byteswapOnLittleEndian(header.msg_id);
    header.msg_len = ancfl::byteswapOnLittleEndian(header.msg_len);
    header.target_id = ancfl::byteswapOnLittleEndian(header.target_id);
    header.user_data = ancfl::byteswapOnLittleEndian(header.user_data);
    
    // 发送消息头
    if (db_server_conn_->send(&header, sizeof(header)) <= 0) {
        LOG_ERROR("向数据库服务器发送请求消息头失败");
        return false;
    }
    
    // 发送消息体
    if (db_server_conn_->send(req_data.data(), req_data.size()) <= 0) {
        LOG_ERROR("向数据库服务器发送请求消息体失败");
        return false;
    }
    
    // 接收响应消息头
    MessageHeader resp_header;
    int ret = db_server_conn_->recv(&resp_header, sizeof(resp_header));
    if (ret <= 0) {
        LOG_ERROR("接收数据库服务器响应消息头失败");
        return false;
    }
    
    // 字节序转换
    resp_header.msg_id = ancfl::byteswapOnLittleEndian(resp_header.msg_id);
    resp_header.msg_len = ancfl::byteswapOnLittleEndian(resp_header.msg_len);
    resp_header.target_id = ancfl::byteswapOnLittleEndian(resp_header.target_id);
    resp_header.user_data = ancfl::byteswapOnLittleEndian(resp_header.user_data);
    
    // 检查消息长度是否合法
    if (resp_header.msg_len < sizeof(MessageHeader) || resp_header.msg_len > 1024 * 1024) {
        LOG_ERROR("数据库服务器响应消息长度异常: " << resp_header.msg_len);
        return false;
    }
    
    // 接收响应消息体
    uint32_t body_len = resp_header.msg_len - sizeof(MessageHeader);
    std::vector<char> resp_buffer(body_len);
    ret = db_server_conn_->recv(resp_buffer.data(), body_len);
    if (ret <= 0) {
        LOG_ERROR("接收数据库服务器响应消息体失败");
        return false;
    }
    
    // 解析响应
    if (!rsp.ParseFromArray(resp_buffer.data(), body_len)) {
        LOG_ERROR("解析数据库服务器响应失败");
        return false;
    }
    
    return true;
}

std::string AccountService::MD5Encrypt(const std::string& input) {
    return ancfl::md5(input);
}

bool AccountService::CreateAccount(const std::string& account_name, const std::string& password, int32_t channel, uint64_t& account_id) {
    LOG_INFO("开始创建账号: 账号名=" << account_name << ", 渠道=" << channel);
    
    // 检查账号名是否已存在
    AccountInfo info;
    if (GetAccountInfo(account_name, info)) {
        ANCFL_LOG_WARN(ANCFL_LOG_ROOT())
            << "Account already exists: " << account_name;
        LOG_INFO("账号已存在: 账号名=" << account_name);
        return false;
    }

    // 加密密码
    std::string encrypted_password = MD5Encrypt(password);
    LOG_INFO("密码加密完成");

    // 构建数据库请求
    DBRequest req;
    req.set_op_type(DBOperationType::DB_OP_CREATE_ACCOUNT);
    req.set_account_name(account_name);
    req.set_password(encrypted_password);
    req.set_channel(channel);
    LOG_INFO("构建数据库请求完成");

    // 发送请求并获取响应
    DBResponse rsp;
    if (!SendDBRequest(req, rsp)) {
        LOG_ERROR("向数据库服务器发送创建账号请求失败");
        return false;
    }

    LOG_INFO("接收到数据库服务器响应: ret_code=" << rsp.ret_code() << ", ret_msg=" << rsp.ret_msg() << ", account_id=" << rsp.account_id());

    if (rsp.ret_code() != 0) {
        LOG_ERROR("创建账号失败: " << rsp.ret_msg());
        return false;
    }

    // 获取账号ID
    account_id = rsp.account_id();
    LOG_INFO("获取账号ID: " << account_id);
    
    // 检查账号ID是否有效
    if (account_id == 0) {
        LOG_ERROR("无效的账号ID: " << account_id);
        return false;
    }

    // 更新缓存
    {
        ancfl::Mutex::Lock lock(cache_mutex_);
        AccountInfo info;
        info.account_id = account_id;
        info.account_name = account_name;
        info.password = encrypted_password;
        info.channel = channel;
        info.create_time = rsp.create_time();
        info.last_login_time = rsp.last_login_time();
        info.is_sealed = rsp.is_sealed();
        info.seal_end_time = rsp.seal_end_time();
        info.review = rsp.review();

        account_cache_[account_id] = info;
        name_to_id_[account_name] = account_id;
        LOG_INFO("更新缓存完成: account_id=" << account_id << ", 缓存大小=" << account_cache_.size());
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT())
        << "Account created: " << account_name << " id=" << account_id;
    LOG_INFO("创建账号完成: 账号名=" << account_name << ", account_id=" << account_id);
    return true;
}

bool AccountService::VerifyAccount(const std::string& account_name, const std::string& password, AccountInfo& info) {
    // 先从缓存查找
    {
        ancfl::Mutex::Lock lock(cache_mutex_);
        auto it = name_to_id_.find(account_name);
        if (it != name_to_id_.end()) {
            auto acc_it = account_cache_.find(it->second);
            if (acc_it != account_cache_.end()) {
                std::string encrypted_password = MD5Encrypt(password);
                if (acc_it->second.password == encrypted_password) {
                    info = acc_it->second;
                    return true;
                }
            }
        }
    }

    // 加密密码
    std::string encrypted_password = MD5Encrypt(password);

    // 构建数据库请求
    DBRequest req;
    req.set_op_type(DBOperationType::DB_OP_VERIFY_ACCOUNT);
    req.set_account_name(account_name);
    req.set_password(encrypted_password);

    // 发送请求并获取响应
    DBResponse rsp;
    if (!SendDBRequest(req, rsp)) {
        LOG_ERROR("向数据库服务器发送验证账号请求失败");
        return false;
    }

    if (rsp.ret_code() != 0) {
        LOG_ERROR("验证账号失败: " << rsp.ret_msg());
        return false;
    }

    // 填充账号信息
    info.account_id = rsp.account_id();
    info.account_name = rsp.account_name();
    info.password = encrypted_password; // 缓存加密后的密码
    info.channel = rsp.channel();
    info.create_time = rsp.create_time();
    info.last_login_time = rsp.last_login_time();
    info.last_login_ip = rsp.last_login_ip();
    info.is_sealed = rsp.is_sealed();
    info.seal_end_time = rsp.seal_end_time();
    info.review = rsp.review();

    // 更新缓存
    {
        ancfl::Mutex::Lock lock(cache_mutex_);
        account_cache_[info.account_id] = info;
        name_to_id_[account_name] = info.account_id;
    }

    return true;
}

bool AccountService::GetAccountInfo(uint64_t account_id, AccountInfo& info) {
    // 先从缓存查找
    {
        ancfl::Mutex::Lock lock(cache_mutex_);
        auto it = account_cache_.find(account_id);
        if (it != account_cache_.end()) {
            info = it->second;
            return true;
        }
    }

    // 构建数据库请求
    DBRequest req;
    req.set_op_type(DBOperationType::DB_OP_GET_ACCOUNT_INFO);
    req.set_account_id(account_id);

    // 发送请求并获取响应
    DBResponse rsp;
    if (!SendDBRequest(req, rsp)) {
        LOG_ERROR("向数据库服务器发送获取账号信息请求失败");
        return false;
    }

    if (rsp.ret_code() != 0) {
        LOG_ERROR("获取账号信息失败: " << rsp.ret_msg());
        return false;
    }

    // 填充账号信息
    info.account_id = rsp.account_id();
    info.account_name = rsp.account_name();
    info.password = "";
    info.channel = rsp.channel();
    info.create_time = rsp.create_time();
    info.last_login_time = rsp.last_login_time();
    info.last_login_ip = rsp.last_login_ip();
    info.is_sealed = rsp.is_sealed();
    info.seal_end_time = rsp.seal_end_time();
    info.review = rsp.review();

    // 更新缓存
    {
        ancfl::Mutex::Lock lock(cache_mutex_);
        account_cache_[info.account_id] = info;
        name_to_id_[info.account_name] = info.account_id;
    }

    return true;
}

bool AccountService::GetAccountInfo(const std::string& account_name, AccountInfo& info) {
    // 先从缓存查找
    {
        ancfl::Mutex::Lock lock(cache_mutex_);
        auto it = name_to_id_.find(account_name);
        if (it != name_to_id_.end()) {
            auto acc_it = account_cache_.find(it->second);
            if (acc_it != account_cache_.end()) {
                info = acc_it->second;
                return true;
            }
        }
    }

    // 构建数据库请求（使用验证账号的方式获取账号信息）
    DBRequest req;
    req.set_op_type(DBOperationType::DB_OP_VERIFY_ACCOUNT);
    req.set_account_name(account_name);
    req.set_password("invalid_password"); // 使用无效密码，只获取账号信息，不验证密码

    // 发送请求并获取响应
    DBResponse rsp;
    if (!SendDBRequest(req, rsp)) {
        LOG_ERROR("向数据库服务器发送获取账号信息请求失败");
        return false;
    }

    // 即使验证失败，只要返回了账号信息，就认为成功
    if (rsp.account_id() > 0) {
        // 填充账号信息
        info.account_id = rsp.account_id();
        info.account_name = rsp.account_name();
        info.password = "";
        info.channel = rsp.channel();
        info.create_time = rsp.create_time();
        info.last_login_time = rsp.last_login_time();
        info.last_login_ip = rsp.last_login_ip();
        info.is_sealed = rsp.is_sealed();
        info.seal_end_time = rsp.seal_end_time();
        info.review = rsp.review();

        // 更新缓存
        {
            ancfl::Mutex::Lock lock(cache_mutex_);
            account_cache_[info.account_id] = info;
            name_to_id_[info.account_name] = info.account_id;
        }

        return true;
    }

    return false;
}

bool AccountService::SealAccount(uint64_t account_id, int32_t seal_time) {
    // 构建数据库请求
    DBRequest req;
    req.set_op_type(DBOperationType::DB_OP_SEAL_ACCOUNT);
    req.set_account_id(account_id);
    req.set_seal_time(seal_time);

    // 发送请求并获取响应
    DBResponse rsp;
    if (!SendDBRequest(req, rsp)) {
        LOG_ERROR("向数据库服务器发送封号请求失败");
        return false;
    }

    if (rsp.ret_code() != 0) {
        LOG_ERROR("封号失败: " << rsp.ret_msg());
        return false;
    }

    // 更新缓存
    {
        ancfl::Mutex::Lock lock(cache_mutex_);
        auto it = account_cache_.find(account_id);
        if (it != account_cache_.end()) {
            it->second.is_sealed = true;
            it->second.seal_end_time = time(nullptr) + seal_time;
        }
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Account sealed: " << account_id
                                     << " for " << seal_time << " seconds";
    return true;
}

bool AccountService::UnsealAccount(uint64_t account_id) {
    // 构建数据库请求
    DBRequest req;
    req.set_op_type(DBOperationType::DB_OP_UNSEAL_ACCOUNT);
    req.set_account_id(account_id);

    // 发送请求并获取响应
    DBResponse rsp;
    if (!SendDBRequest(req, rsp)) {
        LOG_ERROR("向数据库服务器发送解封请求失败");
        return false;
    }

    if (rsp.ret_code() != 0) {
        LOG_ERROR("解封失败: " << rsp.ret_msg());
        return false;
    }

    // 更新缓存
    {
        ancfl::Mutex::Lock lock(cache_mutex_);
        auto it = account_cache_.find(account_id);
        if (it != account_cache_.end()) {
            it->second.is_sealed = false;
            it->second.seal_end_time = 0;
        }
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Account unsealed: " << account_id;
    return true;
}

bool AccountService::IsAccountSealed(uint64_t account_id) {
    AccountInfo info;
    if (!GetAccountInfo(account_id, info)) {
        return false;
    }

    if (!info.is_sealed) {
        return false;
    }

    // 检查封号是否过期
    if (info.seal_end_time > 0 && time(nullptr) > info.seal_end_time) {
        UnsealAccount(account_id);
        return false;
    }

    return true;
}

bool AccountService::RecordLoginLog(uint64_t account_id, int32_t channel, const std::string& version, const std::string& uuid, const std::string& idfa, const std::string& imodel, const std::string& imei, int32_t ip) {
    // 检查账号ID是否有效
    if (account_id == 0) {
        LOG_ERROR("无效的账号ID: " << account_id);
        return false;
    }
    
    // 构建数据库请求
    DBRequest req;
    req.set_op_type(DBOperationType::DB_OP_RECORD_LOGIN_LOG);
    req.set_account_id(account_id);
    req.set_channel(channel);
    req.set_version(version);
    req.set_uuid(uuid);
    req.set_idfa(idfa);
    req.set_imodel(imodel);
    req.set_imei(imei);
    req.set_ip(ip);

    // 发送请求并获取响应
    DBResponse rsp;
    if (!SendDBRequest(req, rsp)) {
        LOG_ERROR("向数据库服务器发送记录登录日志请求失败");
        return false;
    }

    if (rsp.ret_code() != 0) {
        LOG_ERROR("记录登录日志失败: " << rsp.ret_msg());
        return false;
    }

    return true;
}

// ==================== 消息处理器 ====================

bool AccountService::OnAccountRegReq(const NetPacket& packet) {
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "AccountService::OnAccountRegReq";
    // 实现账号注册请求处理
    // 实际应该解析请求消息，创建账号，返回注册结果
    return true;
}

bool AccountService::OnAccountLoginReq(const NetPacket& packet) {
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "AccountService::OnAccountLoginReq";
    // 实现账号登录请求处理
    // 实际应该解析请求消息，验证账号密码，记录登录日志，返回登录结果
    return true;
}

bool AccountService::OnSealAccountReq(const NetPacket& packet) {
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "AccountService::OnSealAccountReq";
    // 实现封号请求处理
    // 实际应该解析请求消息，执行封号操作，返回封号结果
    return true;
}

bool AccountService::OnHeartBeatReq(const NetPacket& packet) {
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "AccountService::OnHeartBeatReq";
    // 实现心跳响应
    // 实际应该返回心跳响应消息
    return true;
}

// ==================== 中心服务器连接管理 ====================

bool AccountService::ConnectToCenterServer() {
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
    
    // 设置超时
    center_server_conn_->setRecvTimeout(30000);
    center_server_conn_->setSendTimeout(10000);
    
    // 连接中心服务器
    if (!center_server_conn_->connect(addr)) {
        LOG_ERROR("连接中心服务器失败: " << center_server_ip_ << ":" << center_server_port_);
        center_server_conn_ = nullptr;
        return false;
    }
    
    LOG_INFO("成功连接到中心服务器: " << center_server_ip_ << ":" << center_server_port_);
    return true;
}

bool AccountService::RegisterToCenterServer() {
    if (!center_server_conn_) {
        LOG_ERROR("中心服务器连接未建立");
        return false;
    }
    
    // 构建注册消息
    uint32_t msg_id = 1001; // 假设1001是注册消息ID
    std::string data = "account_server_register";
    
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

void AccountService::SendHeartbeatToCenterServer() {
    if (!center_server_conn_) {
        LOG_ERROR("中心服务器连接未建立");
        return;
    }
    
    // 构建心跳消息
    uint32_t msg_id = 1002; // 假设1002是心跳消息ID
    std::string data = "account_server_heartbeat";
    
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

void AccountService::HandleLoginRequest(ancfl::Socket::ptr client, const std::string& data) {
    LOG_INFO("接收到登录请求");
    
    // 解析登录请求
    AccountLoginReq req;
    if (!req.ParseFromString(data)) {
        LOG_ERROR("解析登录请求失败");
        return;
    }
    
    LOG_INFO("登录请求信息: 账号=" << req.account_name() << ", 密码=" << req.password());
    
    // 构建登录响应
    AccountLoginAck rsp;
    
    // 验证账号密码
    AccountInfo info;
    if (!VerifyAccount(req.account_name(), req.password(), info)) {
        LOG_ERROR("账号或密码错误: " << req.account_name());
        rsp.set_ret_code(1); // 1表示账号密码错误
        rsp.set_account_id(0);
    } else {
        LOG_INFO("验证账号成功: account_id=" << info.account_id << ", 渠道=" << info.channel);
        
        // 检查账号ID是否有效
        if (info.account_id == 0) {
            LOG_ERROR("无效的账号ID: " << info.account_id);
            rsp.set_ret_code(3); // 3表示账号ID无效
            rsp.set_account_id(0);
        } else {
            // 检查账号是否被封号
            if (IsAccountSealed(info.account_id)) {
                LOG_ERROR("账号已被封号: " << req.account_name());
                rsp.set_ret_code(2); // 2表示账号被封号
                rsp.set_account_id(info.account_id);
            } else {
                // 最后登录时间的更新由数据库服务器在验证账号时处理
                
                // 记录登录日志
                std::string version = "";
                std::string uuid = "";
                std::string idfa = "";
                std::string imodel = "";
                std::string imei = "";
                
                if (req.has_login_log()) {
                    const AccountLog& log = req.login_log();
                    version = log.version();
                    uuid = log.uuid();
                    idfa = log.idfa();
                    imodel = log.imodel();
                    imei = log.imei();
                    LOG_INFO("登录日志信息: version=" << version << ", uuid=" << uuid);
                }
                
                LOG_INFO("准备记录登录日志: account_id=" << info.account_id << ", 渠道=" << info.channel);
                bool log_result = RecordLoginLog(info.account_id, info.channel, version, uuid, 
                              idfa, imodel, imei, 0);
                LOG_INFO("记录登录日志结果: " << (log_result ? "成功" : "失败"));
                
                LOG_INFO("登录成功: 账号=" << req.account_name() << ", account_id=" << info.account_id);
                rsp.set_ret_code(0); // 0表示成功
                rsp.set_account_id(info.account_id);
            }
        }
    }
    
    // 序列化响应
    std::string rsp_data;
    if (!rsp.SerializeToString(&rsp_data)) {
        LOG_ERROR("序列化登录响应失败");
        return;
    }
    
    // 构建消息头
    MessageHeader header;
    header.msg_id = static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_ACK);
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
    
    // 发送响应到登录服务器
    int n = client->send(msg_data.data(), msg_data.size(), 0);
    if (n <= 0) {
        LOG_ERROR("发送登录响应失败");
        return;
    }
    
    LOG_INFO("登录请求处理完成，已发送响应");
}

void AccountService::HandleRegisterRequest(ancfl::Socket::ptr client, const std::string& data) {
    LOG_INFO("接收到注册请求");
    
    // 解析注册请求
    AccountRegReq req;
    if (!req.ParseFromString(data)) {
        LOG_ERROR("解析注册请求失败");
        return;
    }
    
    LOG_INFO("注册请求信息: 账号=" << req.account_name() << ", 密码=" << req.password());
    
    // 构建注册响应
    AccountRegAck rsp;
    
    // 创建账号
    uint64_t account_id = 0;
    if (!CreateAccount(req.account_name(), req.password(), req.channel(), account_id)) {
        LOG_ERROR("创建账号失败: " << req.account_name());
        rsp.set_ret_code(1); // 1表示创建失败
        rsp.set_account_id(0);
    } else {
        LOG_INFO("注册成功: 账号=" << req.account_name() << ", account_id=" << account_id);
        rsp.set_ret_code(0); // 0表示成功
        rsp.set_account_id(account_id);
    }
    
    // 序列化响应
    std::string rsp_data;
    if (!rsp.SerializeToString(&rsp_data)) {
        LOG_ERROR("序列化注册响应失败");
        return;
    }
    
    // 构建消息头
    MessageHeader header;
    header.msg_id = static_cast<uint32_t>(MessageID::MSG_ACCOUNT_REG_ACK);
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
    
    // 发送响应到登录服务器
    int n = client->send(msg_data.data(), msg_data.size(), 0);
    if (n <= 0) {
        LOG_ERROR("发送注册响应失败");
        return;
    }
    
    LOG_INFO("注册请求处理完成，已发送响应");
}

} // namespace game_server
