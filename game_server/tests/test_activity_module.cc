#include <gtest/gtest.h>
#include "logic_server/activity_module.h"
#include "logic_server/logic_service.h"

using namespace game_server;

class ActivityModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        service_ = nullptr;
        activity_module_ = new ActivityModule(service_);
        test_role_id_ = 12345;
    }
    
    void TearDown() override {
        delete activity_module_;
    }
    
    LogicService* service_;
    ActivityModule* activity_module_;
    uint64_t test_role_id_;
};

TEST_F(ActivityModuleTest, JoinActivity) {
    EXPECT_TRUE(activity_module_->JoinActivity(test_role_id_, 1001));
}

TEST_F(ActivityModuleTest, GetActivityInfo) {
    ActivityInfo info;
    EXPECT_TRUE(activity_module_->GetActivityInfo(1001, info));
    EXPECT_EQ(info.activity_id, 1001);
}

TEST_F(ActivityModuleTest, GetActivityList) {
    std::vector<ActivityInfo> activities;
    EXPECT_TRUE(activity_module_->GetActivityList(activities));
    EXPECT_GT(activities.size(), 0);
}

TEST_F(ActivityModuleTest, GetPlayerActivityData) {
    activity_module_->JoinActivity(test_role_id_, 1001);
    
    std::vector<PlayerActivityData> data_list;
    EXPECT_TRUE(activity_module_->GetPlayerActivityData(test_role_id_, data_list));
    EXPECT_GT(data_list.size(), 0);
}

TEST_F(ActivityModuleTest, GetActivityTaskProgress) {
    activity_module_->JoinActivity(test_role_id_, 1001);
    
    std::vector<ActivityTaskInfo> tasks;
    EXPECT_TRUE(activity_module_->GetActivityTaskProgress(test_role_id_, 1001, tasks));
}

TEST_F(ActivityModuleTest, UpdateTaskProgress) {
    activity_module_->JoinActivity(test_role_id_, 1001);
    
    EXPECT_TRUE(activity_module_->UpdateTaskProgress(test_role_id_, 1001, 1, 50));
}

TEST_F(ActivityModuleTest, GetTaskReward) {
    activity_module_->JoinActivity(test_role_id_, 1001);
    
    std::vector<ActivityRewardInfo> rewards;
    EXPECT_TRUE(activity_module_->GetTaskReward(test_role_id_, 1001, 1, rewards));
}

TEST_F(ActivityModuleTest, GetActivityRanking) {
    activity_module_->JoinActivity(test_role_id_, 1001);
    
    std::vector<ActivityRankData> ranking;
    EXPECT_TRUE(activity_module_->GetActivityRanking(1001, 10, ranking));
}

TEST_F(ActivityModuleTest, UpdateActivityScore) {
    activity_module_->JoinActivity(test_role_id_, 1001);
    
    EXPECT_TRUE(activity_module_->UpdateActivityScore(test_role_id_, 1001, 1000));
}
