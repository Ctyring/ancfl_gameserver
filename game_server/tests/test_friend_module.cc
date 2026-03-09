#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "logic_server/friend_module.h"

using namespace game_server;
using namespace testing;

class MockLogicServiceForFriend : public LogicService {
public:
    MOCK_METHOD(bool, SendToClient, (uint64_t role_id, int32_t msg_id, const std::string& data), (override));
    MOCK_METHOD(bool, SendToDB, (int32_t msg_id, const std::string& data), (override));
};

class FriendModuleTest : public Test {
protected:
    void SetUp() override {
        mock_service_ = new MockLogicServiceForFriend();
        friend_module_ = new FriendModule(mock_service_);
        test_role_id_ = 12345;
        target_role_id_ = 67890;
    }
    
    void TearDown() override {
        delete friend_module_;
        delete mock_service_;
    }
    
    MockLogicServiceForFriend* mock_service_;
    FriendModule* friend_module_;
    uint64_t test_role_id_;
    uint64_t target_role_id_;
};

TEST_F(FriendModuleTest, InitFriend) {
    EXPECT_TRUE(friend_module_->InitFriend(test_role_id_));
}

TEST_F(FriendModuleTest, AddFriend) {
    friend_module_->InitFriend(test_role_id_);
    
    EXPECT_TRUE(friend_module_->AddFriend(test_role_id_, target_role_id_));
    EXPECT_TRUE(friend_module_->IsFriend(test_role_id_, target_role_id_));
}

TEST_F(FriendModuleTest, AddDuplicateFriend) {
    friend_module_->InitFriend(test_role_id_);
    friend_module_->AddFriend(test_role_id_, target_role_id_);
    
    EXPECT_FALSE(friend_module_->AddFriend(test_role_id_, target_role_id_));
}

TEST_F(FriendModuleTest, DeleteFriend) {
    friend_module_->InitFriend(test_role_id_);
    friend_module_->AddFriend(test_role_id_, target_role_id_);
    
    EXPECT_TRUE(friend_module_->DeleteFriend(test_role_id_, target_role_id_));
    EXPECT_FALSE(friend_module_->IsFriend(test_role_id_, target_role_id_));
}

TEST_F(FriendModuleTest, GetFriendList) {
    friend_module_->InitFriend(test_role_id_);
    friend_module_->AddFriend(test_role_id_, 1001);
    friend_module_->AddFriend(test_role_id_, 1002);
    friend_module_->AddFriend(test_role_id_, 1003);
    
    std::vector<FriendInfo> friends;
    EXPECT_TRUE(friend_module_->GetFriendList(test_role_id_, friends));
    EXPECT_EQ(friends.size(), 3);
}

TEST_F(FriendModuleTest, AddToBlacklist) {
    friend_module_->InitFriend(test_role_id_);
    
    EXPECT_TRUE(friend_module_->AddToBlacklist(test_role_id_, target_role_id_));
    EXPECT_TRUE(friend_module_->IsInBlacklist(test_role_id_, target_role_id_));
}

TEST_F(FriendModuleTest, RemoveFromBlacklist) {
    friend_module_->InitFriend(test_role_id_);
    friend_module_->AddToBlacklist(test_role_id_, target_role_id_);
    
    EXPECT_TRUE(friend_module_->RemoveFromBlacklist(test_role_id_, target_role_id_));
    EXPECT_FALSE(friend_module_->IsInBlacklist(test_role_id_, target_role_id_));
}

TEST_F(FriendModuleTest, GetBlacklist) {
    friend_module_->InitFriend(test_role_id_);
    friend_module_->AddToBlacklist(test_role_id_, 1001);
    friend_module_->AddToBlacklist(test_role_id_, 1002);
    
    std::vector<uint64_t> blacklist;
    EXPECT_TRUE(friend_module_->GetBlacklist(test_role_id_, blacklist));
    EXPECT_EQ(blacklist.size(), 2);
}

TEST_F(FriendModuleTest, SetRemark) {
    friend_module_->InitFriend(test_role_id_);
    friend_module_->AddFriend(test_role_id_, target_role_id_);
    
    EXPECT_TRUE(friend_module_->SetRemark(test_role_id_, target_role_id_, "Best Friend"));
    
    FriendInfo info;
    friend_module_->GetFriendInfo(test_role_id_, target_role_id_, info);
    EXPECT_EQ(info.remark, "Best Friend");
}

TEST_F(FriendModuleTest, AddIntimacy) {
    friend_module_->InitFriend(test_role_id_);
    friend_module_->AddFriend(test_role_id_, target_role_id_);
    
    EXPECT_TRUE(friend_module_->AddIntimacy(test_role_id_, target_role_id_, 10));
    
    FriendInfo info;
    friend_module_->GetFriendInfo(test_role_id_, target_role_id_, info);
    EXPECT_EQ(info.intimacy, 10);
}

TEST_F(FriendModuleTest, UpdateOnlineStatus) {
    friend_module_->InitFriend(test_role_id_);
    friend_module_->AddFriend(test_role_id_, target_role_id_);
    
    EXPECT_TRUE(friend_module_->UpdateOnlineStatus(target_role_id_, true));
    EXPECT_TRUE(friend_module_->UpdateOnlineStatus(target_role_id_, false));
}
