#include <iostream>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

#include "game_server/common/tcp_client.h"
#include "game_server/proto/msg_account.pb.h"
#include "game_server/proto/msg_id.pb.h"

using namespace game_server;

int main(int argc, char** argv) {
    std::cout << "[客户端] 启动登录测试客户端..." << std::endl;
    
    // 创建客户端
    TcpClient client;
    
    // 连接到登录服务器
    std::cout << "[客户端] 连接到登录服务器..." << std::endl;
    if (!client.Connect("127.0.0.1:8000")) { // 连接到登录服务器的端口
        std::cerr << "[客户端] 连接登录服务器失败" << std::endl;
        return 1;
    }
    std::cout << "[客户端] 已连接到登录服务器" << std::endl;
    
    // 发送注册请求
    std::cout << "[客户端] 发送注册请求到登录服务器..." << std::endl;
    AccountRegReq register_req;
    register_req.set_account_name("test_user_123");
    register_req.set_password("test_password_123");
    
    if (client.SendMessage(static_cast<uint32_t>(MessageID::MSG_ACCOUNT_REG_REQ), 0, 0, register_req)) {
        std::cout << "[客户端] 注册请求发送成功" << std::endl;
    } else {
        std::cout << "[客户端] 注册请求发送失败" << std::endl;
    }
    
    // 等待服务器响应
    std::cout << "[客户端] 等待注册服务器响应..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // 尝试接收服务器的响应
    std::cout << "[客户端] 尝试接收注册服务器响应..." << std::endl;
    NetPacket packet;
    bool response_received = false;
    
    // 尝试多次接收消息，最多尝试 5 次
    for (int i = 0; i < 5; ++i) {
        std::cout << "[客户端] 尝试 " << i+1 << " 次接收注册响应" << std::endl;
        if (client.RecvMessage(packet)) {
            std::cout << "[客户端] 注册响应接收成功！" << std::endl;
            response_received = true;
            break;
        }
        std::cout << "[客户端] 重试接收注册响应..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    if (response_received) {
        // 验证响应消息
        std::cout << "[客户端] 收到注册响应, msg_id=" << packet.msg_id << std::endl;
        if (packet.msg_id == static_cast<uint32_t>(MessageID::MSG_ACCOUNT_REG_ACK)) {
            std::cout << "[客户端] 收到注册响应" << std::endl;
            
            if (packet.msg != nullptr) {
                auto register_ack = dynamic_cast<AccountRegAck*>(packet.msg.get());
                if (register_ack != nullptr) {
                    // 验证消息内容
                    std::cout << "[客户端] 注册响应详情:" << std::endl;
                    std::cout << "  ret_code: " << register_ack->ret_code() << std::endl;
                    std::cout << "  account_id: " << register_ack->account_id() << std::endl;
                }
            }
        }
    } else {
        std::cout << "[客户端] 接收注册服务器响应失败" << std::endl;
    }
    
    // 等待一段时间，确保所有消息都已处理
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    // 断开连接
    client.Disconnect();
    std::cout << "[客户端] 已断开与登录服务器的连接" << std::endl;
    
    // 重新连接发送登录请求
    std::cout << "\n[客户端] 连接到登录服务器进行登录..." << std::endl;
    if (!client.Connect("127.0.0.1:8000")) {
        std::cerr << "[客户端] 连接登录服务器失败" << std::endl;
        return 1;
    }
    std::cout << "[客户端] 已连接到登录服务器" << std::endl;
    
    // 发送登录请求
    std::cout << "[客户端] 发送登录请求到登录服务器..." << std::endl;
    AccountLoginReq login_req;
    login_req.set_account_name("test_user_123");
    login_req.set_password("test_password_123");
    
    if (client.SendMessage(static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_REQ), 0, 0, login_req)) {
        std::cout << "[客户端] 登录请求发送成功" << std::endl;
    } else {
        std::cout << "[客户端] 登录请求发送失败" << std::endl;
    }
    
    // 等待服务器响应
    std::cout << "[客户端] 等待登录服务器响应..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // 尝试接收服务器的响应
    std::cout << "[客户端] 尝试接收登录服务器响应..." << std::endl;
    response_received = false;
    
    // 尝试多次接收消息，最多尝试 5 次
    for (int i = 0; i < 5; ++i) {
        std::cout << "[客户端] 尝试 " << i+1 << " 次接收登录响应" << std::endl;
        if (client.RecvMessage(packet)) {
            std::cout << "[客户端] 登录响应接收成功！" << std::endl;
            response_received = true;
            break;
        }
        std::cout << "[客户端] 重试接收登录响应..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    if (response_received) {
        // 验证响应消息
        std::cout << "[客户端] 收到登录响应, msg_id=" << packet.msg_id << std::endl;
        if (packet.msg_id == static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_ACK)) {
            std::cout << "[客户端] 收到登录响应" << std::endl;
            
            if (packet.msg != nullptr) {
                auto login_ack = dynamic_cast<AccountLoginAck*>(packet.msg.get());
                if (login_ack != nullptr) {
                    // 验证消息内容
                    std::cout << "[客户端] 登录响应详情:" << std::endl;
                    std::cout << "  ret_code: " << login_ack->ret_code() << std::endl;
                    std::cout << "  account_id: " << login_ack->account_id() << std::endl;
                    std::cout << "  last_svr_id: " << login_ack->last_svr_id() << std::endl;
                    std::cout << "  last_svr_name: " << login_ack->last_svr_name() << std::endl;
                    
                    // 输出服务器列表
                    std::cout << "[客户端] 服务器列表: " << std::endl;
                    for (int i = 0; i < login_ack->svr_nodes_size(); i++) {
                        const auto& server = login_ack->svr_nodes(i);
                        std::cout << "  服务器 " << i + 1 << ":" << std::endl;
                        std::cout << "    ID: " << server.svr_id() << std::endl;
                        std::cout << "    名称: " << server.svr_name() << std::endl;
                        std::cout << "    状态: " << server.svr_flag() << " (1:流畅, 2:拥挤, 3:爆满)" << std::endl;
                        std::cout << "    标签: " << server.corner_mark() << " (0:无, 1:新服, 2:推荐)" << std::endl;
                        std::cout << "    开放时间: " << server.svr_open_time() << std::endl;
                        std::cout << "    状态: " << server.svr_status() << " (1:在线, 0:离线)" << std::endl;
                        std::cout << "    地址: " << server.svr_addr() << ":" << server.svr_port() << std::endl;
                    }
                }
            }
        }
    } else {
        std::cout << "[客户端] 接收登录服务器响应失败" << std::endl;
    }
    
    // 等待一段时间，确保所有消息都已处理
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    // 断开连接
    client.Disconnect();
    std::cout << "[客户端] 已断开与登录服务器的连接" << std::endl;
    
    std::cout << "[客户端] 登录测试完成" << std::endl;
    return 0;
}
