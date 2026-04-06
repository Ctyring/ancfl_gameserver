#include <gtest/gtest.h>
#include "logic_server/task_module.h"
#include "logic_server/logic_service.h"

using namespace game_server;

class TaskModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        service_ = nullptr;
        task_module_ = new TaskModule(service_);
        test_role_id_ = 12345;
    }
    
    void TearDown() override {
        delete task_module_;
    }
    
    LogicService* service_;
    TaskModule* task_module_;
    uint64_t test_role_id_;
};

TEST_F(TaskModuleTest, InitTasks) {
    EXPECT_TRUE(task_module_->InitTasks(test_role_id_));
}

TEST_F(TaskModuleTest, AcceptTask) {
    task_module_->InitTasks(test_role_id_);
    
    EXPECT_TRUE(task_module_->AcceptTask(test_role_id_, 1001));
}

TEST_F(TaskModuleTest, GetTasks) {
    task_module_->InitTasks(test_role_id_);
    task_module_->AcceptTask(test_role_id_, 1001);
    task_module_->AcceptTask(test_role_id_, 1002);
    
    std::vector<TaskInfo> tasks;
    EXPECT_TRUE(task_module_->GetTasks(test_role_id_, tasks));
    EXPECT_EQ(tasks.size(), 2);
}

TEST_F(TaskModuleTest, GetTaskInfo) {
    task_module_->InitTasks(test_role_id_);
    task_module_->AcceptTask(test_role_id_, 1001);
    
    TaskInfo info;
    EXPECT_TRUE(task_module_->GetTaskInfo(test_role_id_, 1001, info));
    EXPECT_EQ(info.task_config_id, 1001);
}

TEST_F(TaskModuleTest, UpdateTaskProgress) {
    task_module_->InitTasks(test_role_id_);
    task_module_->AcceptTask(test_role_id_, 1001);
    
    EXPECT_TRUE(task_module_->UpdateTaskProgress(test_role_id_, 1001, 0, 1));
}

TEST_F(TaskModuleTest, AddTaskProgress) {
    task_module_->InitTasks(test_role_id_);
    task_module_->AcceptTask(test_role_id_, 1001);
    
    EXPECT_TRUE(task_module_->AddTaskProgress(test_role_id_, TaskConditionType::KILL_MONSTER, 100, 1));
}

TEST_F(TaskModuleTest, SubmitTask) {
    task_module_->InitTasks(test_role_id_);
    task_module_->AcceptTask(test_role_id_, 1001);
    
    // 先完成任务
    task_module_->UpdateTaskProgress(test_role_id_, 1001, 0, 1);
    
    EXPECT_TRUE(task_module_->SubmitTask(test_role_id_, 1001));
}

TEST_F(TaskModuleTest, AbandonTask) {
    task_module_->InitTasks(test_role_id_);
    task_module_->AcceptTask(test_role_id_, 1001);
    
    EXPECT_TRUE(task_module_->AbandonTask(test_role_id_, 1001));
}

TEST_F(TaskModuleTest, TrackTask) {
    task_module_->InitTasks(test_role_id_);
    task_module_->AcceptTask(test_role_id_, 1001);
    
    EXPECT_TRUE(task_module_->TrackTask(test_role_id_, 1001));
    
    int32_t tracked_task = 0;
    EXPECT_TRUE(task_module_->GetTrackedTask(test_role_id_, tracked_task));
    EXPECT_EQ(tracked_task, 1001);
}

TEST_F(TaskModuleTest, RefreshDailyTasks) {
    task_module_->InitTasks(test_role_id_);
    
    EXPECT_TRUE(task_module_->RefreshDailyTasks(test_role_id_));
}

TEST_F(TaskModuleTest, GetDailyTasks) {
    task_module_->InitTasks(test_role_id_);
    task_module_->RefreshDailyTasks(test_role_id_);
    
    std::vector<DailyTaskInfo> tasks;
    EXPECT_TRUE(task_module_->GetDailyTasks(test_role_id_, tasks));
}
