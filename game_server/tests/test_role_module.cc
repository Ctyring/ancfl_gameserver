#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "logic_server/role_module.h"

using namespace game_server;
using namespace testing;

class MockLogicService : public LogicService {
public:
    MOCK_METHOD(bool, SendToClient, (uint64_t role_id, int32_t msg_id, const std::string& data), (override));
    MOCK_METHOD(bool, SendToDB, (int32_t msg_id, const std::string& data), (override));
    MOCK_METHOD(bool, SendToLogin, (int32_t msg_id, const std::string& data), (override));
};

class RoleModuleTest : public Test {
protected:
    void SetUp() override {
        mock_service_ = new MockLogicService();
        role_module_ = new RoleModule(mock_service_);
    }
    
    void TearDown() override {
        delete role_module_;
        delete mock_service_;
    }
    
    MockLogicService* mock_service_;
    RoleModule* role_module_;
};

TEST_F(RoleModuleTest, CreateRole) {
    uint64_t role_id = 0;
    EXPECT_TRUE(role_module_->CreateRole(123456, "TestRole", 1, role_id));
    EXPECT_NE(role_id, 0);
}

TEST_F(RoleModuleTest, CreateDuplicateRole) {
    uint64_t role_id1 = 0;
    uint64_t role_id2 = 0;
    
    EXPECT_TRUE(role_module_->CreateRole(123456, "TestRole", 1, role_id1));
    EXPECT_FALSE(role_module_->CreateRole(123456, "TestRole2", 1, role_id2));
}

TEST_F(RoleModuleTest, GetRoleInfo) {
    uint64_t role_id = 0;
    role_module_->CreateRole(123456, "TestRole", 1, role_id);
    
    RoleInfo info;
    EXPECT_TRUE(role_module_->GetRoleInfo(role_id, info));
    EXPECT_EQ(info.role_id, role_id);
    EXPECT_EQ(info.role_name, "TestRole");
}

TEST_F(RoleModuleTest, GetNonExistingRole) {
    RoleInfo info;
    EXPECT_FALSE(role_module_->GetRoleInfo(999999, info));
}

TEST_F(RoleModuleTest, SetRoleLevel) {
    uint64_t role_id = 0;
    role_module_->CreateRole(123456, "TestRole", 1, role_id);
    
    EXPECT_TRUE(role_module_->SetRoleLevel(role_id, 10));
    
    RoleInfo info;
    role_module_->GetRoleInfo(role_id, info);
    EXPECT_EQ(info.level, 10);
}

TEST_F(RoleModuleTest, AddExp) {
    uint64_t role_id = 0;
    role_module_->CreateRole(123456, "TestRole", 1, role_id);
    
    EXPECT_TRUE(role_module_->AddExp(role_id, 100));
    
    RoleInfo info;
    role_module_->GetRoleInfo(role_id, info);
    EXPECT_EQ(info.exp, 100);
}

TEST_F(RoleModuleTest, SetRoleName) {
    uint64_t role_id = 0;
    role_module_->CreateRole(123456, "TestRole", 1, role_id);
    
    EXPECT_TRUE(role_module_->SetRoleName(role_id, "NewName"));
    
    RoleInfo info;
    role_module_->GetRoleInfo(role_id, info);
    EXPECT_EQ(info.role_name, "NewName");
}

TEST_F(RoleModuleTest, DeleteRole) {
    uint64_t role_id = 0;
    role_module_->CreateRole(123456, "TestRole", 1, role_id);
    
    EXPECT_TRUE(role_module_->DeleteRole(role_id));
    
    RoleInfo info;
    EXPECT_FALSE(role_module_->GetRoleInfo(role_id, info));
}

TEST_F(RoleModuleTest, RoleOnlineOffline) {
    uint64_t role_id = 0;
    role_module_->CreateRole(123456, "TestRole", 1, role_id);
    
    EXPECT_TRUE(role_module_->SetRoleOnline(role_id, true));
    EXPECT_TRUE(role_module_->IsRoleOnline(role_id));
    
    EXPECT_TRUE(role_module_->SetRoleOnline(role_id, false));
    EXPECT_FALSE(role_module_->IsRoleOnline(role_id));
}

TEST_F(RoleModuleTest, GetOnlineRoles) {
    uint64_t role_id1 = 0;
    uint64_t role_id2 = 0;
    
    role_module_->CreateRole(123456, "Role1", 1, role_id1);
    role_module_->CreateRole(123457, "Role2", 1, role_id2);
    
    role_module_->SetRoleOnline(role_id1, true);
    role_module_->SetRoleOnline(role_id2, false);
    
    std::vector<uint64_t> online_roles;
    role_module_->GetOnlineRoles(online_roles);
    
    EXPECT_EQ(online_roles.size(), 1);
    EXPECT_EQ(online_roles[0], role_id1);
}

TEST_F(RoleModuleTest, SetAndGetAttribute) {
    uint64_t role_id = 0;
    role_module_->CreateRole(123456, "TestRole", 1, role_id);
    
    role_module_->SetAttribute(role_id, "hp", 100);
    role_module_->SetAttribute(role_id, "mp", 50);
    
    EXPECT_EQ(role_module_->GetAttribute(role_id, "hp", 0), 100);
    EXPECT_EQ(role_module_->GetAttribute(role_id, "mp", 0), 50);
}
