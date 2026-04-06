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
 * 测试账号创建
 * 验证账号创建功能是否正常
 */
TEST_F(AccountServiceTest, CreateAccount) {
    uint64_t account_id = 0;
    bool result = account_service_->CreateAccount("test_account", "password123", 1, account_id);
    EXPECT_TRUE(result);
    EXPECT_GT(account_id, 0);
}

/**
 * 测试账号验证
 * 验证账号验证功能是否正常
 */
TEST_F(AccountServiceTest, VerifyAccount) {
    // 先创建一个账号
    uint64_t account_id = 0;
    bool create_result = account_service_->CreateAccount("verify_test", "password123", 1, account_id);
    EXPECT_TRUE(create_result);
    EXPECT_GT(account_id, 0);

    // 验证账号
    AccountInfo info;
    bool verify_result = account_service_->VerifyAccount("verify_test", "password123", info);
    EXPECT_TRUE(verify_result);
    EXPECT_EQ(info.account_id, account_id);
    EXPECT_EQ(info.account_name, "verify_test");
}

/**
 * 测试账号信息获取（通过账号ID）
 * 验证账号信息获取功能是否正常
 */
TEST_F(AccountServiceTest, GetAccountInfoById) {
    // 先创建一个账号
    uint64_t account_id = 0;
    bool create_result = account_service_->CreateAccount("info_test", "password123", 1, account_id);
    EXPECT_TRUE(create_result);
    EXPECT_GT(account_id, 0);

    // 通过账号ID获取信息
    AccountInfo info;
    bool get_result = account_service_->GetAccountInfo(account_id, info);
    EXPECT_TRUE(get_result);
    EXPECT_EQ(info.account_id, account_id);
    EXPECT_EQ(info.account_name, "info_test");
}

/**
 * 测试账号信息获取（通过账号名称）
 * 验证账号信息获取功能是否正常
 */
TEST_F(AccountServiceTest, GetAccountInfoByName) {
    // 先创建一个账号
    uint64_t account_id = 0;
    bool create_result = account_service_->CreateAccount("info_test2", "password123", 1, account_id);
    EXPECT_TRUE(create_result);
    EXPECT_GT(account_id, 0);

    // 通过账号名称获取信息
    AccountInfo info;
    bool get_result = account_service_->GetAccountInfo("info_test2", info);
    EXPECT_TRUE(get_result);
    EXPECT_EQ(info.account_id, account_id);
    EXPECT_EQ(info.account_name, "info_test2");
}

/**
 * 测试账号封号
 * 验证账号封号功能是否正常
 */
TEST_F(AccountServiceTest, SealAccount) {
    // 先创建一个账号
    uint64_t account_id = 0;
    bool create_result = account_service_->CreateAccount("seal_test", "password123", 1, account_id);
    EXPECT_TRUE(create_result);
    EXPECT_GT(account_id, 0);

    // 封号
    bool seal_result = account_service_->SealAccount(account_id, 3600); // 封号1小时
    EXPECT_TRUE(seal_result);

    // 检查账号是否被封
    bool is_sealed = account_service_->IsAccountSealed(account_id);
    EXPECT_TRUE(is_sealed);
}

/**
 * 测试账号解封
 * 验证账号解封功能是否正常
 */
TEST_F(AccountServiceTest, UnsealAccount) {
    // 先创建一个账号
    uint64_t account_id = 0;
    bool create_result = account_service_->CreateAccount("unseal_test", "password123", 1, account_id);
    EXPECT_TRUE(create_result);
    EXPECT_GT(account_id, 0);

    // 封号
    bool seal_result = account_service_->SealAccount(account_id, 3600);
    EXPECT_TRUE(seal_result);

    // 检查账号是否被封
    bool is_sealed = account_service_->IsAccountSealed(account_id);
    EXPECT_TRUE(is_sealed);

    // 解封
    bool unseal_result = account_service_->UnsealAccount(account_id);
    EXPECT_TRUE(unseal_result);

    // 检查账号是否被解封
    is_sealed = account_service_->IsAccountSealed(account_id);
    EXPECT_FALSE(is_sealed);
}

/**
 * 测试记录登录日志
 * 验证登录日志记录功能是否正常
 */
TEST_F(AccountServiceTest, RecordLoginLog) {
    // 先创建一个账号
    uint64_t account_id = 0;
    bool create_result = account_service_->CreateAccount("login_log_test", "password123", 1, account_id);
    EXPECT_TRUE(create_result);
    EXPECT_GT(account_id, 0);

    // 记录登录日志
    bool log_result = account_service_->RecordLoginLog(
        account_id, 1, "1.0.0", "uuid123", "idfa123", "iPhone", "imei123", 0x7f000001);
    EXPECT_TRUE(log_result);
}

/**
 * 测试服务初始化
 * 验证账号服务能够成功初始化
 */
TEST_F(AccountServiceTest, InitService) {
    EXPECT_TRUE(account_service_->InitService());
}

/**
 * 测试服务停止
 * 验证账号服务能够正常停止
 */
TEST_F(AccountServiceTest, StopService) {
    account_service_->Stop();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
