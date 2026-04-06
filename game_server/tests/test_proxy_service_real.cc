#include <gtest/gtest.h>
#include "proxy_server/proxy_service.h"

using namespace game_server;

class ProxyServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        proxy_service_ = new ProxyService();
    }
    
    void TearDown() override {
        delete proxy_service_;
    }
    
    ProxyService* proxy_service_;
};

TEST_F(ProxyServiceTest, InitService) {
    EXPECT_TRUE(proxy_service_->InitService());
}

TEST_F(ProxyServiceTest, GenerateSessionId) {
    std::string session_id1 = proxy_service_->GenerateSessionId();
    std::string session_id2 = proxy_service_->GenerateSessionId();
    EXPECT_NE(session_id1, session_id2);
    EXPECT_GT(session_id1.length(), 0);
}

TEST_F(ProxyServiceTest, CreateAndRemoveSession) {
    uint32_t conn_id = 1;
    uint64_t account_id = 12345;
    uint64_t role_id = 67890;
    std::string session_id = "test_session";
    
    // 创建会话
    EXPECT_TRUE(proxy_service_->CreateSession(conn_id, account_id, role_id, session_id));
    
    // 获取会话
    ClientSession* session = proxy_service_->GetSession(conn_id);
    EXPECT_TRUE(session != nullptr);
    EXPECT_EQ(session->conn_id, conn_id);
    EXPECT_EQ(session->account_id, account_id);
    EXPECT_EQ(session->role_id, role_id);
    EXPECT_EQ(session->session_id, session_id);
    
    // 通过账号获取会话
    session = proxy_service_->GetSessionByAccount(account_id);
    EXPECT_TRUE(session != nullptr);
    EXPECT_EQ(session->conn_id, conn_id);
    
    // 删除会话
    EXPECT_TRUE(proxy_service_->RemoveSession(conn_id));
    
    // 确认会话已删除
    session = proxy_service_->GetSession(conn_id);
    EXPECT_TRUE(session == nullptr);
    
    session = proxy_service_->GetSessionByAccount(account_id);
    EXPECT_TRUE(session == nullptr);
}

TEST_F(ProxyServiceTest, AddAndRemoveLogicServer) {
    ServerConnection server;
    server.conn_id = 1001;
    server.server_name = "LogicServer1";
    server.ip = "127.0.0.1";
    server.port = 8001;
    server.last_active_time = time(nullptr);
    server.player_count = 0;
    
    // 添加逻辑服务器
    proxy_service_->AddLogicServer(server);
    
    // 获取逻辑服务器
    ServerConnection* found_server = proxy_service_->GetLogicServer(server.conn_id);
    EXPECT_TRUE(found_server != nullptr);
    EXPECT_EQ(found_server->conn_id, server.conn_id);
    EXPECT_EQ(found_server->server_name, server.server_name);
    
    // 移除逻辑服务器
    proxy_service_->RemoveLogicServer(server.conn_id);
    
    // 确认服务器已移除
    found_server = proxy_service_->GetLogicServer(server.conn_id);
    EXPECT_TRUE(found_server == nullptr);
}

TEST_F(ProxyServiceTest, SelectLogicServer) {
    // 添加几个逻辑服务器
    for (int i = 1; i <= 3; ++i) {
        ServerConnection server;
        server.conn_id = 1000 + i;
        server.server_name = "LogicServer" + std::to_string(i);
        server.ip = "127.0.0.1";
        server.port = 8000 + i;
        server.last_active_time = time(nullptr);
        server.player_count = i * 10;
        proxy_service_->AddLogicServer(server);
    }
    
    // 测试选择逻辑服务器
    uint32_t selected_server = proxy_service_->SelectLogicServer();
    EXPECT_GT(selected_server, 0);
}
