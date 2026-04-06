#include <gtest/gtest.h>
#include "logic_server/logic_service.h"

using namespace game_server;

/**
 * 逻辑服务器测试类
 * 测试逻辑服务器的核心功能，包括：
 * - 服务器初始化
 * - 模块协调
 * - 消息处理
 * - 游戏逻辑执行
 */
class LogicServiceTest : public ::testing::Test {
protected:
    /**
     * 测试准备：创建逻辑服务实例
     */
    void SetUp() override {
        // 创建 LogicService 实例
        logic_service_ = new LogicService();
    }

    /**
     * 测试清理：销毁逻辑服务实例
     */
    void TearDown() override {
        delete logic_service_;
    }

    // 逻辑服务实例
    LogicService* logic_service_;
};

/**
 * 测试服务名称
 * 验证逻辑服务的名称是否正确
 */
TEST_F(LogicServiceTest, GetServiceName) {
    EXPECT_EQ(logic_service_->GetServiceName(), "LogicService");
}

/**
 * 测试服务 ID
 * 验证逻辑服务的初始 ID 是否为 0
 */
TEST_F(LogicServiceTest, GetServiceId) {
    EXPECT_EQ(logic_service_->GetServiceId(), 0);
}

/**
 * 测试设置服务 ID
 * 验证服务 ID 可以正确设置和获取
 */
TEST_F(LogicServiceTest, SetServiceId) {
    logic_service_->SetServiceId(300);
    EXPECT_EQ(logic_service_->GetServiceId(), 300);
}

/**
 * 测试服务初始化
 * 验证逻辑服务能够成功初始化
 */
TEST_F(LogicServiceTest, InitService) {
    EXPECT_TRUE(logic_service_->InitService());
}

/**
 * 测试服务停止
 * 验证逻辑服务能够正常停止
 */
TEST_F(LogicServiceTest, StopService) {
    logic_service_->Stop();
}

/**
 * 测试连接数据库服务器
 * 验证逻辑服务能够连接到数据库服务器
 */
TEST_F(LogicServiceTest, ConnectToDBServer) {
    EXPECT_TRUE(logic_service_->ConnectToDBServer());
}

/**
 * 测试创建角色
 * 验证角色创建功能是否正常
 */
TEST_F(LogicServiceTest, CreateRole) {
    bool result = logic_service_->CreateRole(1, "TestRole", 1, 1);
    EXPECT_TRUE(result);
}

/**
 * 测试获取角色列表
 * 验证能够获取角色列表
 */
TEST_F(LogicServiceTest, GetRoleList) {
    // 先创建一个角色
    logic_service_->CreateRole(1, "TestRole2", 1, 1);

    std::vector<RoleData> roles;
    bool result = logic_service_->GetRoleList(1, roles);
    EXPECT_TRUE(result);
}

/**
 * 测试初始化共享内存
 * 验证共享内存能够成功初始化
 */
TEST_F(LogicServiceTest, InitSharedMemory) {
    EXPECT_TRUE(logic_service_->InitSharedMemory());
}

/**
 * 测试分配角色数据
 * 验证能够分配角色数据
 */
TEST_F(LogicServiceTest, AllocateRoleData) {
    RoleData* data = logic_service_->AllocateRoleData();
    EXPECT_NE(data, nullptr);

    // 释放角色数据
    logic_service_->FreeRoleData(data);
}

/**
 * 测试生成角色 ID
 * 验证能够生成角色 ID
 */
TEST_F(LogicServiceTest, GetNextRoleId) {
    uint64_t role_id1 = logic_service_->GetNextRoleId();
    uint64_t role_id2 = logic_service_->GetNextRoleId();

    EXPECT_GT(role_id1, 0);
    EXPECT_GT(role_id2, 0);
    EXPECT_NE(role_id1, role_id2);
}

/**
 * 测试获取角色模块
 * 验证能够获取角色模块
 */
TEST_F(LogicServiceTest, GetRoleModule) {
    RoleModule* module = logic_service_->GetRoleModule();
    EXPECT_NE(module, nullptr);
}

/**
 * 测试获取背包模块
 * 验证能够获取背包模块
 */
TEST_F(LogicServiceTest, GetBagModule) {
    BagModule* module = logic_service_->GetBagModule();
    EXPECT_NE(module, nullptr);
}

/**
 * 测试获取装备模块
 * 验证能够获取装备模块
 */
TEST_F(LogicServiceTest, GetEquipModule) {
    EquipModule* module = logic_service_->GetEquipModule();
    EXPECT_NE(module, nullptr);
}

/**
 * 测试获取任务模块
 * 验证能够获取任务模块
 */
TEST_F(LogicServiceTest, GetTaskModule) {
    TaskModule* module = logic_service_->GetTaskModule();
    EXPECT_NE(module, nullptr);
}

/**
 * 测试获取邮件模块
 * 验证能够获取邮件模块
 */
TEST_F(LogicServiceTest, GetMailModule) {
    MailModule* module = logic_service_->GetMailModule();
    EXPECT_NE(module, nullptr);
}

/**
 * 测试获取好友模块
 * 验证能够获取好友模块
 */
TEST_F(LogicServiceTest, GetFriendModule) {
    FriendModule* module = logic_service_->GetFriendModule();
    EXPECT_NE(module, nullptr);
}

/**
 * 测试获取商店模块
 * 验证能够获取商店模块
 */
TEST_F(LogicServiceTest, GetShopModule) {
    ShopModule* module = logic_service_->GetShopModule();
    EXPECT_NE(module, nullptr);
}

/**
 * 测试获取公会模块
 * 验证能够获取公会模块
 */
TEST_F(LogicServiceTest, GetGuildModule) {
    GuildModule* module = logic_service_->GetGuildModule();
    EXPECT_NE(module, nullptr);
}

/**
 * 测试获取 Buff 模块
 * 验证能够获取 Buff 模块
 */
TEST_F(LogicServiceTest, GetBuffModule) {
    BuffModule* module = logic_service_->GetBuffModule();
    EXPECT_NE(module, nullptr);
}

/**
 * 测试获取技能模块
 * 验证能够获取技能模块
 */
TEST_F(LogicServiceTest, GetSkillModule) {
    SkillModule* module = logic_service_->GetSkillModule();
    EXPECT_NE(module, nullptr);
}

/**
 * 测试获取场景模块
 * 验证能够获取场景模块
 */
TEST_F(LogicServiceTest, GetSceneModule) {
    SceneModule* module = logic_service_->GetSceneModule();
    EXPECT_NE(module, nullptr);
}

/**
 * 测试获取活动模块
 * 验证能够获取活动模块
 */
TEST_F(LogicServiceTest, GetActivityModule) {
    ActivityModule* module = logic_service_->GetActivityModule();
    EXPECT_NE(module, nullptr);
}
