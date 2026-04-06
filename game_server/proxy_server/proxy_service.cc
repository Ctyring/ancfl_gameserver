#include "proxy_service.h"
#include "proto/msg_account.pb.h"
#include "proto/msg_id.pb.h"
  #include "ancfl/config.h"

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
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "[" << time_str << "] [代理服务器] " << msg; \
} while(0)

#define LOG_ERROR(msg) do { \
    time_t now = time(nullptr); \
    struct tm* tm_info = localtime(&now); \
    char time_str[20]; \
    strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", tm_info); \
    ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "[" << time_str << "] [代理服务器] 错误: " << msg; \
} while(0)

ProxyService::ProxyService() 
    : GameServiceBase("proxy_server")
    , next_conn_id_(1)
    , max_connections_(10000) // 默认最大连接数10000
    , test_msg_sent_(false)
    , center_server_id_(1)
    , center_server_ip_("127.0.0.1")
    , center_server_port_(8007)
    , center_server_conn_(nullptr) {
}

ProxyService::~ProxyService() {
    UninitService();
}

/**
 * @brief 初始化代理服务器服务
 * 
 * 主要完成以下工作：
 * 1. 读取配置文件
 * 2. 绑定到配置的端口
 * 3. 启动TCP服务器
 * 4. 注册消息处理器
 * 
 * @return true 初始化成功
 * @return false 初始化失败
 */
bool ProxyService::InitService() {
    // 读取配置文件
    std::string config_path = "bin/conf/servers.yml";
    
    // 配置变量
    std::string host = "0.0.0.0";
    int port = 8008;
    size_t max_connections = 10000;
    int recv_timeout = 30000;
    int send_timeout = 10000;
    int center_port = 8007;
    
    // 加载配置文件
    try {
        YAML::Node root = YAML::LoadFile(config_path);
        // 直接从YAML节点读取配置，避免与其他配置文件冲突
        
        // 读取代理服务器配置
        if (root["proxy_server"]) {
            if (root["proxy_server"]["host"]) {
                host = root["proxy_server"]["host"].as<std::string>();
            }
            if (root["proxy_server"]["port"]) {
                port = root["proxy_server"]["port"].as<int>();
            }
            if (root["proxy_server"]["max_connections"]) {
                max_connections = root["proxy_server"]["max_connections"].as<size_t>();
            }
            if (root["proxy_server"]["recv_timeout"]) {
                recv_timeout = root["proxy_server"]["recv_timeout"].as<int>();
            }
            if (root["proxy_server"]["send_timeout"]) {
                send_timeout = root["proxy_server"]["send_timeout"].as<int>();
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
        
        max_connections_ = max_connections;
        center_server_ip_ = center_host;
        center_server_port_ = center_port;
        
        LOG_INFO("正在初始化网络，绑定地址: " << host << ":" << port);
        LOG_INFO("最大连接数: " << max_connections_);
        LOG_INFO("接收超时: " << recv_timeout << "ms");
        LOG_INFO("发送超时: " << send_timeout << "ms");
        LOG_INFO("中心服务器地址: " << center_host << ":" << center_port);
    } catch (std::exception& e) {
        LOG_ERROR("加载配置文件失败: " << config_path << "，错误: " << e.what());
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
    
    RegisterAllHandlers();
    
    // 连接中心服务器
    if (!ConnectToCenterServer()) {
        LOG_ERROR("连接中心服务器失败");
        return false;
    }
    
    // 向中心服务器注册
    if (!RegisterToCenterServer()) {
        LOG_ERROR("向中心服务器注册失败");
        return false;
    }
    
    LOG_INFO("代理服务器初始化成功");
    return true;
}

/**
 * @brief 反初始化代理服务器服务
 * 
 * 主要完成以下工作：
 * 1. 关闭所有客户端连接
 * 2. 清空连接映射表
 * 3. 清空心跳时间记录
 */
void ProxyService::UninitService() {
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
    
    LOG_INFO("代理服务器已反初始化");
}

/**
 * @brief 注册所有消息处理器
 * 
 * 注意：当前实现在HandleRecv中直接处理消息，不需要注册处理器
 * 保留此方法是为了保持代码结构完整性，方便后续扩展
 */
void ProxyService::RegisterAllHandlers() {
    LOG_INFO("所有消息处理器已注册");
}

/**
 * @brief 连接中心服务器
 * 
 * @return true 连接成功
 * @return false 连接失败
 */
bool ProxyService::ConnectToCenterServer() {
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

/**
 * @brief 向中心服务器注册
 * 
 * @return true 注册成功
 * @return false 注册失败
 */
bool ProxyService::RegisterToCenterServer() {
    if (!center_server_conn_) {
        LOG_ERROR("中心服务器连接未建立");
        return false;
    }
    
    // 构建注册消息
    uint32_t msg_id = 1001; // 假设1001是注册消息ID
    std::string data = "proxy_server_register";
    
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

/**
 * @brief 发送心跳到中心服务器
 */
void ProxyService::SendHeartbeatToCenterServer() {
    if (!center_server_conn_) {
        LOG_ERROR("中心服务器连接未建立");
        return;
    }
    
    // 构建心跳消息
    uint32_t msg_id = 1002; // 假设1002是心跳消息ID
    std::string data = "proxy_server_heartbeat";
    
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

/**
 * @brief 5秒定时器回调
 * 
 * 主要完成以下工作：
 * 1. 向中心服务器发送心跳消息
 */
void ProxyService::OnTimer5s() {
    LOG_INFO("执行5秒定时器回调");
    SendHeartbeatToCenterServer();
}

/**
 * @brief 处理客户端连接（重写父类方法）
 * 
 * 主要流程：
 * 1. 检查连接数是否超过最大值
 * 2. 为客户端分配连接ID并加入连接映射表
 * 3. 发送初始测试消息给客户端
 * 4. 启动接收协程处理客户端消息
 * 
 * @param client 客户端socket
 */
void ProxyService::handleClient(ancfl::Socket::ptr client) {
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
    
    AccountLoginAck ack;
    ack.set_ret_code(0);
    ack.set_account_id(12345);
    ack.set_last_svr_id(1);
    ack.set_last_svr_name("TestServer");
    
    std::string data;
    if (ack.SerializeToString(&data)) {
        uint32_t msg_id = static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_ACK);
        
        MessageHeader header;
        header.msg_id = msg_id;
        header.msg_len = sizeof(header) + data.size();
        header.target_id = 0;
        header.user_data = 0;

        header.msg_id = ancfl::byteswapOnLittleEndian(header.msg_id);
        header.msg_len = ancfl::byteswapOnLittleEndian(header.msg_len);
        header.target_id = ancfl::byteswapOnLittleEndian(header.target_id);
        header.user_data = ancfl::byteswapOnLittleEndian(header.user_data);

        int ret = client->send(&header, sizeof(header));
        if (ret > 0) {
            ret = client->send(data.data(), data.size());
            if (ret > 0) {
                test_msg_sent_ = true;
                LOG_INFO("已发送初始消息给客户端，消息ID=" << msg_id);
            } else {
                LOG_ERROR("发送初始消息体失败，返回值=" << ret);
            }
        } else {
            LOG_ERROR("发送初始消息头失败，返回值=" << ret);
        }
    } else {
        LOG_ERROR("序列化初始消息失败");
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    if (m_worker) {
        m_worker->schedule(std::bind(&ProxyService::HandleRecv, this, client, conn_id));
    } else {
        LOG_ERROR("工作线程为空，无法调度HandleRecv");
    }
}

/**
 * @brief 处理接收客户端消息
 * 
 * 主要流程：
 * 1. 循环接收消息头（20字节）
 * 2. 解析消息头并进行大小端转换
 * 3. 接收消息体
 * 4. 根据消息类型处理消息
 * 5. 发送响应消息
 * 
 * 消息头结构（20字节）：
 * - msg_id (4字节): 消息ID
 * - msg_len (4字节): 消息总长度（包含消息头）
 * - target_id (8字节): 目标ID
 * - user_data (4字节): 用户数据
 * 
 * @param client 客户端socket
 * @param conn_id 连接ID
 */
void ProxyService::HandleRecv(ancfl::Socket::ptr client, int32_t conn_id) {
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
            
            if (header.msg_id == static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_REQ)) {
                AccountLoginReq login_req;
                if (login_req.ParseFromArray(buffer.data(), body_len)) {
                    LOG_INFO("收到登录请求，用户: " << login_req.account_name());
                    
                    AccountLoginAck ack;
                    ack.set_ret_code(0);
                    ack.set_account_id(67890);
                    ack.set_last_svr_id(1);
                    ack.set_last_svr_name("TestServer");
                    
                    std::string data;
                    if (ack.SerializeToString(&data)) {
                        MessageHeader resp_header;
                        resp_header.msg_id = static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_ACK);
                        resp_header.msg_len = sizeof(resp_header) + data.size();
                        resp_header.target_id = 0;
                        resp_header.user_data = 0;

                        resp_header.msg_id = ancfl::byteswapOnLittleEndian(resp_header.msg_id);
                        resp_header.msg_len = ancfl::byteswapOnLittleEndian(resp_header.msg_len);
                        resp_header.target_id = ancfl::byteswapOnLittleEndian(resp_header.target_id);
                        resp_header.user_data = ancfl::byteswapOnLittleEndian(resp_header.user_data);

                        ret = client->send(&resp_header, sizeof(resp_header));
                        if (ret > 0) {
                            ret = client->send(data.data(), data.size());
                            if (ret > 0) {
                                LOG_INFO("登录响应发送成功");
                            } else {
                                LOG_ERROR("发送登录响应体失败，返回值=" << ret);
                            }
                        } else {
                            LOG_ERROR("发送登录响应头失败，返回值=" << ret);
                        }
                    } else {
                        LOG_ERROR("序列化登录响应失败");
                    }
                } else {
                    LOG_ERROR("解析登录请求失败");
                }
            } else {
                LOG_INFO("收到未知消息类型: " << header.msg_id);
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

/**
 * @brief 获取当前连接数
 * @return 当前连接数
 */
size_t ProxyService::GetConnectionCount() const {
    ancfl::Mutex::Lock lock(const_cast<ancfl::Mutex&>(conn_mutex_));
    return connections_.size();
}

/**
 * @brief 根据连接ID获取客户端socket
 * @param conn_id 连接ID
 * @return 客户端socket，如果不存在返回nullptr
 */
ancfl::Socket::ptr ProxyService::GetClientByConnId(uint32_t conn_id) {
    ancfl::Mutex::Lock lock(conn_mutex_);
    auto it = connections_.find(conn_id);
    if (it != connections_.end()) {
        return it->second;
    }
    return nullptr;
}

/**
 * @brief 设置最大连接数
 * @param max_connections 最大连接数
 */
void ProxyService::SetMaxConnections(size_t max_connections) {
    max_connections_ = max_connections;
    LOG_INFO("最大连接数已设置为: " << max_connections_);
}

} // namespace game_server
