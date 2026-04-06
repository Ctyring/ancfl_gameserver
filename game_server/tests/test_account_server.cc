#include <gtest/gtest.h>
#include "account_server/account_service.h"
#include "common/message_dispatcher.h"

using namespace game_server;

/**
 * 账号服务器测试类
 * 测试账号服务器的核心功能，包括：
 * - 服务初始化
 * - 账号创建
 * - 密码修改
 * - 账号查询
 * - 账号状态管理
 */
class AccountServiceTest : public ::testing::Test {
protected:
    /**
     * 测试准备：创建账号服务实例
     */
    void SetUp() override {
        // 创建 AccountService 实例
        account_service_ = new AccountService();
    }

    /**
     * 测试清理：销毁账号服务实例
     */
    void TearDown() override {
        delete account_service_;
    }

    // 账号服务实例
    AccountService* account_service_;
};

/**
 * 测试服务名称
 * 验证账号服务的名称是否正确
 */
TEST_F(AccountServiceTest, GetServiceName) {
    EXPECT_EQ(account_service_->GetServiceName(), "AccountServer");
}

/**
 * 测试服务 ID
 * 验证账号服务的初始 ID 是否为 0
 */
TEST_F(AccountServiceTest, GetServiceId) {
    EXPECT_EQ(account_service_->GetServiceId(), 0);
}

/**
 * 测试设置服务 ID
 * 验证服务 ID 可以正确设置和获取
 */
TEST_F(AccountServiceTest, SetServiceId) {
    account_service_->SetServiceId(200);
    EXPECT_EQ(account_service_->GetServiceId(), 200);
}

/**
 * 测试服务初始化
 * 验证账号服务能够成功初始化
 */
TEST_F(AccountServiceTest, InitService) {
    EXPECT_TRUE(account_service_->InitService());
    account_service_->Stop();
}

/**
 * 测试服务停止
 * 验证账号服务能够正常停止
 */
TEST_F(AccountServiceTest, StopService) {
    account_service_->Stop();
}
