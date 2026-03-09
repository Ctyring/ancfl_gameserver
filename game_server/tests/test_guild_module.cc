#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "logic_server/guild_module.h"

using namespace game_server;
using namespace testing;

class MockLogicServiceForGuild : public LogicService {
public:
    MOCK_METHOD(bool, SendToClient, (uint64_t role_id, int32_t msg_id, const std::string& data), (override));
    MOCK_METHOD(bool, SendToDB, (int32_t msg_id, const std::string& data), (override));
    MOCK_METHOD(bool, BroadcastToGuild, (uint64_t guild_id, int32_t msg_id, const std::string& data), (override));
};

class GuildModuleTest : public Test {
protected:
    void SetUp() override {
        mock_service_ = new MockLogicServiceForGuild();
        guild_module_ = new GuildModule(mock_service_);
        test_role_id_ = 12345;
        target_role_id_ = 67890;
    }
    
    void TearDown() override {
        delete guild_module_;
        delete mock_service_;
    }
    
    MockLogicServiceForGuild* mock_service_;
    GuildModule* guild_module_;
    uint64_t test_role_id_;
    uint64_t target_role_id_;
};

TEST_F(GuildModuleTest, CreateGuild) {
    uint64_t guild_id = 0;
    EXPECT_TRUE(guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id));
    EXPECT_NE(guild_id, 0);
}

TEST_F(GuildModuleTest, CreateDuplicateGuildName) {
    uint64_t guild_id1 = 0;
    uint64_t guild_id2 = 0;
    
    guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id1);
    EXPECT_FALSE(guild_module_->CreateGuild(target_role_id_, "TestGuild", guild_id2));
}

TEST_F(GuildModuleTest, DismissGuild) {
    uint64_t guild_id = 0;
    guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id);
    
    EXPECT_TRUE(guild_module_->DismissGuild(test_role_id_));
}

TEST_F(GuildModuleTest, GetGuildInfo) {
    uint64_t guild_id = 0;
    guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id);
    
    GuildInfo info;
    EXPECT_TRUE(guild_module_->GetGuildInfo(guild_id, info));
    EXPECT_EQ(info.guild_name, "TestGuild");
}

TEST_F(GuildModuleTest, ApplyJoinGuild) {
    uint64_t guild_id = 0;
    guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id);
    
    EXPECT_TRUE(guild_module_->ApplyJoinGuild(target_role_id_, guild_id, "I want to join"));
}

TEST_F(GuildModuleTest, HandleApply) {
    uint64_t guild_id = 0;
    guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id);
    guild_module_->ApplyJoinGuild(target_role_id_, guild_id, "I want to join");
    
    std::vector<GuildApplyInfo> applies;
    guild_module_->GetGuildApplies(guild_id, applies);
    
    if (!applies.empty()) {
        EXPECT_TRUE(guild_module_->HandleApply(guild_id, applies[0].apply_id, true));
    }
}

TEST_F(GuildModuleTest, LeaveGuild) {
    uint64_t guild_id = 0;
    guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id);
    guild_module_->ApplyJoinGuild(target_role_id_, guild_id, "");
    
    std::vector<GuildApplyInfo> applies;
    guild_module_->GetGuildApplies(guild_id, applies);
    if (!applies.empty()) {
        guild_module_->HandleApply(guild_id, applies[0].apply_id, true);
    }
    
    EXPECT_TRUE(guild_module_->LeaveGuild(target_role_id_));
}

TEST_F(GuildModuleTest, KickMember) {
    uint64_t guild_id = 0;
    guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id);
    guild_module_->ApplyJoinGuild(target_role_id_, guild_id, "");
    
    std::vector<GuildApplyInfo> applies;
    guild_module_->GetGuildApplies(guild_id, applies);
    if (!applies.empty()) {
        guild_module_->HandleApply(guild_id, applies[0].apply_id, true);
    }
    
    EXPECT_TRUE(guild_module_->KickMember(test_role_id_, target_role_id_));
}

TEST_F(GuildModuleTest, SetMemberPosition) {
    uint64_t guild_id = 0;
    guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id);
    guild_module_->ApplyJoinGuild(target_role_id_, guild_id, "");
    
    std::vector<GuildApplyInfo> applies;
    guild_module_->GetGuildApplies(guild_id, applies);
    if (!applies.empty()) {
        guild_module_->HandleApply(guild_id, applies[0].apply_id, true);
    }
    
    EXPECT_TRUE(guild_module_->SetMemberPosition(test_role_id_, target_role_id_, GuildPosition::ELITE));
}

TEST_F(GuildModuleTest, AddContribution) {
    uint64_t guild_id = 0;
    guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id);
    
    EXPECT_TRUE(guild_module_->AddContribution(test_role_id_, 100));
    
    int32_t contribution = 0;
    guild_module_->GetContribution(test_role_id_, contribution);
    EXPECT_EQ(contribution, 100);
}

TEST_F(GuildModuleTest, SetAnnouncement) {
    uint64_t guild_id = 0;
    guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id);
    
    EXPECT_TRUE(guild_module_->SetAnnouncement(test_role_id_, "Welcome to TestGuild!"));
    
    std::string announcement;
    guild_module_->GetAnnouncement(guild_id, announcement);
    EXPECT_EQ(announcement, "Welcome to TestGuild!");
}

TEST_F(GuildModuleTest, GetGuildMembers) {
    uint64_t guild_id = 0;
    guild_module_->CreateGuild(test_role_id_, "TestGuild", guild_id);
    
    std::vector<GuildMember> members;
    EXPECT_TRUE(guild_module_->GetGuildMembers(guild_id, members));
    EXPECT_EQ(members.size(), 1);
}
