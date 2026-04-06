#include <gtest/gtest.h>
#include "db_server/db_service.h"

using namespace game_server;

class DBServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_service_ = new DBService();
        // 初始化服务
        db_service_->InitService();
        // 连接数据库
        db_service_->ConnectToDatabase();
    }
    
    void TearDown() override {
        delete db_service_;
    }
    
    DBService* db_service_;
};

TEST_F(DBServiceTest, CreateAccount) {
    std::string account_name = "test_account_" + std::to_string(time(nullptr));
    std::string password = "test_password";
    int32_t channel = 1;
    uint64_t account_id = 0;
    
    EXPECT_TRUE(db_service_->CreateAccount(account_name, password, channel, account_id));
    EXPECT_GT(account_id, 0);
}

TEST_F(DBServiceTest, VerifyAccount) {
    // 先创建一个账号
    std::string account_name = "test_account_" + std::to_string(time(nullptr));
    std::string password = "test_password";
    int32_t channel = 1;
    uint64_t account_id = 0;
    
    EXPECT_TRUE(db_service_->CreateAccount(account_name, password, channel, account_id));
    
    // 验证账号
    uint64_t verify_account_id = 0;
    EXPECT_TRUE(db_service_->VerifyAccount(account_name, password, verify_account_id));
    EXPECT_EQ(verify_account_id, account_id);
    
    // 验证错误密码
    verify_account_id = 0;
    EXPECT_FALSE(db_service_->VerifyAccount(account_name, "wrong_password", verify_account_id));
    EXPECT_EQ(verify_account_id, 0);
}

TEST_F(DBServiceTest, GetAccountInfo) {
    // 先创建一个账号
    std::string account_name = "test_account_" + std::to_string(time(nullptr));
    std::string password = "test_password";
    int32_t channel = 1;
    uint64_t account_id = 0;
    
    EXPECT_TRUE(db_service_->CreateAccount(account_name, password, channel, account_id));
    
    // 获取账号信息
    AccountInfo info;
    EXPECT_TRUE(db_service_->GetAccountInfo(account_id, info));
    EXPECT_EQ(info.account_id, account_id);
    EXPECT_EQ(info.account_name, account_name);
}

TEST_F(DBServiceTest, SealAndUnsealAccount) {
    // 先创建一个账号
    std::string account_name = "test_account_" + std::to_string(time(nullptr));
    std::string password = "test_password";
    int32_t channel = 1;
    uint64_t account_id = 0;
    
    EXPECT_TRUE(db_service_->CreateAccount(account_name, password, channel, account_id));
    
    // 封禁账号
    int64_t seal_end_time = time(nullptr) + 3600; // 封禁1小时
    EXPECT_TRUE(db_service_->SealAccount(account_id, seal_end_time));
    EXPECT_TRUE(db_service_->IsAccountSealed(account_id));
    
    // 解封账号
    EXPECT_TRUE(db_service_->UnsealAccount(account_id));
    EXPECT_FALSE(db_service_->IsAccountSealed(account_id));
}

TEST_F(DBServiceTest, CreateAndGetRole) {
    // 先创建一个账号
    std::string account_name = "test_account_" + std::to_string(time(nullptr));
    std::string password = "test_password";
    int32_t channel = 1;
    uint64_t account_id = 0;
    
    EXPECT_TRUE(db_service_->CreateAccount(account_name, password, channel, account_id));
    
    // 创建角色
    RoleInfo role_info;
    role_info.account_id = account_id;
    role_info.server_id = 1;
    role_info.role_name = "TestRole";
    role_info.career = 1;
    role_info.level = 1;
    role_info.exp = 0;
    role_info.head_id = 1;
    role_info.portrait_frame = 1;
    role_info.create_time = time(nullptr);
    role_info.last_login_time = time(nullptr);
    role_info.is_deleted = 0;
    role_info.delete_time = 0;
    
    EXPECT_TRUE(db_service_->CreateRole(role_info));
    
    // 获取角色数据
    RoleInfo retrieved_role;
    EXPECT_TRUE(db_service_->GetRoleData(role_info.role_id, retrieved_role));
    EXPECT_EQ(retrieved_role.role_id, role_info.role_id);
    EXPECT_EQ(retrieved_role.account_id, role_info.account_id);
    EXPECT_EQ(retrieved_role.role_name, role_info.role_name);
}

TEST_F(DBServiceTest, GetRoleList) {
    // 先创建一个账号
    std::string account_name = "test_account_" + std::to_string(time(nullptr));
    std::string password = "test_password";
    int32_t channel = 1;
    uint64_t account_id = 0;
    
    EXPECT_TRUE(db_service_->CreateAccount(account_name, password, channel, account_id));
    
    // 创建两个角色
    for (int i = 1; i <= 2; ++i) {
        RoleInfo role_info;
        role_info.account_id = account_id;
        role_info.server_id = 1;
        role_info.role_name = "TestRole" + std::to_string(i);
        role_info.career = i;
        role_info.level = 1;
        role_info.exp = 0;
        role_info.head_id = 1;
        role_info.portrait_frame = 1;
        role_info.create_time = time(nullptr);
        role_info.last_login_time = time(nullptr);
        role_info.is_deleted = 0;
        role_info.delete_time = 0;
        
        EXPECT_TRUE(db_service_->CreateRole(role_info));
    }
    
    // 获取角色列表
    std::vector<RoleInfo> roles;
    EXPECT_TRUE(db_service_->GetRoleList(account_id, roles));
    EXPECT_EQ(roles.size(), 2);
}

TEST_F(DBServiceTest, UpdateRole) {
    // 先创建一个账号和角色
    std::string account_name = "test_account_" + std::to_string(time(nullptr));
    std::string password = "test_password";
    int32_t channel = 1;
    uint64_t account_id = 0;
    
    EXPECT_TRUE(db_service_->CreateAccount(account_name, password, channel, account_id));
    
    RoleInfo role_info;
    role_info.account_id = account_id;
    role_info.server_id = 1;
    role_info.role_name = "TestRole";
    role_info.career = 1;
    role_info.level = 1;
    role_info.exp = 0;
    role_info.head_id = 1;
    role_info.portrait_frame = 1;
    role_info.create_time = time(nullptr);
    role_info.last_login_time = time(nullptr);
    role_info.is_deleted = 0;
    role_info.delete_time = 0;
    
    EXPECT_TRUE(db_service_->CreateRole(role_info));
    
    // 更新角色信息
    role_info.level = 10;
    role_info.exp = 1000;
    EXPECT_TRUE(db_service_->UpdateRole(role_info));
    
    // 验证更新
    RoleInfo updated_role;
    EXPECT_TRUE(db_service_->GetRoleData(role_info.role_id, updated_role));
    EXPECT_EQ(updated_role.level, 10);
    EXPECT_EQ(updated_role.exp, 1000);
}

TEST_F(DBServiceTest, DeleteRole) {
    // 先创建一个账号和角色
    std::string account_name = "test_account_" + std::to_string(time(nullptr));
    std::string password = "test_password";
    int32_t channel = 1;
    uint64_t account_id = 0;
    
    EXPECT_TRUE(db_service_->CreateAccount(account_name, password, channel, account_id));
    
    RoleInfo role_info;
    role_info.account_id = account_id;
    role_info.server_id = 1;
    role_info.role_name = "TestRole";
    role_info.career = 1;
    role_info.level = 1;
    role_info.exp = 0;
    role_info.head_id = 1;
    role_info.portrait_frame = 1;
    role_info.create_time = time(nullptr);
    role_info.last_login_time = time(nullptr);
    role_info.is_deleted = 0;
    role_info.delete_time = 0;
    
    EXPECT_TRUE(db_service_->CreateRole(role_info));
    
    // 删除角色
    EXPECT_TRUE(db_service_->DeleteRole(role_info.role_id));
    
    // 验证角色已删除
    RoleInfo retrieved_role;
    EXPECT_FALSE(db_service_->GetRoleData(role_info.role_id, retrieved_role));
}
