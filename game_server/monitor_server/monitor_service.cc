#include "monitor_service.h"
#include "ancfl/config.h"
#include <ctime>
#include <yaml-cpp/yaml.h>

namespace game_server {

// 日志宏，添加时间戳和统一前缀
#define LOG_INFO(msg) do { \
    time_t now = time(nullptr); \
    struct tm* tm_info = localtime(&now); \
    char time_str[20]; \
    strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", tm_info); \
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "[" << time_str << "] [监控服务器] " << msg; \
} while(0)

#define LOG_ERROR(msg) do { \
    time_t now = time(nullptr); \
    struct tm* tm_info = localtime(&now); \
    char time_str[20]; \
    strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", tm_info); \
    ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "[" << time_str << "] [监控服务器] 错误: " << msg; \
} while(0)

MonitorService::MonitorService()
    : GameServiceBase("monitor_server")
    , center_server_ip_("127.0.0.1")
    , center_server_port_(8007)
    , center_server_conn_(nullptr) {}

MonitorService::~MonitorService() {}

bool MonitorService::InitService() {
    LOG_INFO("正在初始化监控服务器...");

    // 读取配置文件
    std::string config_path = "bin/conf/servers.yml";
    
    std::string ip = "0.0.0.0";
    int port = 8009;
    std::string center_host = "127.0.0.1";
    int center_port = 8007;
    
    // 加载配置文件
    try {
        YAML::Node root = YAML::LoadFile(config_path);
        
        // 读取监控服务器配置
        if (root["monitor_server"]) {
            if (root["monitor_server"]["host"]) {
                ip = root["monitor_server"]["host"].as<std::string>();
            }
            if (root["monitor_server"]["port"]) {
                port = root["monitor_server"]["port"].as<int>();
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

    LOG_INFO("监控服务器初始化成功");
    return true;
}

void MonitorService::UninitService() {
    LOG_INFO("正在反初始化监控服务器...");
    LOG_INFO("监控服务器已反初始化");
}

void MonitorService::RegisterAllHandlers() {
    // 注册消息处理器（暂时不实现）
    LOG_INFO("消息处理器注册完成");
}

void MonitorService::OnTimer() {
    // 每秒定时器（暂时不实现）
}

void MonitorService::OnTimer5s() {
    LOG_INFO("执行5秒定时器回调");
    SendHeartbeatToCenterServer();
}

// ==================== 中心服务器连接管理 ====================

bool MonitorService::ConnectToCenterServer() {
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

bool MonitorService::RegisterToCenterServer() {
    if (!center_server_conn_) {
        LOG_ERROR("中心服务器连接未建立");
        return false;
    }
    
    // 构建注册消息
    uint32_t msg_id = 1001; // 假设1001是注册消息ID
    std::string data = "monitor_server_register";
    
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

void MonitorService::SendHeartbeatToCenterServer() {
    if (!center_server_conn_) {
        LOG_ERROR("中心服务器连接未建立");
        return;
    }
    
    // 构建心跳消息
    uint32_t msg_id = 1002; // 假设1002是心跳消息ID
    std::string data = "monitor_server_heartbeat";
    
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

} // namespace game_server
