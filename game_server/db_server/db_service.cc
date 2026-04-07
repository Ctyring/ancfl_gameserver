#include "db_service.h"
#include "ancfl/log.h"
#include "ancfl/config.h"
#include "proto/msg_id.pb.h"
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>

namespace game_server {

// 日志宏，添加时间戳和统一前缀
#define LOG_INFO(msg) do { \
    time_t now = time(nullptr); \
    struct tm* tm_info = localtime(&now); \
    char time_str[20]; \
    strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", tm_info); \
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "[" << time_str << "] [数据库服务器] " << msg; \
} while(0)

#define LOG_ERROR(msg) do { \
    time_t now = time(nullptr); \
    struct tm* tm_info = localtime(&now); \
    char time_str[20]; \
    strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", tm_info); \
    ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "[" << time_str << "] [数据库服务器] 错误: " << msg; \
} while(0)

// 简单的 SHA1 哈希函数
std::string Sha1Hash(const std::string& input) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), hash);
    
    std::stringstream ss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

DBService::DBService()
    : GameServiceBase("db_server"),
      next_conn_id_(1),
      max_connections_(10000), // 默认最大连接数10000
      center_server_id_(1),
      center_server_ip_("127.0.0.1"),
      center_server_port_(8007),
      center_server_conn_(nullptr),
      db_port_(3306) {}

DBService::~DBService() {
    UninitService();
}

bool DBService::InitService() {
    // 读取配置文件
    std::string config_path = "bin/conf/servers.yml";
    
    // 配置变量
    std::string host = "0.0.0.0";
    int port = 8009;
    size_t max_connections = 10000;
    int recv_timeout = 30000;
    int send_timeout = 10000;
    int center_port = 8007;
    
    // 数据库配置
    std::string db_host = "127.0.0.1";
    int db_port = 3306;
    std::string db_user = "root";
    std::string db_password = "12345678";
    std::string db_name = "game_server";
    
    // 加载配置文件
    try {
        YAML::Node root = YAML::LoadFile(config_path);
        // 直接从YAML节点读取配置，避免与其他配置文件冲突
        
        // 读取数据库服务器配置
        if (root["db_server"]) {
            if (root["db_server"]["host"]) {
                host = root["db_server"]["host"].as<std::string>();
            }
            if (root["db_server"]["port"]) {
                port = root["db_server"]["port"].as<int>();
            }
            if (root["db_server"]["max_connections"]) {
                max_connections = root["db_server"]["max_connections"].as<size_t>();
            }
            if (root["db_server"]["recv_timeout"]) {
                recv_timeout = root["db_server"]["recv_timeout"].as<int>();
            }
            if (root["db_server"]["send_timeout"]) {
                send_timeout = root["db_server"]["send_timeout"].as<int>();
            }
        }
        
        // 读取中心服务器配置
        std::string center_host = "127.0.0.1";
        if (root["center_server"]) {
            if (root["center_server"]["host"]) {
                center_host = root["center_server"]["host"].as<std::string>();
            }
            if (root["center_server"]["port"]) {
                center_port = root["center_server"]["port"].as<int>();
            }
        }
        
        // 读取数据库配置
        if (root["database"]) {
            if (root["database"]["host"]) {
                db_host = root["database"]["host"].as<std::string>();
            }
            if (root["database"]["port"]) {
                db_port = root["database"]["port"].as<int>();
            }
            if (root["database"]["user"]) {
                db_user = root["database"]["user"].as<std::string>();
            }
            if (root["database"]["password"]) {
                db_password = root["database"]["password"].as<std::string>();
            }
            if (root["database"]["dbname"]) {
                db_name = root["database"]["dbname"].as<std::string>();
            }
        }
        
        max_connections_ = max_connections;
        center_server_ip_ = center_host;
        center_server_port_ = center_port;
        
        // 设置数据库配置
        db_host_ = db_host;
        db_port_ = db_port;
        db_user_ = db_user;
        db_password_ = db_password;
        db_name_ = db_name;
        
        LOG_INFO("正在初始化网络，绑定地址: " << host << ":" << port);
        LOG_INFO("最大连接数: " << max_connections_);
        LOG_INFO("接收超时: " << recv_timeout << "ms");
        LOG_INFO("发送超时: " << send_timeout << "ms");
        LOG_INFO("中心服务器地址: " << center_host << ":" << center_port);
        LOG_INFO("数据库地址: " << db_host << ":" << db_port);
    } catch (std::exception& e) {
        LOG_ERROR("加载配置文件失败: " << config_path << "，错误: " << e.what());
        return false;
    }
    
    // 连接数据库
    if (!ConnectToDatabase()) {
        LOG_ERROR("连接数据库失败");
        return false;
    }
    
    auto addr = ancfl::IPv4Address::Create(host.c_str(), port);
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
    
    LOG_INFO("数据库服务器启动成功，正在监听: " << *addr);
    
    RegisterAllHandlers();
    
    // 连接中心服务器
    if (!ConnectToCenterServer()) {
        LOG_ERROR("连接中心服务器失败");
        // 继续运行，不退出
    } else {
        // 向中心服务器注册
        if (!RegisterToCenterServer()) {
            LOG_ERROR("向中心服务器注册失败");
            // 继续运行，不退出
        }
    }
    
    LOG_INFO("数据库服务器初始化成功");
    return true;
}

void DBService::UninitService() {
    { 
        ancfl::Mutex::Lock lock(conn_mutex_);
        for (auto& it : connections_) {
            if (it.second) {
                it.second->close();
            }
        }
        connections_.clear();
        last_heart_time_.clear();
    }
    
    std::lock_guard<std::mutex> lock(db_mutex_);
    db_connections_.clear();
    
    LOG_INFO("数据库服务器已反初始化");
}

void DBService::RegisterAllHandlers() {
    LOG_INFO("所有消息处理器已注册");
}

void DBService::handleClient(ancfl::Socket::ptr client) {
    // 检查连接数是否超过最大值
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
    
    int32_t conn_id;
    {
        ancfl::Mutex::Lock lock(conn_mutex_);
        conn_id = next_conn_id_++;
        connections_[conn_id] = client;
        last_heart_time_[conn_id] = time(nullptr);
    }
    
    LOG_INFO("客户端已连接，连接ID=" << conn_id << ", 当前连接数=" << connections_.size() << "/" << max_connections_);
    
    if (m_worker) {
        m_worker->schedule(std::bind(&DBService::HandleRecv, this, client, conn_id));
    } else {
        LOG_ERROR("工作线程为空，无法调度HandleRecv");
    }
}

void DBService::HandleRecv(ancfl::Socket::ptr client, int32_t conn_id) {
    LOG_INFO("开始处理接收消息，连接ID=" << conn_id);
    
    std::vector<char> buffer(8192);
    
    while (true) {
        MessageHeader header;
        int ret = client->recv(&header, sizeof(header));
        if (ret <= 0) {
            LOG_INFO("连接已关闭或出错，连接ID=" << conn_id << ", 返回值=" << ret);
            break;
        }
        
        header.msg_id = ancfl::byteswapOnLittleEndian(header.msg_id);
        header.msg_len = ancfl::byteswapOnLittleEndian(header.msg_len);
        header.target_id = ancfl::byteswapOnLittleEndian(header.target_id);
        header.user_data = ancfl::byteswapOnLittleEndian(header.user_data);
        
        LOG_INFO("收到消息头，消息ID=" << header.msg_id << ", 消息长度=" << header.msg_len);
        
        if (header.msg_len > 32768 || header.msg_len < sizeof(header)) {
            LOG_ERROR("无效的消息长度: " << header.msg_len);
            break;
        }
        
        uint32_t body_len = header.msg_len - sizeof(header);
        if (body_len > 0) {
            if (buffer.size() < body_len) {
                buffer.resize(body_len);
            }
            
            ret = client->recv(buffer.data(), body_len);
            if (ret <= 0) {
                LOG_INFO("接收消息体失败，连接ID=" << conn_id << ", 返回值=" << ret);
                break;
            }
            
            // 处理数据库请求消息
            DBRequest req;
            if (req.ParseFromArray(buffer.data(), body_len)) {
                LOG_INFO("解析数据库请求成功，操作类型=" << req.op_type());
                
                // 处理不同类型的数据库操作
                DBResponse rsp;
                switch (req.op_type()) {
                    case DBOperationType::DB_OP_CREATE_ACCOUNT:
                        HandleCreateAccount(req, rsp);
                        break;
                    case DBOperationType::DB_OP_VERIFY_ACCOUNT:
                        HandleVerifyAccount(req, rsp);
                        break;
                    case DBOperationType::DB_OP_GET_ACCOUNT_INFO:
                        HandleGetAccountInfo(req, rsp);
                        break;
                    case DBOperationType::DB_OP_SEAL_ACCOUNT:
                        HandleSealAccount(req, rsp);
                        break;
                    case DBOperationType::DB_OP_UNSEAL_ACCOUNT:
                        HandleUnsealAccount(req, rsp);
                        break;
                    case DBOperationType::DB_OP_RECORD_LOGIN_LOG:
                        HandleRecordLoginLog(req, rsp);
                        break;
                    case DBOperationType::DB_OP_CREATE_ROLE:
                        HandleCreateRole(req, rsp);
                        break;
                    case DBOperationType::DB_OP_UPDATE_ROLE:
                        HandleUpdateRole(req, rsp);
                        break;
                    case DBOperationType::DB_OP_DELETE_ROLE:
                        HandleDeleteRole(req, rsp);
                        break;
                    case DBOperationType::DB_OP_GET_ROLE_LIST:
                        HandleGetRoleList(req, rsp);
                        break;
                    case DBOperationType::DB_OP_GET_ROLE_DATA:
                        HandleGetRoleData(req, rsp);
                        break;
                    default:
                        LOG_ERROR("未知的数据库操作类型: " << req.op_type());
                        rsp.set_ret_code(1);
                        rsp.set_ret_msg("未知的操作类型");
                        break;
                }
                
                // 序列化响应
                std::string rsp_data;
                if (rsp.SerializeToString(&rsp_data)) {
                    // 构建响应消息头
                    MessageHeader resp_header;
                    resp_header.msg_id = static_cast<uint32_t>(MessageID::MSG_DB_RESP);
                    resp_header.msg_len = sizeof(resp_header) + rsp_data.size();
                    resp_header.target_id = 0;
                    resp_header.user_data = 0;

                    // 字节序转换
                    resp_header.msg_id = ancfl::byteswapOnLittleEndian(resp_header.msg_id);
                    resp_header.msg_len = ancfl::byteswapOnLittleEndian(resp_header.msg_len);
                    resp_header.target_id = ancfl::byteswapOnLittleEndian(resp_header.target_id);
                    resp_header.user_data = ancfl::byteswapOnLittleEndian(resp_header.user_data);

                    // 发送响应
                    client->send(&resp_header, sizeof(resp_header));
                    client->send(rsp_data.data(), rsp_data.size());
                    
                    LOG_INFO("数据库操作响应发送成功，返回码=" << rsp.ret_code());
                } else {
                    LOG_ERROR("序列化数据库响应失败");
                }
            } else {
                LOG_ERROR("解析数据库请求失败");
            }
        }
    }
    
    {
        ancfl::Mutex::Lock lock(conn_mutex_);
        connections_.erase(conn_id);
        last_heart_time_.erase(conn_id);
    }
    
    LOG_INFO("连接已移除，连接ID=" << conn_id << ", 剩余连接数=" << connections_.size() << "/" << max_connections_);
}

bool DBService::ConnectToCenterServer() {
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

bool DBService::RegisterToCenterServer() {
    if (!center_server_conn_) {
        LOG_ERROR("中心服务器连接未建立");
        return false;
    }
    
    // 构建注册消息
    uint32_t msg_id = 1001; // 假设1001是注册消息ID
    std::string data = "db_server_register";
    
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

void DBService::SendHeartbeatToCenterServer() {
    if (!center_server_conn_) {
        LOG_ERROR("中心服务器连接未建立");
        return;
    }
    
    // 构建心跳消息
    uint32_t msg_id = 1002; // 假设1002是心跳消息ID
    std::string data = "db_server_heartbeat";
    
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

void DBService::OnTimer5s() {
    LOG_INFO("执行5秒定时器回调");
    SendHeartbeatToCenterServer();
}

size_t DBService::GetConnectionCount() const {
    ancfl::Mutex::Lock lock(const_cast<ancfl::Mutex&>(conn_mutex_));
    return connections_.size();
}

ancfl::Socket::ptr DBService::GetClientByConnId(uint32_t conn_id) {
    ancfl::Mutex::Lock lock(conn_mutex_);
    auto it = connections_.find(conn_id);
    if (it != connections_.end()) {
        return it->second;
    }
    return nullptr;
}

void DBService::SetMaxConnections(size_t max_connections) {
    max_connections_ = max_connections;
    LOG_INFO("最大连接数已设置为: " << max_connections_);
}

// 处理创建账号请求
void DBService::HandleCreateAccount(const DBRequest& req, DBResponse& rsp) {
    LOG_INFO("处理创建账号请求，账号名=" << req.account_name());
    
    uint64_t account_id = 0;
    bool success = CreateAccount(req.account_name(), req.password(), req.channel(), account_id);
    
    LOG_INFO("创建账号结果: success=" << success << ", account_id=" << account_id);
    
    if (success) {
        rsp.set_ret_code(0);
        rsp.set_ret_msg("创建账号成功");
        rsp.set_account_id(account_id);
        rsp.set_account_name(req.account_name());
        rsp.set_channel(req.channel());
        LOG_INFO("设置响应: account_id=" << account_id);
    } else {
        rsp.set_ret_code(1);
        rsp.set_ret_msg("创建账号失败");
        LOG_INFO("创建账号失败，account_id=" << account_id);
    }
}

// 处理验证账号请求
void DBService::HandleVerifyAccount(const DBRequest& req, DBResponse& rsp) {
    LOG_INFO("处理验证账号请求，账号名=" << req.account_name());
    
    uint64_t account_id = 0;
    bool success = VerifyAccount(req.account_name(), req.password(), account_id);
    
    if (success) {
        rsp.set_ret_code(0);
        rsp.set_ret_msg("验证账号成功");
        rsp.set_account_id(account_id);
        rsp.set_account_name(req.account_name());
        
        // 获取账号信息
        AccountInfo info;
        if (GetAccountInfo(account_id, info)) {
            rsp.set_channel(info.channel);
            rsp.set_create_time(info.create_time);
            rsp.set_last_login_time(info.last_login_time);
            rsp.set_last_login_ip(info.last_login_ip);
            rsp.set_is_sealed(info.is_sealed);
            rsp.set_seal_end_time(info.seal_end_time);
            rsp.set_review(info.review);
        }
    } else {
        rsp.set_ret_code(1);
        rsp.set_ret_msg("验证账号失败");
    }
}

// 处理获取账号信息请求
void DBService::HandleGetAccountInfo(const DBRequest& req, DBResponse& rsp) {
    LOG_INFO("处理获取账号信息请求，账号ID=" << req.account_id());
    
    AccountInfo info;
    bool success = GetAccountInfo(req.account_id(), info);
    
    if (success) {
        rsp.set_ret_code(0);
        rsp.set_ret_msg("获取账号信息成功");
        rsp.set_account_id(info.account_id);
        rsp.set_account_name(info.account_name);
        rsp.set_channel(info.channel);
        rsp.set_create_time(info.create_time);
        rsp.set_last_login_time(info.last_login_time);
        rsp.set_last_login_ip(info.last_login_ip);
        rsp.set_is_sealed(info.is_sealed);
        rsp.set_seal_end_time(info.seal_end_time);
        rsp.set_review(info.review);
    } else {
        rsp.set_ret_code(1);
        rsp.set_ret_msg("获取账号信息失败");
    }
}

// 处理封号请求
void DBService::HandleSealAccount(const DBRequest& req, DBResponse& rsp) {
    LOG_INFO("处理封号请求，账号ID=" << req.account_id() << "，封号时间=" << req.seal_time());
    
    int64_t seal_end_time = time(nullptr) + req.seal_time();
    bool success = SealAccount(req.account_id(), seal_end_time);
    
    if (success) {
        rsp.set_ret_code(0);
        rsp.set_ret_msg("封号成功");
    } else {
        rsp.set_ret_code(1);
        rsp.set_ret_msg("封号失败");
    }
}

// 处理解封请求
void DBService::HandleUnsealAccount(const DBRequest& req, DBResponse& rsp) {
    LOG_INFO("处理解封请求，账号ID=" << req.account_id());
    
    bool success = UnsealAccount(req.account_id());
    
    if (success) {
        rsp.set_ret_code(0);
        rsp.set_ret_msg("解封成功");
    } else {
        rsp.set_ret_code(1);
        rsp.set_ret_msg("解封失败");
    }
}

// 处理记录登录日志请求
void DBService::HandleRecordLoginLog(const DBRequest& req, DBResponse& rsp) {
    LOG_INFO("处理记录登录日志请求，账号ID=" << req.account_id() << ", 渠道=" << req.channel() << ", IP=" << req.ip());
    LOG_INFO("版本=" << req.version() << ", UUID=" << req.uuid() << ", IDFA=" << req.idfa());
    LOG_INFO("设备型号=" << req.imodel() << ", IMEI=" << req.imei());
    
    // 检查账号ID是否有效
    if (req.account_id() == 0) {
        LOG_ERROR("无效的账号ID: " << req.account_id());
        rsp.set_ret_code(1);
        rsp.set_ret_msg("无效的账号ID");
        return;
    }
    
    bool success = RecordLoginLog(
        req.account_id(),
        req.channel(),
        req.version(),
        req.uuid(),
        req.idfa(),
        req.imodel(),
        req.imei(),
        req.ip()
    );
    
    if (success) {
        LOG_INFO("记录登录日志成功，账号ID=" << req.account_id());
        rsp.set_ret_code(0);
        rsp.set_ret_msg("记录登录日志成功");
    } else {
        LOG_ERROR("记录登录日志失败，账号ID=" << req.account_id());
        rsp.set_ret_code(1);
        rsp.set_ret_msg("记录登录日志失败");
    }
}

// 处理创建角色请求
void DBService::HandleCreateRole(const DBRequest& req, DBResponse& rsp) {
    LOG_INFO("处理创建角色请求，账号ID=" << req.account_id() << "，角色名=" << req.role_name());
    
    RoleInfo role_info;
    role_info.set_role_id(req.role_id());
    role_info.set_account_id(req.account_id());
    role_info.set_server_id(req.server_id());
    role_info.set_role_name(req.role_name());
    role_info.set_career(req.career());
    role_info.set_level(req.level());
    role_info.set_exp(req.exp());
    role_info.set_head_id(req.head_id());
    role_info.set_portrait_frame(req.portrait_frame());
    role_info.set_create_time(time(nullptr));
    role_info.set_last_login_time(time(nullptr));
    role_info.set_is_deleted(0);
    role_info.set_delete_time(0);
    
    bool success = CreateRole(role_info);
    
    if (success) {
        rsp.set_ret_code(0);
        rsp.set_ret_msg("创建角色成功");
        
        // 填充角色信息
        auto role_data = rsp.mutable_role_info();
        role_data->set_role_id(role_info.role_id());
        role_data->set_account_id(role_info.account_id());
        role_data->set_server_id(role_info.server_id());
        role_data->set_role_name(role_info.role_name());
        role_data->set_career(role_info.career());
        role_data->set_level(role_info.level());
        role_data->set_exp(role_info.exp());
        role_data->set_head_id(role_info.head_id());
        role_data->set_portrait_frame(role_info.portrait_frame());
        role_data->set_create_time(role_info.create_time());
        role_data->set_last_login_time(role_info.last_login_time());
        role_data->set_is_deleted(role_info.is_deleted());
        role_data->set_delete_time(role_info.delete_time());
    } else {
        rsp.set_ret_code(1);
        rsp.set_ret_msg("创建角色失败");
    }
}

// 处理更新角色请求
void DBService::HandleUpdateRole(const DBRequest& req, DBResponse& rsp) {
    LOG_INFO("处理更新角色请求，角色ID=" << req.role_id());
    
    RoleInfo role_info;
    role_info.set_role_id(req.role_id());
    role_info.set_account_id(req.account_id());
    role_info.set_server_id(req.server_id());
    role_info.set_role_name(req.role_name());
    role_info.set_career(req.career());
    role_info.set_level(req.level());
    role_info.set_exp(req.exp());
    role_info.set_head_id(req.head_id());
    role_info.set_portrait_frame(req.portrait_frame());
    role_info.set_create_time(0); // 不需要更新创建时间
    role_info.set_last_login_time(time(nullptr));
    role_info.set_is_deleted(req.is_deleted());
    role_info.set_delete_time(req.delete_time());
    
    bool success = UpdateRole(role_info);
    
    if (success) {
        rsp.set_ret_code(0);
        rsp.set_ret_msg("更新角色成功");
    } else {
        rsp.set_ret_code(1);
        rsp.set_ret_msg("更新角色失败");
    }
}

// 处理删除角色请求
void DBService::HandleDeleteRole(const DBRequest& req, DBResponse& rsp) {
    LOG_INFO("处理删除角色请求，角色ID=" << req.role_id());
    
    bool success = DeleteRole(req.role_id());
    
    if (success) {
        rsp.set_ret_code(0);
        rsp.set_ret_msg("删除角色成功");
    } else {
        rsp.set_ret_code(1);
        rsp.set_ret_msg("删除角色失败");
    }
}

// 处理获取角色列表请求
void DBService::HandleGetRoleList(const DBRequest& req, DBResponse& rsp) {
    LOG_INFO("处理获取角色列表请求，账号ID=" << req.account_id());
    
    std::vector<RoleInfo> roles;
    bool success = GetRoleList(req.account_id(), roles);
    
    if (success) {
        rsp.set_ret_code(0);
        rsp.set_ret_msg("获取角色列表成功");
        
        // 填充角色列表
        for (const auto& role : roles) {
            auto role_data = rsp.add_role_list();
            role_data->set_role_id(role.role_id());
            role_data->set_account_id(role.account_id());
            role_data->set_server_id(role.server_id());
            role_data->set_role_name(role.role_name());
            role_data->set_career(role.career());
            role_data->set_level(role.level());
            role_data->set_exp(role.exp());
            role_data->set_head_id(role.head_id());
            role_data->set_portrait_frame(role.portrait_frame());
            role_data->set_create_time(role.create_time());
            role_data->set_last_login_time(role.last_login_time());
            role_data->set_is_deleted(role.is_deleted());
            role_data->set_delete_time(role.delete_time());
        }
    } else {
        rsp.set_ret_code(1);
        rsp.set_ret_msg("获取角色列表失败");
    }
}

// 处理获取角色数据请求
void DBService::HandleGetRoleData(const DBRequest& req, DBResponse& rsp) {
    LOG_INFO("处理获取角色数据请求，角色ID=" << req.role_id());
    
    RoleInfo role_info;
    bool success = GetRoleData(req.role_id(), role_info);
    
    if (success) {
        rsp.set_ret_code(0);
        rsp.set_ret_msg("获取角色数据成功");
        
        // 填充角色信息
        auto role_data = rsp.mutable_role_info();
        role_data->set_role_id(role_info.role_id());
        role_data->set_account_id(role_info.account_id());
        role_data->set_server_id(role_info.server_id());
        role_data->set_role_name(role_info.role_name());
        role_data->set_career(role_info.career());
        role_data->set_level(role_info.level());
        role_data->set_exp(role_info.exp());
        role_data->set_head_id(role_info.head_id());
        role_data->set_portrait_frame(role_info.portrait_frame());
        role_data->set_create_time(role_info.create_time());
        role_data->set_last_login_time(role_info.last_login_time());
        role_data->set_is_deleted(role_info.is_deleted());
        role_data->set_delete_time(role_info.delete_time());
    } else {
        rsp.set_ret_code(1);
        rsp.set_ret_msg("获取角色数据失败");
    }
}

bool DBService::ConnectToDatabase() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    
    for (int i = 0; i < 5; ++i) {
        std::map<std::string, std::string> params;
        params["host"] = db_host_;
        params["port"] = std::to_string(db_port_);
        params["user"] = db_user_;
        params["passwd"] = db_password_;
        params["dbname"] = db_name_;

        auto conn = std::make_shared<ancfl::MySQL>(params);
        if (!conn->connect()) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to connect to database";
            return false;
        }
        db_connections_.push_back(conn);
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Connected to database successfully";
    return true;
}

std::shared_ptr<ancfl::MySQL> DBService::GetDBConnection() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (db_connections_.empty()) {
        return nullptr;
    }
    auto conn = db_connections_.back();
    db_connections_.pop_back();
    return conn;
}

void DBService::ReleaseDBConnection(std::shared_ptr<ancfl::MySQL> conn) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    db_connections_.push_back(conn);
}

bool DBService::CreateRole(const RoleInfo& role_info) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    std::string sql =
        "INSERT INTO role_base (account_id, server_id, role_name, career, "
        "level, exp, head_id, portrait_frame, create_time, last_login_time, "
        "is_deleted, delete_time) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    try {
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            return false;
        }

        stmt->bindInt64(1, role_info.account_id());
        stmt->bindInt32(2, role_info.server_id());
        stmt->bindString(3, role_info.role_name());
        stmt->bindInt32(4, role_info.career());
        stmt->bindInt32(5, role_info.level());
        stmt->bindInt64(6, role_info.exp());
        stmt->bindInt32(7, role_info.head_id());
        stmt->bindInt32(8, role_info.portrait_frame());
        stmt->bindInt64(9, role_info.create_time());
        stmt->bindInt64(10, role_info.last_login_time());
        stmt->bindInt8(11, role_info.is_deleted());
        stmt->bindInt64(12, role_info.delete_time());

        if (!stmt->execute()) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to execute statement";
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in CreateRole: " << e.what();
        return false;
    }
}

bool DBService::UpdateRole(const RoleInfo& role_info) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    std::string sql =
        "UPDATE role_base SET account_id=?, server_id=?, role_name=?, "
        "career=?, level=?, exp=?, head_id=?, portrait_frame=?, "
        "last_login_time=?, is_deleted=?, delete_time=? "
        "WHERE role_id=?";

    try {
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            return false;
        }

        stmt->bindInt64(1, role_info.account_id());
        stmt->bindInt32(2, role_info.server_id());
        stmt->bindString(3, role_info.role_name());
        stmt->bindInt32(4, role_info.career());
        stmt->bindInt32(5, role_info.level());
        stmt->bindInt64(6, role_info.exp());
        stmt->bindInt32(7, role_info.head_id());
        stmt->bindInt32(8, role_info.portrait_frame());
        stmt->bindInt64(9, role_info.last_login_time());
        stmt->bindInt8(10, role_info.is_deleted());
        stmt->bindInt64(11, role_info.delete_time());
        stmt->bindInt64(12, role_info.role_id());

        if (!stmt->execute()) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to execute statement";
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in UpdateRole: " << e.what();
        return false;
    }
}

bool DBService::DeleteRole(uint64_t role_id) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    std::string sql = "DELETE FROM role_base WHERE role_id=?";

    try {
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            return false;
        }

        stmt->bindInt64(1, role_id);

        if (!stmt->execute()) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to execute statement";
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in DeleteRole: " << e.what();
        return false;
    }
}

bool DBService::GetRoleData(uint64_t role_id, RoleInfo& role_info) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    std::string sql =
        "SELECT role_id, account_id, server_id, role_name, career, "
        "level, exp, head_id, portrait_frame, create_time, last_login_time, "
        "is_deleted, delete_time FROM role_base WHERE role_id=?";

    try {
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            return false;
        }

        stmt->bindInt64(1, role_id);

        auto result = stmt->query();
        if (!result) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to query";
            return false;
        }

        if (result->next()) {
            role_info.set_role_id(result->getInt64(0));
            role_info.set_account_id(result->getInt64(1));
            role_info.set_server_id(result->getInt32(2));
            role_info.set_role_name(result->getString(3));
            role_info.set_career(result->getInt32(4));
            role_info.set_level(result->getInt32(5));
            role_info.set_exp(result->getInt64(6));
            role_info.set_head_id(result->getInt32(7));
            role_info.set_portrait_frame(result->getInt32(8));
            role_info.set_create_time(result->getInt64(9));
            role_info.set_last_login_time(result->getInt64(10));
            role_info.set_is_deleted(result->getInt8(11));
            role_info.set_delete_time(result->getInt64(12));
            return true;
        }

        return false;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in GetRoleData: " << e.what();
        return false;
    }
}

bool DBService::GetRoleList(uint64_t account_id, std::vector<RoleInfo>& roles) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    std::string sql =
        "SELECT role_id, account_id, server_id, role_name, career, "
        "level, exp, head_id, portrait_frame, create_time, last_login_time, "
        "is_deleted, delete_time FROM role_base WHERE account_id=?";

    try {
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            return false;
        }

        stmt->bindInt64(1, account_id);

        auto result = stmt->query();
        if (!result) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to query";
            return false;
        }

        roles.clear();
        while (result->next()) {
            RoleInfo role_info;
            role_info.set_role_id(result->getInt64(0));
            role_info.set_account_id(result->getInt64(1));
            role_info.set_server_id(result->getInt32(2));
            role_info.set_role_name(result->getString(3));
            role_info.set_career(result->getInt32(4));
            role_info.set_level(result->getInt32(5));
            role_info.set_exp(result->getInt64(6));
            role_info.set_head_id(result->getInt32(7));
            role_info.set_portrait_frame(result->getInt32(8));
            role_info.set_create_time(result->getInt64(9));
            role_info.set_last_login_time(result->getInt64(10));
            role_info.set_is_deleted(result->getInt8(11));
            role_info.set_delete_time(result->getInt64(12));
            roles.push_back(role_info);
        }

        return true;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in GetRoleList: " << e.what();
        return false;
    }
}

bool DBService::CreateAccount(const std::string& account_name,
                              const std::string& password,
                              int32_t channel,
                              uint64_t& account_id) {
    auto conn = GetDBConnection();
    if (!conn) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to get database connection";
        return false;
    }

    std::string sql =
        "INSERT INTO account (account_name, password, channel, create_time, "
        "last_login_time, is_sealed, seal_end_time) VALUES (?, ?, ?, ?, ?, ?, ?)";

    try {
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            ReleaseDBConnection(conn);
            return false;
        }

        std::string encrypted_password = Sha1Hash(password);
        int64_t now = time(nullptr);

        stmt->bindString(1, account_name);
        stmt->bindString(2, encrypted_password);
        stmt->bindInt32(3, channel);
        stmt->bindInt64(4, now);
        stmt->bindInt64(5, now);
        stmt->bindInt32(6, 0);
        stmt->bindInt64(7, 0);

        if (!stmt->execute()) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to execute statement";
            ReleaseDBConnection(conn);
            return false;
        }

        account_id = conn->getLastInsertId();
        
        // 检查账号ID是否有效
        if (account_id == 0) {
            // 尝试从数据库中查询刚创建的账号的ID
            std::string query_sql = "SELECT account_id FROM account WHERE account_name = ?";
            auto query_stmt = conn->prepare(query_sql);
            if (query_stmt) {
                query_stmt->bindString(1, account_name);
                auto result = query_stmt->query();
                if (result && result->next()) {
                    account_id = result->getInt64(0);
                    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Retrieved account_id from database: " << account_id;
                }
            }
        }
        
        ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "CreateAccount: account_id=" << account_id;
        ReleaseDBConnection(conn);
        return account_id > 0;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in CreateAccount: " << e.what();
        ReleaseDBConnection(conn);
        return false;
    }
}

bool DBService::VerifyAccount(const std::string& account_name,
                              const std::string& password,
                              uint64_t& account_id) {
    auto conn = GetDBConnection();
    if (!conn) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to get database connection";
        return false;
    }

    std::string sql =
        "SELECT account_id, password FROM account WHERE account_name=?";

    try {
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            ReleaseDBConnection(conn);
            return false;
        }

        stmt->bindString(1, account_name);

        auto result = stmt->query();
        if (!result) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to query";
            ReleaseDBConnection(conn);
            return false;
        }

        if (result->next()) {
            account_id = result->getInt64(0);
            std::string stored_password = result->getString(1);
            std::string encrypted_password = ancfl::md5(password);
            ReleaseDBConnection(conn);
            return stored_password == encrypted_password;
        }

        ReleaseDBConnection(conn);
        return false;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in VerifyAccount: " << e.what();
        ReleaseDBConnection(conn);
        return false;
    }
}

bool DBService::GetAccountInfo(uint64_t account_id, AccountInfo& account_info) {
    // 先从缓存中获取
    if (GetAccountFromCache(account_id, account_info)) {
        return true;
    }

    // 缓存中没有，从数据库中查询
    auto conn = GetDBConnection();
    if (!conn) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to get database connection";
        return false;
    }

    std::string sql =
        "SELECT account_id, account_name, channel, create_time, last_login_time, "
        "is_sealed, seal_end_time FROM account WHERE account_id=?";

    try {
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            ReleaseDBConnection(conn);
            return false;
        }

        stmt->bindInt64(1, account_id);

        auto result = stmt->query();
        if (!result) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to query";
            ReleaseDBConnection(conn);
            return false;
        }

        if (result->next()) {
            account_info.account_id = result->getInt64(0);
            account_info.account_name = result->getString(1);
            account_info.channel = result->getInt32(2);
            account_info.create_time = result->getInt64(3);
            account_info.last_login_time = result->getInt64(4);
            account_info.is_sealed = result->getInt32(5);
            account_info.seal_end_time = result->getInt64(6);
            
            // 更新缓存
            UpdateAccountCache(account_info);
            ReleaseDBConnection(conn);
            return true;
        }

        ReleaseDBConnection(conn);
        return false;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in GetAccountInfo: " << e.what();
        ReleaseDBConnection(conn);
        return false;
    }
}

bool DBService::SealAccount(uint64_t account_id, int64_t seal_end_time) {
    auto conn = GetDBConnection();
    if (!conn) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to get database connection";
        return false;
    }

    std::string sql =
        "UPDATE account SET is_sealed=1, seal_end_time=? WHERE account_id=?";

    try {
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            ReleaseDBConnection(conn);
            return false;
        }

        stmt->bindInt64(1, seal_end_time);
        stmt->bindInt64(2, account_id);

        if (!stmt->execute()) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to execute statement";
            ReleaseDBConnection(conn);
            return false;
        }

        // 更新缓存
        AccountInfo info;
        if (GetAccountFromCache(account_id, info)) {
            info.is_sealed = true;
            info.seal_end_time = seal_end_time;
            UpdateAccountCache(info);
        }

        ReleaseDBConnection(conn);
        return true;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in SealAccount: " << e.what();
        ReleaseDBConnection(conn);
        return false;
    }
}

bool DBService::UnsealAccount(uint64_t account_id) {
    auto conn = GetDBConnection();
    if (!conn) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to get database connection";
        return false;
    }

    std::string sql =
        "UPDATE account SET is_sealed=0, seal_end_time=0 WHERE account_id=?";

    try {
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            ReleaseDBConnection(conn);
            return false;
        }

        stmt->bindInt64(1, account_id);

        if (!stmt->execute()) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to execute statement";
            ReleaseDBConnection(conn);
            return false;
        }

        // 更新缓存
        AccountInfo info;
        if (GetAccountFromCache(account_id, info)) {
            info.is_sealed = false;
            info.seal_end_time = 0;
            UpdateAccountCache(info);
        }

        ReleaseDBConnection(conn);
        return true;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in UnsealAccount: " << e.what();
        ReleaseDBConnection(conn);
        return false;
    }
}

bool DBService::IsAccountSealed(uint64_t account_id) {
    // 先从缓存中获取
    AccountInfo info;
    if (GetAccountFromCache(account_id, info)) {
        return info.is_sealed && info.seal_end_time > time(nullptr);
    }

    // 缓存中没有，从数据库中查询
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    std::string sql =
        "SELECT is_sealed, seal_end_time FROM account WHERE account_id=?";

    try {
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            return false;
        }

        stmt->bindInt64(1, account_id);

        auto result = stmt->query();
        if (!result) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to query";
            return false;
        }

        if (result->next()) {
            int32_t is_sealed = result->getInt32(0);
            int64_t seal_end_time = result->getInt64(1);

            if (is_sealed && seal_end_time > time(nullptr)) {
                return true;
            }
        }

        return false;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in IsAccountSealed: " << e.what();
        return false;
    }
}

bool DBService::RecordLoginLog(uint64_t account_id,
                               int32_t channel,
                               const std::string& version,
                               const std::string& uuid,
                               const std::string& idfa,
                               const std::string& imodel,
                               const std::string& imei,
                               int32_t ip) {
    auto conn = GetDBConnection();
    if (!conn) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to get database connection";
        return false;
    }

    std::string sql =
        "INSERT INTO account_login_log (account_id, login_time, login_ip, "
        "channel, version, uuid, idfa, imodel, imei) VALUES (?, ?, ?, ?, ?, "
        "?, ?, ?, ?)";

    try {
        // 检查账号ID是否有效
        if (account_id == 0) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Invalid account_id: " << account_id;
            ReleaseDBConnection(conn);
            return false;
        }
        
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "SQL: " << sql;
            ReleaseDBConnection(conn);
            return false;
        }

        int64_t now = time(nullptr);
        stmt->bindInt64(1, account_id);
        stmt->bindInt64(2, now);
        stmt->bindInt32(3, ip);
        stmt->bindInt32(4, channel);
        stmt->bindString(5, version);
        stmt->bindString(6, uuid);
        stmt->bindString(7, idfa);
        stmt->bindString(8, imodel);
        stmt->bindString(9, imei);

        ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Executing RecordLoginLog SQL: " << sql;
        ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Params: account_id=" << account_id << ", now=" << now << ", ip=" << ip << ", channel=" << channel;
        ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Params: version=" << version << ", uuid=" << uuid << ", idfa=" << idfa << ", imodel=" << imodel << ", imei=" << imei;

        if (!stmt->execute()) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to execute statement";
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "SQL: " << sql;
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Params: account_id=" << account_id << ", now=" << now << ", ip=" << ip << ", channel=" << channel;
            ReleaseDBConnection(conn);
            return false;
        }

        ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "RecordLoginLog executed successfully for account_id: " << account_id;
        ReleaseDBConnection(conn);
        return true;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in RecordLoginLog: " << e.what();
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "SQL: " << sql;
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Params: account_id=" << account_id << ", channel=" << channel << ", ip=" << ip;
        ReleaseDBConnection(conn);
        return false;
    }
}

// 更新账号缓存
void DBService::UpdateAccountCache(const AccountInfo& info) {
    ancfl::Mutex::Lock lock(cache_mutex_);
    account_cache_[info.account_id] = info;
    name_to_id_[info.account_name] = info.account_id;
}

// 从缓存中获取账号信息（通过账号ID）
bool DBService::GetAccountFromCache(uint64_t account_id, AccountInfo& info) {
    ancfl::Mutex::Lock lock(cache_mutex_);
    auto it = account_cache_.find(account_id);
    if (it != account_cache_.end()) {
        info = it->second;
        return true;
    }
    return false;
}

// 从缓存中获取账号信息（通过账号名称）
bool DBService::GetAccountFromCache(const std::string& account_name, AccountInfo& info) {
    ancfl::Mutex::Lock lock(cache_mutex_);
    auto it = name_to_id_.find(account_name);
    if (it != name_to_id_.end()) {
        auto acc_it = account_cache_.find(it->second);
        if (acc_it != account_cache_.end()) {
            info = acc_it->second;
            return true;
        }
    }
    return false;
}

} // namespace game_server
