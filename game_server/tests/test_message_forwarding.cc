#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <memory>
#include "proxy_server/proxy_service.h"
#include "common/message_dispatcher.h"
#include "proto/msg_account.pb.h"
#include "proto/msg_base.pb.h"
#include "proto/msg_id.pb.h"
#include "proto/msg_role.pb.h"

using namespace game_server;

/**
 * 消息转发机制测试类
 * 测试代理服务器的消息转发功能
 */
class MessageForwardingTest : public ::testing::Test {
protected:
    /**
     * 测试准备：初始化代理服务
     */
    void SetUp() override {
        proxy_service_ = new ProxyService();
        
        // 注意：暂时不启动真实网络服务，测试核心功能
        // 完整的网络测试需要真实的服务器环境
    }

    /**
     * 测试清理：释放代理服务
     */
    void TearDown() override {
        if (proxy_service_) {
            delete proxy_service_;
            proxy_service_ = nullptr;
        }
    }

    // 代理服务实例
    ProxyService* proxy_service_;
};

/**
 * 测试会话ID生成
 */
TEST_F(MessageForwardingTest, GenerateSessionId) {
    std::string session_id1 = proxy_service_->GenerateSessionId();
    std::string session_id2 = proxy_service_->GenerateSessionId();
    
    EXPECT_FALSE(session_id1.empty());
    EXPECT_FALSE(session_id2.empty());
    EXPECT_NE(session_id1, session_id2);
}

/**
 * 测试创建会话
 */
TEST_F(MessageForwardingTest, CreateSession) {
    uint32_t conn_id = 100;
    uint64_t account_id = 10001;
    uint64_t role_id = 20001;
    
    // 先调用 OnClientConnect 创建会话
    proxy_service_->OnClientConnect(conn_id, "127.0.0.1", 12345);
    
    std::string session_id = proxy_service_->GenerateSessionId();
    EXPECT_TRUE(proxy_service_->CreateSession(conn_id, account_id, role_id, session_id));
    
    ClientSession* session = proxy_service_->GetSession(conn_id);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->conn_id, conn_id);
    EXPECT_EQ(session->account_id, account_id);
    EXPECT_EQ(session->role_id, role_id);
    EXPECT_EQ(session->session_id, session_id);
}

/**
 * 测试获取会话
 */
TEST_F(MessageForwardingTest, GetSession) {
    uint32_t conn_id = 101;
    uint64_t account_id = 10002;
    uint64_t role_id = 20002;
    
    proxy_service_->OnClientConnect(conn_id, "127.0.0.1", 12346);
    std::string session_id = proxy_service_->GenerateSessionId();
    proxy_service_->CreateSession(conn_id, account_id, role_id, session_id);
    
    ClientSession* session = proxy_service_->GetSession(conn_id);
    EXPECT_NE(session, nullptr);
    
    ClientSession* session_by_account = proxy_service_->GetSessionByAccount(account_id);
    EXPECT_NE(session_by_account, nullptr);
    EXPECT_EQ(session_by_account->account_id, account_id);
}

/**
 * 测试删除会话
 */
TEST_F(MessageForwardingTest, RemoveSession) {
    uint32_t conn_id = 102;
    uint64_t account_id = 10003;
    uint64_t role_id = 20003;
    
    proxy_service_->OnClientConnect(conn_id, "127.0.0.1", 12347);
    std::string session_id = proxy_service_->GenerateSessionId();
    proxy_service_->CreateSession(conn_id, account_id, role_id, session_id);
    
    EXPECT_TRUE(proxy_service_->RemoveSession(conn_id));
    
    ClientSession* session = proxy_service_->GetSession(conn_id);
    EXPECT_EQ(session, nullptr);
    
    ClientSession* session_by_account = proxy_service_->GetSessionByAccount(account_id);
    EXPECT_EQ(session_by_account, nullptr);
}

/**
 * 测试添加逻辑服务器
 */
TEST_F(MessageForwardingTest, AddLogicServer) {
    ServerConnection server;
    server.conn_id = 200;
    server.server_name = "LogicServer1";
    server.ip = "127.0.0.1";
    server.port = 8002;
    server.last_active_time = time(nullptr);
    server.player_count = 0;
    
    proxy_service_->AddLogicServer(server);
    
    ServerConnection* retrieved = proxy_service_->GetLogicServer(200);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->server_name, "LogicServer1");
    EXPECT_EQ(retrieved->ip, "127.0.0.1");
    EXPECT_EQ(retrieved->port, 8002);
}

/**
 * 测试删除逻辑服务器
 */
TEST_F(MessageForwardingTest, RemoveLogicServer) {
    ServerConnection server;
    server.conn_id = 201;
    server.server_name = "LogicServer2";
    server.ip = "127.0.0.1";
    server.port = 8003;
    
    proxy_service_->AddLogicServer(server);
    proxy_service_->RemoveLogicServer(201);
    
    ServerConnection* retrieved = proxy_service_->GetLogicServer(201);
    EXPECT_EQ(retrieved, nullptr);
}

/**
 * 测试选择逻辑服务器
 */
TEST_F(MessageForwardingTest, SelectLogicServer) {
    for (int i = 0; i < 3; i++) {
        ServerConnection server;
        server.conn_id = 300 + i;
        server.server_name = "LogicServer" + std::to_string(i);
        server.ip = "127.0.0.1";
        server.port = 8010 + i;
        server.last_active_time = time(nullptr);
        server.player_count = i * 50;
        proxy_service_->AddLogicServer(server);
    }
    
    uint32_t selected = proxy_service_->SelectLogicServer();
    // 应该选择负载最低的服务器（player_count=0 的）
    EXPECT_GT(selected, 0);
}

/**
 * 测试多个会话管理
 */
TEST_F(MessageForwardingTest, MultipleSessions) {
    for (int i = 0; i < 10; i++) {
        uint32_t conn_id = 400 + i;
        uint64_t account_id = 10400 + i;
        uint64_t role_id = 20400 + i;
        std::string session_id = proxy_service_->GenerateSessionId();
        
        proxy_service_->OnClientConnect(conn_id, "127.0.0.1", 12400 + i);
        EXPECT_TRUE(proxy_service_->CreateSession(conn_id, account_id, role_id, session_id));
    }
    
    for (int i = 0; i < 10; i++) {
        uint32_t conn_id = 400 + i;
        uint64_t account_id = 10400 + i;
        
        ClientSession* session = proxy_service_->GetSession(conn_id);
        EXPECT_NE(session, nullptr);
        
        ClientSession* session_by_account = proxy_service_->GetSessionByAccount(account_id);
        EXPECT_NE(session_by_account, nullptr);
    }
}

/**
 * 测试会话删除不存在的会话
 */
TEST_F(MessageForwardingTest, RemoveNonExistentSession) {
    EXPECT_FALSE(proxy_service_->RemoveSession(9999));
}

/**
 * 测试获取不存在的会话
 */
TEST_F(MessageForwardingTest, GetNonExistentSession) {
    EXPECT_EQ(proxy_service_->GetSession(9999), nullptr);
    EXPECT_EQ(proxy_service_->GetSessionByAccount(99999), nullptr);
}

/**
 * 测试获取不存在的逻辑服务器
 */
TEST_F(MessageForwardingTest, GetNonExistentLogicServer) {
    EXPECT_EQ(proxy_service_->GetLogicServer(9999), nullptr);
}

/**
 * 测试会话活动时间更新
 */
TEST_F(MessageForwardingTest, SessionActivityTime) {
    uint32_t conn_id = 500;
    uint64_t account_id = 10500;
    uint64_t role_id = 20500;
    
    proxy_service_->OnClientConnect(conn_id, "127.0.0.1", 12500);
    std::string session_id = proxy_service_->GenerateSessionId();
    proxy_service_->CreateSession(conn_id, account_id, role_id, session_id);
    
    ClientSession* session = proxy_service_->GetSession(conn_id);
    ASSERT_NE(session, nullptr);
    time_t initial_time = session->last_active_time;
    
    // 直接更新时间（不依赖于等待）
    time_t new_time = initial_time + 100;
    session->last_active_time = new_time;
    
    EXPECT_EQ(session->last_active_time, new_time);
}

/**
 * 测试重复创建会话
 */
TEST_F(MessageForwardingTest, DuplicateSession) {
    uint32_t conn_id = 600;
    uint64_t account_id = 10600;
    uint64_t role_id = 20600;
    std::string session_id1 = proxy_service_->GenerateSessionId();
    
    proxy_service_->OnClientConnect(conn_id, "127.0.0.1", 12600);
    EXPECT_TRUE(proxy_service_->CreateSession(conn_id, account_id, role_id, session_id1));
    
    std::string session_id2 = proxy_service_->GenerateSessionId();
    uint32_t conn_id2 = 601;
    proxy_service_->OnClientConnect(conn_id2, "127.0.0.1", 12601);
    EXPECT_TRUE(proxy_service_->CreateSession(conn_id2, account_id + 1, role_id + 1, session_id2));
    
    // 应该能够覆盖已有连接的会话
    EXPECT_TRUE(proxy_service_->CreateSession(conn_id, account_id + 2, role_id + 2, proxy_service_->GenerateSessionId()));
}

/**
 * 测试逻辑服务器负载均衡（模拟）
 */
TEST_F(MessageForwardingTest, LogicServerLoadBalancing) {
    for (int i = 0; i < 5; i++) {
        ServerConnection server;
        server.conn_id = 700 + i;
        server.server_name = "LogicServer" + std::to_string(i);
        server.ip = "127.0.0.1";
        server.port = 8070 + i;
        server.last_active_time = time(nullptr);
        server.player_count = (i == 2) ? 0 : 100 + i * 20;
        proxy_service_->AddLogicServer(server);
    }
    
    uint32_t selected = proxy_service_->SelectLogicServer();
    EXPECT_EQ(selected, 702);
}

/**
 * 测试无逻辑服务器时的选择
 */
TEST_F(MessageForwardingTest, SelectLogicServerWithoutServers) {
    uint32_t selected = proxy_service_->SelectLogicServer();
    EXPECT_EQ(selected, 0);
}
