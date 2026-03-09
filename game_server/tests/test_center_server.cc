#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "center_server/center_server.h"

using namespace game_server;
using namespace testing;

class MockCenterCallback : public CenterCallback {
public:
    MOCK_METHOD(void, OnServerRegister, (int32_t server_id, const std::string& server_type), (override));
    MOCK_METHOD(void, OnServerUnregister, (int32_t server_id), (override));
    MOCK_METHOD(void, OnCrossBattleStart, (int64_t battle_id), (override));
    MOCK_METHOD(void, OnCrossBattleEnd, (int64_t battle_id), (override));
};

class CenterServerTest : public Test {
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
    EXPECT_TRUE(center_server_->Init("0.0.0.0", 9999));
}

TEST_F(CenterServerTest, StartStop) {
    center_server_->Init("0.0.0.0", 9999);
    EXPECT_TRUE(center_server_->Start());
    center_server_->Stop();
}

TEST_F(CenterServerTest, RegisterServer) {
    center_server_->Init("0.0.0.0", 9999);
    
    EXPECT_TRUE(center_server_->RegisterServer(1, "logic", "127.0.0.1", 8001));
    EXPECT_TRUE(center_server_->IsServerRegistered(1));
}

TEST_F(CenterServerTest, UnregisterServer) {
    center_server_->Init("0.0.0.0", 9999);
    center_server_->RegisterServer(1, "logic", "127.0.0.1", 8001);
    
    EXPECT_TRUE(center_server_->UnregisterServer(1));
    EXPECT_FALSE(center_server_->IsServerRegistered(1));
}

TEST_F(CenterServerTest, GetServerList) {
    center_server_->Init("0.0.0.0", 9999);
    center_server_->RegisterServer(1, "logic", "127.0.0.1", 8001);
    center_server_->RegisterServer(2, "logic", "127.0.0.1", 8002);
    
    std::vector<ServerInfo> servers;
    EXPECT_TRUE(center_server_->GetServerList("logic", servers));
    EXPECT_EQ(servers.size(), 2);
}

TEST_F(CenterServerTest, GetServerInfo) {
    center_server_->Init("0.0.0.0", 9999);
    center_server_->RegisterServer(1, "logic", "127.0.0.1", 8001);
    
    ServerInfo info;
    EXPECT_TRUE(center_server_->GetServerInfo(1, info));
    EXPECT_EQ(info.server_id, 1);
    EXPECT_EQ(info.server_type, "logic");
}

TEST_F(CenterServerTest, UpdateServerLoad) {
    center_server_->Init("0.0.0.0", 9999);
    center_server_->RegisterServer(1, "logic", "127.0.0.1", 8001);
    
    EXPECT_TRUE(center_server_->UpdateServerLoad(1, 50));
    
    ServerInfo info;
    center_server_->GetServerInfo(1, info);
    EXPECT_EQ(info.load, 50);
}

TEST_F(CenterServerTest, SelectLowestLoadServer) {
    center_server_->Init("0.0.0.0", 9999);
    center_server_->RegisterServer(1, "logic", "127.0.0.1", 8001);
    center_server_->RegisterServer(2, "logic", "127.0.0.1", 8002);
    center_server_->UpdateServerLoad(1, 80);
    center_server_->UpdateServerLoad(2, 30);
    
    int32_t server_id = 0;
    EXPECT_TRUE(center_server_->SelectLowestLoadServer("logic", server_id));
    EXPECT_EQ(server_id, 2);
}

TEST_F(CenterServerTest, CreateCrossBattle) {
    center_server_->Init("0.0.0.0", 9999);
    
    CrossBattleInfo battle;
    battle.battle_type = 1;
    battle.server_ids = {1, 2, 3};
    
    int64_t battle_id = 0;
    EXPECT_TRUE(center_server_->CreateCrossBattle(battle, battle_id));
    EXPECT_NE(battle_id, 0);
}

TEST_F(CenterServerTest, JoinCrossBattle) {
    center_server_->Init("0.0.0.0", 9999);
    
    CrossBattleInfo battle;
    battle.battle_type = 1;
    battle.server_ids = {1, 2, 3};
    
    int64_t battle_id = 0;
    center_server_->CreateCrossBattle(battle, battle_id);
    
    EXPECT_TRUE(center_server_->JoinCrossBattle(battle_id, 12345));
}

TEST_F(CenterServerTest, GetCrossBattleInfo) {
    center_server_->Init("0.0.0.0", 9999);
    
    CrossBattleInfo battle;
    battle.battle_type = 1;
    battle.server_ids = {1, 2, 3};
    
    int64_t battle_id = 0;
    center_server_->CreateCrossBattle(battle, battle_id);
    
    CrossBattleInfo info;
    EXPECT_TRUE(center_server_->GetCrossBattleInfo(battle_id, info));
    EXPECT_EQ(info.battle_type, 1);
}

TEST_F(CenterServerTest, EndCrossBattle) {
    center_server_->Init("0.0.0.0", 9999);
    
    CrossBattleInfo battle;
    battle.battle_type = 1;
    battle.server_ids = {1, 2, 3};
    
    int64_t battle_id = 0;
    center_server_->CreateCrossBattle(battle, battle_id);
    
    EXPECT_TRUE(center_server_->EndCrossBattle(battle_id));
}
