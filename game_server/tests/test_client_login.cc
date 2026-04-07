#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "tcp_client.h"
#include "proto/msg_account.pb.h"
#include "proto/msg_id.pb.h"

using namespace game_server;

/**
 * 客户端登录测试
 */
class ClientLoginTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 解析命令行参数获取代理服务器地址
        const char* proxy_addr = getenv("PROXY_ADDR");
        if (proxy_addr) {
            proxy_address_ = proxy_addr;
        } else {
            proxy_address_ = "127.0.0.1:8005";
        }
        
        std::cout << "Using proxy server: " << proxy_address_ << std::endl;
    }
    
    std::string proxy_address_;
};

/**
 * 测试完整登录流程
 */
TEST_F(ClientLoginTest, LoginFlow) {
    // 创建TCP客户端
    TcpClient client;
    
    // 连接到代理服务器
    ASSERT_TRUE(client.Connect(proxy_address_)) << "Failed to connect to proxy server";
    
    // 发送登录请求
    AccountLoginReq login_req;
    login_req.set_account_name("test_user");
    login_req.set_password("test_password");
    login_req.set_channel(1);
    
    // 序列化消息
    std::string serialized;
    ASSERT_TRUE(login_req.SerializeToString(&serialized));
    
    // 发送消息到服务器
    uint32_t msg_id = static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_REQ);
    ASSERT_TRUE(client.SendMessage(msg_id, 0, 0, serialized)) << "Failed to send login request";
    
    // 等待响应
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 接收响应
    NetPacket packet;
    ASSERT_TRUE(client.RecvMessage(packet)) << "Failed to receive login response";
    
    // 解析响应
    ASSERT_TRUE(packet.msg != nullptr) << "Message object is null";
    auto login_ack = dynamic_cast<AccountLoginAck*>(packet.msg.get());
    ASSERT_TRUE(login_ack != nullptr) << "Failed to cast to AccountLoginAck";
    
    // 验证登录成功
    EXPECT_EQ(login_ack->ret_code(), 0) << "Login failed with code: " << login_ack->ret_code();
    EXPECT_GT(login_ack->account_id(), 0) << "Account ID is zero";
    
    std::cout << "Login successful! Account ID: " << login_ack->account_id() << std::endl;
    std::cout << "Last server ID: " << login_ack->last_svr_id() << std::endl;
    std::cout << "Last server name: " << login_ack->last_svr_name() << std::endl;
    
    // 输出服务器列表
    std::cout << "Server list: " << std::endl;
    for (int i = 0; i < login_ack->svr_nodes_size(); i++) {
        const auto& server = login_ack->svr_nodes(i);
        std::cout << "  Server " << i + 1 << ":" << std::endl;
        std::cout << "    ID: " << server.svr_id() << std::endl;
        std::cout << "    Name: " << server.svr_name() << std::endl;
        std::cout << "    Flag: " << server.svr_flag() << " (1:流畅, 2:拥挤, 3:爆满)" << std::endl;
        std::cout << "    Corner mark: " << server.corner_mark() << " (0:无, 1:新服, 2:推荐)" << std::endl;
        std::cout << "    Open time: " << server.svr_open_time() << std::endl;
        std::cout << "    Status: " << server.svr_status() << " (1:在线, 0:离线)" << std::endl;
        std::cout << "    Address: " << server.svr_addr() << ":" << server.svr_port() << std::endl;
    }
    
    // 断开连接
    client.Disconnect();
}

/**
 * 测试错误登录
 */
TEST_F(ClientLoginTest, InvalidLogin) {
    TcpClient client;
    ASSERT_TRUE(client.Connect(proxy_address_)) << "Failed to connect to proxy server";
    
    // 发送错误的登录请求
    AccountLoginReq login_req;
    login_req.set_account_name("invalid_user");
    login_req.set_password("wrong_password");
    login_req.set_channel(1);
    
    std::string serialized;
    ASSERT_TRUE(login_req.SerializeToString(&serialized));
    
    uint32_t msg_id = static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_REQ);
    ASSERT_TRUE(client.SendMessage(msg_id, 0, 0, serialized)) << "Failed to send login request";
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    NetPacket packet;
    ASSERT_TRUE(client.RecvMessage(packet)) << "Failed to receive login response";
    
    ASSERT_TRUE(packet.msg != nullptr) << "Message object is null";
    auto login_ack = dynamic_cast<AccountLoginAck*>(packet.msg.get());
    ASSERT_TRUE(login_ack != nullptr) << "Failed to cast to AccountLoginAck";
    
    // 验证登录失败
    EXPECT_NE(login_ack->ret_code(), 0) << "Login should have failed";
    
    std::cout << "Invalid login test passed! Error code: " << login_ack->ret_code() << std::endl;
    
    client.Disconnect();
}

// int main(int argc, char** argv) {
//     // 解析命令行参数
//     for (int i = 1; i < argc; i++) {
//         if (std::string(argv[i]) == "--proxy" && i + 1 < argc) {
//             setenv("PROXY_ADDR", argv[i + 1], 1);
//         }
//     }
//     
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }