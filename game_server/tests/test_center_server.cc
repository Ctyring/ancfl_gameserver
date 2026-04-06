#include <gtest/gtest.h>
#include "center_server/center_server.h"

using namespace game_server;

class CenterServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        center_server_ = new CenterServer();
    }
    
    void TearDown() override {
        delete center_server_;
    }
    
    CenterServer* center_server_;
};

TEST_F(CenterServerTest, Init) {
    EXPECT_TRUE(center_server_->Init("config/center_server.yaml"));
}

TEST_F(CenterServerTest, StartStop) {
    center_server_->Init("config/center_server.yaml");
    EXPECT_TRUE(center_server_->Start());
    center_server_->Stop();
}

TEST_F(CenterServerTest, RegisterServer) {
    center_server_->Init("config/center_server.yaml");
    
    ServerInfo info;
    info.server_id = 1;
    info.type = ServerType::LOGIC;
    info.ip = "127.0.0.1";
    info.port = 8001;
    info.status = ServerStatus::RUNNING;
    info.online_count = 0;
    info.max_online = 1000;
    
    EXPECT_TRUE(center_server_->RegisterServer(info));
}

TEST_F(CenterServerTest, UnregisterServer) {
    center_server_->Init("config/center_server.yaml");
    
    ServerInfo info;
    info.server_id = 1;
    info.type = ServerType::LOGIC;
    info.ip = "127.0.0.1";
    info.port = 8001;
    
    center_server_->RegisterServer(info);
    EXPECT_TRUE(center_server_->UnregisterServer(1));
}

TEST_F(CenterServerTest, GetServerInfo) {
    center_server_->Init("config/center_server.yaml");
    
    ServerInfo info;
    info.server_id = 1;
    info.type = ServerType::LOGIC;
    info.ip = "127.0.0.1";
    info.port = 8001;
    center_server_->RegisterServer(info);
    
    ServerInfo retrieved;
    EXPECT_TRUE(center_server_->GetServerInfo(1, retrieved));
    EXPECT_EQ(retrieved.server_id, 1);
}

TEST_F(CenterServerTest, GetServerList) {
    center_server_->Init("config/center_server.yaml");
    
    ServerInfo info1;
    info1.server_id = 1;
    info1.type = ServerType::LOGIC;
    info1.ip = "127.0.0.1";
    info1.port = 8001;
    center_server_->RegisterServer(info1);
    
    ServerInfo info2;
    info2.server_id = 2;
    info2.type = ServerType::LOGIC;
    info2.ip = "127.0.0.1";
    info2.port = 8002;
    center_server_->RegisterServer(info2);
    
    std::vector<ServerInfo> servers;
    EXPECT_TRUE(center_server_->GetServerList(ServerType::LOGIC, servers));
    EXPECT_EQ(servers.size(), 2);
}

TEST_F(CenterServerTest, UpdateServerLoad) {
    center_server_->Init("config/center_server.yaml");
    
    ServerInfo info;
    info.server_id = 1;
    info.type = ServerType::LOGIC;
    info.ip = "127.0.0.1";
    info.port = 8001;
    center_server_->RegisterServer(info);
    
    EXPECT_TRUE(center_server_->UpdateServerLoad(1, 50));
    EXPECT_EQ(center_server_->GetServerLoad(1), 50);
}

TEST_F(CenterServerTest, SelectBestServer) {
    center_server_->Init("config/center_server.yaml");
    
    ServerInfo info1;
    info1.server_id = 1;
    info1.type = ServerType::LOGIC;
    info1.ip = "127.0.0.1";
    info1.port = 8001;
    center_server_->RegisterServer(info1);
    center_server_->UpdateServerLoad(1, 80);
    
    ServerInfo info2;
    info2.server_id = 2;
    info2.type = ServerType::LOGIC;
    info2.ip = "127.0.0.1";
    info2.port = 8002;
    center_server_->RegisterServer(info2);
    center_server_->UpdateServerLoad(2, 30);
    
    int32_t server_id = center_server_->SelectBestServer(ServerType::LOGIC);
    EXPECT_EQ(server_id, 2);
}

TEST_F(CenterServerTest, PlayerEnterCross) {
    center_server_->Init("config/center_server.yaml");
    
    EXPECT_TRUE(center_server_->PlayerEnterCross(12345, 1, 2, (int32_t)CrossType::CROSS_BATTLE, "token123"));
}

TEST_F(CenterServerTest, PlayerLeaveCross) {
    center_server_->Init("config/center_server.yaml");
    center_server_->PlayerEnterCross(12345, 1, 2, (int32_t)CrossType::CROSS_BATTLE, "token123");
    
    EXPECT_TRUE(center_server_->PlayerLeaveCross(12345));
}

TEST_F(CenterServerTest, IsPlayerInCross) {
    center_server_->Init("config/center_server.yaml");
    center_server_->PlayerEnterCross(12345, 1, 2, (int32_t)CrossType::CROSS_BATTLE, "token123");
    
    EXPECT_TRUE(center_server_->IsPlayerInCross(12345));
}

TEST_F(CenterServerTest, AddToMatchQueue) {
    center_server_->Init("config/center_server.yaml");
    
    EXPECT_TRUE(center_server_->AddToMatchQueue(12345, 1, 1000));
}

TEST_F(CenterServerTest, RemoveFromMatchQueue) {
    center_server_->Init("config/center_server.yaml");
    center_server_->AddToMatchQueue(12345, 1, 1000);
    
    EXPECT_TRUE(center_server_->RemoveFromMatchQueue(12345));
}
