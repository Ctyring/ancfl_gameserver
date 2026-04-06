#include <iostream>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

#include "common/tcp_client.h"
#include "proto/msg_account.pb.h"
#include "proto/msg_id.pb.h"

using namespace game_server;

int main(int argc, char** argv) {
    std::cout << "[Client] Starting TCP client..." << std::endl;
    
    // 创建客户端
    TcpClient client;
    
    // 连接到服务器
    std::cout << "[Client] Connecting to server..." << std::endl;
    if (!client.Connect("127.0.0.1:8005")) { // 连接到代理服务器的端口
        std::cerr << "[Client] Failed to connect to server" << std::endl;
        return 1;
    }
    std::cout << "[Client] Connected to server" << std::endl;
    
    // 等待服务器处理连接并发送消息
    std::cout << "[Client] Waiting for server to send message..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // 尝试接收服务器发送的初始消息
    std::cout << "[Client] Attempting to receive initial message from server..." << std::endl;
    NetPacket packet;
    bool received = false;
    
    // 尝试多次接收消息，最多尝试 5 次
    for (int i = 0; i < 5; ++i) {
        std::cout << "[Client] Attempt " << i+1 << " to receive initial message" << std::endl;
        if (client.RecvMessage(packet)) {
            std::cout << "[Client] Initial message received successfully!" << std::endl;
            received = true;
            break;
        } else {
            std::cout << "[Client] Failed to receive initial message, error: " << strerror(errno) << std::endl;
        }
        std::cout << "[Client] Retrying to receive initial message..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    if (received) {
        // 验证消息
        std::cout << "[Client] Message received, msg_id=" << packet.msg_id << std::endl;
        if (packet.msg_id == static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_ACK)) {
            std::cout << "[Client] Received login ack message" << std::endl;
            
            if (packet.msg != nullptr) {
                auto login_ack = dynamic_cast<AccountLoginAck*>(packet.msg.get());
                if (login_ack != nullptr) {
                    // 验证消息内容
                    std::cout << "[Client] Login ack details:" << std::endl;
                    std::cout << "  ret_code: " << login_ack->ret_code() << std::endl;
                    std::cout << "  account_id: " << login_ack->account_id() << std::endl;
                    std::cout << "  last_svr_id: " << login_ack->last_svr_id() << std::endl;
                    std::cout << "  last_svr_name: " << login_ack->last_svr_name() << std::endl;
                }
            }
        }
    } else {
        std::cout << "[Client] Failed to receive initial message" << std::endl;
    }
    
    // 发送登录请求给服务器
    std::cout << "[Client] Sending login request to server..." << std::endl;
    AccountLoginReq login_req;
    login_req.set_account_name("test_user");
    login_req.set_password("test_password");
    
    if (client.SendMessage(static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_REQ), 0, 0, login_req)) {
        std::cout << "[Client] Login request sent successfully" << std::endl;
    } else {
        std::cout << "[Client] Failed to send login request" << std::endl;
    }
    
    // 等待服务器响应
    std::cout << "[Client] Waiting for server response..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    // 尝试接收服务器的响应
    std::cout << "[Client] Attempting to receive server response..." << std::endl;
    bool response_received = false;
    
    // 尝试多次接收消息，最多尝试 5 次
    for (int i = 0; i < 5; ++i) {
        std::cout << "[Client] Attempt " << i+1 << " to receive response" << std::endl;
        if (client.RecvMessage(packet)) {
            std::cout << "[Client] Response received successfully!" << std::endl;
            response_received = true;
            break;
        }
        std::cout << "[Client] Retrying to receive response..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    if (response_received) {
        // 验证响应消息
        std::cout << "[Client] Response received, msg_id=" << packet.msg_id << std::endl;
        if (packet.msg_id == static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_ACK)) {
            std::cout << "[Client] Received login response" << std::endl;
            
            if (packet.msg != nullptr) {
                auto login_ack = dynamic_cast<AccountLoginAck*>(packet.msg.get());
                if (login_ack != nullptr) {
                    // 验证消息内容
                    std::cout << "[Client] Login response details:" << std::endl;
                    std::cout << "  ret_code: " << login_ack->ret_code() << std::endl;
                    std::cout << "  account_id: " << login_ack->account_id() << std::endl;
                    std::cout << "  last_svr_id: " << login_ack->last_svr_id() << std::endl;
                    std::cout << "  last_svr_name: " << login_ack->last_svr_name() << std::endl;
                }
            }
        }
    } else {
        std::cout << "[Client] Failed to receive server response" << std::endl;
    }
    
    // 等待一段时间，确保所有消息都已处理
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    
    // 断开连接
    client.Disconnect();
    std::cout << "[Client] Disconnected from server" << std::endl;
    
    std::cout << "[Client] Client exiting" << std::endl;
    return 0;
}
