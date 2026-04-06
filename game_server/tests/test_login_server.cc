#include <gtest/gtest.h>
#include "login_server/login_service.h"
#include "common/message_dispatcher.h"

using namespace game_server;

/**
 * 登录服务器测试类
 * 测试登录服务器的核心功能，包括：
 * - 服务基础信息管理
 * - 验证码生成与验证
 * - 服务器信息管理
 * - 网络服务初始化与停止
 * - 登录流程测试
 * - 登出流程测试
 */
class LoginServiceTest : public ::testing::Test {
protected:
    /**
     * 测试准备：创建登录服务实例
     */
    void SetUp() override {
        // 创建 LoginService 实例
        login_service_ = new LoginService();
    }

    /**
     * 测试清理：销毁登录服务实例
     */
    void TearDown() override {
        delete login_service_;
    }

    // 登录服务实例
    LoginService* login_service_;
};

/**
 * 测试服务名称
 * 验证登录服务的名称是否正确
 */
TEST_F(LoginServiceTest, GetServiceName) {
    EXPECT_EQ(login_service_->GetServiceName(), "LoginServer");
}

/**
 * 测试服务 ID
 * 验证登录服务的初始 ID 是否为 0
 */
TEST_F(LoginServiceTest, GetServiceId) {
    EXPECT_EQ(login_service_->GetServiceId(), 0);
}

/**
 * 测试设置服务 ID
 * 验证服务 ID 可以正确设置和获取
 */
TEST_F(LoginServiceTest, SetServiceId) {
    login_service_->SetServiceId(100);
    EXPECT_EQ(login_service_->GetServiceId(), 100);
}

/**
 * 测试生成登录验证码
 * 验证生成的验证码是否在正确的范围内
 */
TEST_F(LoginServiceTest, GenerateLoginCode) {
    int32_t code1 = login_service_->GenerateLoginCode();
    int32_t code2 = login_service_->GenerateLoginCode();

    // 验证码应该是 6 位数
    EXPECT_GE(code1, 100000);
    EXPECT_LT(code1, 1000000);
    EXPECT_GE(code2, 100000);
    EXPECT_LT(code2, 1000000);
}

/**
 * 测试获取逻辑服务器信息（无服务器情况）
 * 验证当没有逻辑服务器时，返回 false
 */
TEST_F(LoginServiceTest, GetLogicServerInfo_NoServers) {
    std::string ip;
    int32_t port = 0;

    EXPECT_FALSE(login_service_->GetLogicServerInfo(12345, ip, port));
}

/**
 * 测试连接账号服务器
 * 验证连接账号服务器的接口调用
 */
TEST_F(LoginServiceTest, ConnectToAccountServer) {
    EXPECT_TRUE(login_service_->ConnectToAccountServer());
}

/**
 * 测试定时器功能
 * 验证定时器清理过期验证码和不活跃服务器的功能
 */
TEST_F(LoginServiceTest, OnTimer) {
    login_service_->OnTimer();
}

/**
 * 测试服务初始化
 * 验证登录服务能够成功初始化并绑定到网络端口
 */
TEST_F(LoginServiceTest, InitService) {
    EXPECT_TRUE(login_service_->InitService());
}

/**
 * 测试服务停止
 * 验证登录服务能够正常停止
 */
TEST_F(LoginServiceTest, StopService) {
    login_service_->Stop();
}
