#include <gtest/gtest.h>
#include "logic_server/friend_module.h"
#include "logic_server/logic_service.h"

using namespace game_server;

class FriendModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        service_ = nullptr;
        friend_module_ = new FriendModule(service_);
        test_role_id_ = 12345;
        target_role_id_ = 67890;
    }
    
    void TearDown() override {
        delete friend_module_;
    }
    
    LogicService* service_;
    FriendModule* friend_module_;
    uint64_t test_role_id_;
    uint64_t target_role_id_;
};

TEST_F(FriendModuleTest, InitFriends) {
    EXPECT_TRUE(friend_module_->InitFriends(test_role_id_));
}

TEST_F(FriendModuleTest, AddFriend) {
    friend_module_->InitFriends(test_role_id_);
    friend_module_->InitFriends(target_role_id_);
    
    EXPECT_TRUE(friend_module_->AddFriend(test_role_id_, target_role_id_));
}

TEST_F(FriendModuleTest, RemoveFriend) {
    friend_module_->InitFriends(test_role_id_);
    friend_module_->InitFriends(target_role_id_);
    friend_module_->AddFriend(test_role_id_, target_role_id_);
    
    EXPECT_TRUE(friend_module_->RemoveFriend(test_role_id_, target_role_id_));
}

TEST_F(FriendModuleTest, GetFriends) {
    friend_module_->InitFriends(test_role_id_);
    friend_module_->InitFriends(target_role_id_);
    friend_module_->AddFriend(test_role_id_, target_role_id_);
    
    std::vector<FriendInfo> friends;
    EXPECT_TRUE(friend_module_->GetFriends(test_role_id_, friends));
    EXPECT_EQ(friends.size(), 1);
}

TEST_F(FriendModuleTest, GetFriendInfo) {
    friend_module_->InitFriends(test_role_id_);
    friend_module_->InitFriends(target_role_id_);
    friend_module_->AddFriend(test_role_id_, target_role_id_);
    
    FriendInfo info;
    EXPECT_TRUE(friend_module_->GetFriendInfo(test_role_id_, target_role_id_, info));
    EXPECT_EQ(info.friend_id, target_role_id_);
}

TEST_F(FriendModuleTest, SendFriendRequest) {
    friend_module_->InitFriends(test_role_id_);
    friend_module_->InitFriends(target_role_id_);
    
    EXPECT_TRUE(friend_module_->SendFriendRequest(test_role_id_, target_role_id_, "Please be my friend"));
}

TEST_F(FriendModuleTest, GetFriendRequests) {
    friend_module_->InitFriends(test_role_id_);
    friend_module_->InitFriends(target_role_id_);
    friend_module_->SendFriendRequest(test_role_id_, target_role_id_, "Hello");
    
    std::vector<FriendApplyInfo> requests;
    EXPECT_TRUE(friend_module_->GetFriendRequests(target_role_id_, requests));
}

TEST_F(FriendModuleTest, AddRecentPlayer) {
    friend_module_->InitFriends(test_role_id_);
    
    EXPECT_TRUE(friend_module_->AddRecentPlayer(test_role_id_, target_role_id_, "PlayerName", 10, 1));
}

TEST_F(FriendModuleTest, GetRecentPlayers) {
    friend_module_->InitFriends(test_role_id_);
    friend_module_->AddRecentPlayer(test_role_id_, target_role_id_, "PlayerName", 10, 1);
    
    std::vector<FriendInfo> players;
    EXPECT_TRUE(friend_module_->GetRecentPlayers(test_role_id_, players));
    EXPECT_EQ(players.size(), 1);
}

TEST_F(FriendModuleTest, UpdateFriendStatus) {
    friend_module_->InitFriends(test_role_id_);
    friend_module_->InitFriends(target_role_id_);
    friend_module_->AddFriend(test_role_id_, target_role_id_);
    
    EXPECT_TRUE(friend_module_->UpdateFriendStatus(test_role_id_, target_role_id_, FriendStatus::ONLINE));
    EXPECT_TRUE(friend_module_->UpdateFriendStatus(test_role_id_, target_role_id_, FriendStatus::OFFLINE));
}
