#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "logic_server/activity_module.h"

using namespace game_server;
using namespace testing;

class MockLogicServiceForActivity : public LogicService {
public:
    MOCK_METHOD(bool, SendToClient, (uint64_t role_id, int32_t msg_id, const std::string& data), (override));
    MOCK_METHOD(bool, SendToDB, (int32_t msg_id, const std::string& data), (override));
};

class ActivityModuleTest : public Test {
protected:
    void SetUp() override {
        mock_service_ = new MockLogicServiceForActivity();
        activity_module_ = new ActivityModule(mock_service_);
        test_role_id_ = 12345;
    }
    
    void TearDown() override {
        delete activity_module_;
        delete mock_service_;
    }
    
    MockLogicServiceForActivity* mock_service_;
    ActivityModule* activity_module_;
    uint64_t test_role_id_;
};

TEST_F(ActivityModuleTest, InitActivity) {
    EXPECT_TRUE(activity_module_->InitActivity(test_role_id_));
}

TEST_F(ActivityModuleTest, GetActivityList) {
    activity_module_->InitActivity(test_role_id_);
    
    std::vector<ActivityInfo> activities;
    EXPECT_TRUE(activity_module_->GetActivityList(activities));
}

TEST_F(ActivityModuleTest, JoinActivity) {
    activity_module_->InitActivity(test_role_id_);
    
    EXPECT_TRUE(activity_module_->JoinActivity(test_role_id_, 1001));
    EXPECT_TRUE(activity_module_->IsInActivity(test_role_id_, 1001));
}

TEST_F(ActivityModuleTest, LeaveActivity) {
    activity_module_->InitActivity(test_role_id_);
    activity_module_->JoinActivity(test_role_id_, 1001);
    
    EXPECT_TRUE(activity_module_->LeaveActivity(test_role_id_, 1001));
    EXPECT_FALSE(activity_module_->IsInActivity(test_role_id_, 1001));
}

TEST_F(ActivityModuleTest, UpdateProgress) {
    activity_module_->InitActivity(test_role_id_);
    activity_module_->JoinActivity(test_role_id_, 1001);
    
    EXPECT_TRUE(activity_module_->UpdateProgress(test_role_id_, 1001, 50));
    
    int32_t progress = 0;
    activity_module_->GetProgress(test_role_id_, 1001, progress);
    EXPECT_EQ(progress, 50);
}

TEST_F(ActivityModuleTest, GetReward) {
    activity_module_->InitActivity(test_role_id_);
    activity_module_->JoinActivity(test_role_id_, 1001);
    activity_module_->UpdateProgress(test_role_id_, 1001, 100);
    
    EXPECT_TRUE(activity_module_->GetReward(test_role_id_, 1001, 1));
}

TEST_F(ActivityModuleTest, GetRewardNotEnoughProgress) {
    activity_module_->InitActivity(test_role_id_);
    activity_module_->JoinActivity(test_role_id_, 1001);
    activity_module_->UpdateProgress(test_role_id_, 1001, 10);
    
    EXPECT_FALSE(activity_module_->GetReward(test_role_id_, 1001, 1));
}

TEST_F(ActivityModuleTest, GetActivityInfo) {
    activity_module_->InitActivity(test_role_id_);
    activity_module_->JoinActivity(test_role_id_, 1001);
    
    ActivityInfo info;
    EXPECT_TRUE(activity_module_->GetActivityInfo(test_role_id_, 1001, info));
}

TEST_F(ActivityModuleTest, CheckActivityTime) {
    activity_module_->InitActivity(test_role_id_);
    
    EXPECT_TRUE(activity_module_->CheckActivityTime(1001));
}

TEST_F(ActivityModuleTest, GetActivityRank) {
    activity_module_->InitActivity(test_role_id_);
    activity_module_->JoinActivity(test_role_id_, 1001);
    
    std::vector<ActivityRankInfo> ranks;
    EXPECT_TRUE(activity_module_->GetActivityRank(1001, ranks));
}

TEST_F(ActivityModuleTest, UpdateRank) {
    activity_module_->InitActivity(test_role_id_);
    activity_module_->JoinActivity(test_role_id_, 1001);
    
    EXPECT_TRUE(activity_module_->UpdateRank(test_role_id_, 1001, 1000));
}

TEST_F(ActivityModuleTest, ResetDailyActivity) {
    activity_module_->InitActivity(test_role_id_);
    activity_module_->JoinActivity(test_role_id_, 1001);
    activity_module_->UpdateProgress(test_role_id_, 1001, 50);
    
    EXPECT_TRUE(activity_module_->ResetDailyActivity(test_role_id_));
    
    int32_t progress = 0;
    activity_module_->GetProgress(test_role_id_, 1001, progress);
    EXPECT_EQ(progress, 0);
}
