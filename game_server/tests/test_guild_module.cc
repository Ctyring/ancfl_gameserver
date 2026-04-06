#include <gtest/gtest.h>
#include "logic_server/guild_module.h"
#include "logic_server/logic_service.h"

using namespace game_server;

class GuildModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        service_ = nullptr;
        guild_module_ = new GuildModule(service_);
        test_role_id_ = 12345;
        target_role_id_ = 67890;
    }
    
    void TearDown() override {
        delete guild_module_;
    }
    
    LogicService* service_;
    GuildModule* guild_module_;
    uint64_t test_role_id_;
    uint64_t target_role_id_;
};

TEST_F(GuildModuleTest, CreateGuild) {
    uint64_t guild_id = 0;
    EXPECT_TRUE(guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id));
    EXPECT_NE(guild_id, 0);
}

TEST_F(GuildModuleTest, JoinGuild) {
    uint64_t guild_id = 0;
    guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id);
    
    EXPECT_TRUE(guild_module_->JoinGuild(target_role_id_, guild_id));
}

TEST_F(GuildModuleTest, LeaveGuild) {
    uint64_t guild_id = 0;
    guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id);
    guild_module_->JoinGuild(target_role_id_, guild_id);
    
    EXPECT_TRUE(guild_module_->LeaveGuild(target_role_id_));
}

TEST_F(GuildModuleTest, GetGuildInfo) {
    uint64_t guild_id = 0;
    guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id);
    
    GuildInfo info;
    EXPECT_TRUE(guild_module_->GetGuildInfo(guild_id, info));
    EXPECT_EQ(info.guild_name, "TestGuild");
    EXPECT_EQ(info.leader_id, test_role_id_);
}

TEST_F(GuildModuleTest, GetGuildMembers) {
    uint64_t guild_id = 0;
    guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id);
    guild_module_->JoinGuild(target_role_id_, guild_id);
    
    std::vector<GuildMemberInfo> members;
    EXPECT_TRUE(guild_module_->GetGuildMembers(guild_id, members));
    EXPECT_EQ(members.size(), 2);
}

TEST_F(GuildModuleTest, GetGuildMemberInfo) {
    uint64_t guild_id = 0;
    guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id);
    
    GuildMemberInfo info;
    EXPECT_TRUE(guild_module_->GetGuildMemberInfo(test_role_id_, info));
    EXPECT_EQ(info.role_id, test_role_id_);
    EXPECT_EQ(info.position, GuildPosition::LEADER);
}

TEST_F(GuildModuleTest, IsInGuild) {
    uint64_t guild_id = 0;
    guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id);
    
    EXPECT_TRUE(guild_module_->IsInGuild(test_role_id_));
    EXPECT_FALSE(guild_module_->IsInGuild(target_role_id_));
}

TEST_F(GuildModuleTest, Contribute) {
    uint64_t guild_id = 0;
    guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id);
    
    EXPECT_TRUE(guild_module_->Contribute(test_role_id_, 1, 100));
}

TEST_F(GuildModuleTest, UpdateAnnouncement) {
    uint64_t guild_id = 0;
    guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id);
    
    EXPECT_TRUE(guild_module_->UpdateAnnouncement(test_role_id_, "Welcome to TestGuild!"));
}

TEST_F(GuildModuleTest, UpgradeGuild) {
    uint64_t guild_id = 0;
    guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id);
    
    EXPECT_TRUE(guild_module_->UpgradeGuild(test_role_id_));
}

TEST_F(GuildModuleTest, KickMember) {
    uint64_t guild_id = 0;
    guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id);
    guild_module_->JoinGuild(target_role_id_, guild_id);
    
    EXPECT_TRUE(guild_module_->KickMember(test_role_id_, target_role_id_));
}

TEST_F(GuildModuleTest, DissolveGuild) {
    uint64_t guild_id = 0;
    guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id);
    
    EXPECT_TRUE(guild_module_->DissolveGuild(test_role_id_));
}
