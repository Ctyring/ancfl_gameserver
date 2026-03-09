#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "logic_server/task_module.h"

using namespace game_server;
using namespace testing;

class MockLogicServiceForTask : public LogicService {
public:
    MOCK_METHOD(bool, SendToClient, (uint64_t role_id, int32_t msg_id, const std::string& data), (override));
    MOCK_METHOD(bool, SendToDB, (int32_t msg_id, const std::string& data), (override));
};

class TaskModuleTest : public Test {
protected:
    void SetUp() override {
        mock_service_ = new MockLogicServiceForTask();
        task_module_ = new TaskModule(mock_service_);
        test_role_id_ = 12345;
    }
    
    void TearDown() override {
        delete task_module_;
        delete mock_service_;
    }
    
    MockLogicServiceForTask* mock_service_;
    TaskModule* task_module_;
    uint64_t test_role_id_;
};

TEST_F(TaskModuleTest, InitTask) {
    EXPECT_TRUE(task_module_->InitTask(test_role_id_));
}

TEST_F(TaskModuleTest, AcceptTask) {
    task_module_->InitTask(test_role_id_);
    
    EXPECT_TRUE(task_module_->AcceptTask(test_role_id_, 1001));
    EXPECT_TRUE(task_module_->HasTask(test_role_id_, 1001));
}

TEST_F(TaskModuleTest, AcceptDuplicateTask) {
    task_module_->InitTask(test_role_id_);
    task_module_->AcceptTask(test_role_id_, 1001);
    
    EXPECT_FALSE(task_module_->AcceptTask(test_role_id_, 1001));
}

TEST_F(TaskModuleTest, AbandonTask) {
    task_module_->InitTask(test_role_id_);
    task_module_->AcceptTask(test_role_id_, 1001);
    
    EXPECT_TRUE(task_module_->AbandonTask(test_role_id_, 1001));
    EXPECT_FALSE(task_module_->HasTask(test_role_id_, 1001));
}

TEST_F(TaskModuleTest, CompleteTask) {
    task_module_->InitTask(test_role_id_);
    task_module_->AcceptTask(test_role_id_, 1001);
    
    EXPECT_TRUE(task_module_->CompleteTask(test_role_id_, 1001));
    
    TaskInfo info;
    task_module_->GetTaskInfo(test_role_id_, 1001, info);
    EXPECT_EQ(info.status, TaskStatus::COMPLETED);
}

TEST_F(TaskModuleTest, SubmitTask) {
    task_module_->InitTask(test_role_id_);
    task_module_->AcceptTask(test_role_id_, 1001);
    task_module_->CompleteTask(test_role_id_, 1001);
    
    EXPECT_TRUE(task_module_->SubmitTask(test_role_id_, 1001));
    EXPECT_FALSE(task_module_->HasTask(test_role_id_, 1001));
}

TEST_F(TaskModuleTest, UpdateProgress) {
    task_module_->InitTask(test_role_id_);
    task_module_->AcceptTask(test_role_id_, 1001);
    
    EXPECT_TRUE(task_module_->UpdateProgress(test_role_id_, 1001, 1, 5));
    
    TaskInfo info;
    task_module_->GetTaskInfo(test_role_id_, 1001, info);
    EXPECT_EQ(info.conditions[0].current_count, 5);
}

TEST_F(TaskModuleTest, GetTaskList) {
    task_module_->InitTask(test_role_id_);
    task_module_->AcceptTask(test_role_id_, 1001);
    task_module_->AcceptTask(test_role_id_, 1002);
    
    std::vector<TaskInfo> tasks;
    EXPECT_TRUE(task_module_->GetTaskList(test_role_id_, tasks));
    EXPECT_EQ(tasks.size(), 2);
}

TEST_F(TaskModuleTest, GetDailyTasks) {
    task_module_->InitTask(test_role_id_);
    
    std::vector<TaskInfo> tasks;
    EXPECT_TRUE(task_module_->GetDailyTasks(test_role_id_, tasks));
}

TEST_F(TaskModuleTest, ResetDailyTasks) {
    task_module_->InitTask(test_role_id_);
    
    EXPECT_TRUE(task_module_->ResetDailyTasks(test_role_id_));
}

TEST_F(TaskModuleTest, GetTaskRewards) {
    task_module_->InitTask(test_role_id_);
    task_module_->AcceptTask(test_role_id_, 1001);
    task_module_->CompleteTask(test_role_id_, 1001);
    
    std::vector<TaskReward> rewards;
    EXPECT_TRUE(task_module_->GetTaskRewards(test_role_id_, 1001, rewards));
}
