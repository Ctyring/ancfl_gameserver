#include <gtest/gtest.h>
#include "logic_server/role_module.h"
#include "logic_server/logic_service.h"

using namespace game_server;

class RoleModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        service_ = nullptr;
        role_module_ = new RoleModule(service_);
    }
    
    void TearDown() override {
        delete role_module_;
    }
    
    LogicService* service_;
    RoleModule* role_module_;
};

TEST_F(RoleModuleTest, CreateRole) {
    uint64_t role_id = 0;
    EXPECT_TRUE(role_module_->CreateRole(123456, "TestRole", 1, 1, role_id));
    EXPECT_NE(role_id, 0);
}

TEST_F(RoleModuleTest, GetRoleInfo) {
    uint64_t role_id = 0;
    role_module_->CreateRole(123456, "TestRole", 1, 1, role_id);
    
    RoleData info;
    EXPECT_TRUE(role_module_->GetRoleInfo(role_id, info));
    EXPECT_EQ(info.role_id, role_id);
    EXPECT_EQ(info.role_name, "TestRole");
}

TEST_F(RoleModuleTest, GetNonExistingRole) {
    RoleData info;
    EXPECT_FALSE(role_module_->GetRoleInfo(999999, info));
}

TEST_F(RoleModuleTest, DeleteRole) {
    uint64_t role_id = 0;
    role_module_->CreateRole(123456, "TestRole", 1, 1, role_id);
    
    EXPECT_TRUE(role_module_->DeleteRole(role_id));
    
    RoleData info;
    EXPECT_FALSE(role_module_->GetRoleInfo(role_id, info));
}

TEST_F(RoleModuleTest, RoleOnlineOffline) {
    uint64_t role_id = 0;
    role_module_->CreateRole(123456, "TestRole", 1, 1, role_id);
    
    EXPECT_TRUE(role_module_->SetRoleOnline(role_id, true));
    EXPECT_TRUE(role_module_->IsRoleOnline(role_id));
    
    EXPECT_TRUE(role_module_->SetRoleOnline(role_id, false));
    EXPECT_FALSE(role_module_->IsRoleOnline(role_id));
}

TEST_F(RoleModuleTest, AddRoleExp) {
    uint64_t role_id = 0;
    role_module_->CreateRole(123456, "TestRole", 1, 1, role_id);
    
    EXPECT_TRUE(role_module_->AddRoleExp(role_id, 100));
}

TEST_F(RoleModuleTest, AddRoleLevel) {
    uint64_t role_id = 0;
    role_module_->CreateRole(123456, "TestRole", 1, 1, role_id);
    
    EXPECT_TRUE(role_module_->AddRoleLevel(role_id, 10));
}

TEST_F(RoleModuleTest, GetRoleProperty) {
    uint64_t role_id = 0;
    role_module_->CreateRole(123456, "TestRole", 1, 1, role_id);
    
    RoleProperty property;
    EXPECT_TRUE(role_module_->GetRoleProperty(role_id, property));
}

TEST_F(RoleModuleTest, UpdateRoleProperty) {
    uint64_t role_id = 0;
    role_module_->CreateRole(123456, "TestRole", 1, 1, role_id);
    
    RoleProperty property;
    property.hp = 100;
    property.max_hp = 100;
    property.mp = 50;
    property.max_mp = 50;
    property.attack = 10;
    property.defense = 5;
    
    EXPECT_TRUE(role_module_->UpdateRoleProperty(role_id, property));
}

TEST_F(RoleModuleTest, GetRolePosition) {
    uint64_t role_id = 0;
    role_module_->CreateRole(123456, "TestRole", 1, 1, role_id);
    
    RolePosition position;
    EXPECT_TRUE(role_module_->GetRolePosition(role_id, position));
}

TEST_F(RoleModuleTest, UpdateRolePosition) {
    uint64_t role_id = 0;
    role_module_->CreateRole(123456, "TestRole", 1, 1, role_id);
    
    RolePosition position;
    position.scene_id = 1001;
    position.x = 100.0f;
    position.y = 200.0f;
    position.z = 300.0f;
    
    EXPECT_TRUE(role_module_->UpdateRolePosition(role_id, position));
}

TEST_F(RoleModuleTest, GetRoleList) {
    uint64_t role_id1 = 0;
    uint64_t role_id2 = 0;
    
    role_module_->CreateRole(123456, "Role1", 1, 1, role_id1);
    role_module_->CreateRole(123456, "Role2", 2, 1, role_id2);
    
    std::vector<RoleData> roles;
    EXPECT_TRUE(role_module_->GetRoleList(123456, roles));
    EXPECT_GE(roles.size(), 2);
}
