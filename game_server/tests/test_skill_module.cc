#include <gtest/gtest.h>
#include "logic_server/skill_module.h"
#include "logic_server/logic_service.h"

using namespace game_server;

class SkillModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        service_ = nullptr;
        skill_module_ = new SkillModule(service_);
        test_role_id_ = 12345;
    }
    
    void TearDown() override {
        delete skill_module_;
    }
    
    LogicService* service_;
    SkillModule* skill_module_;
    uint64_t test_role_id_;
};

TEST_F(SkillModuleTest, InitSkills) {
    EXPECT_TRUE(skill_module_->InitSkills(test_role_id_));
}

TEST_F(SkillModuleTest, LearnSkill) {
    skill_module_->InitSkills(test_role_id_);
    
    EXPECT_TRUE(skill_module_->LearnSkill(test_role_id_, 1001));
}

TEST_F(SkillModuleTest, UpgradeSkill) {
    skill_module_->InitSkills(test_role_id_);
    skill_module_->LearnSkill(test_role_id_, 1001);
    
    EXPECT_TRUE(skill_module_->UpgradeSkill(test_role_id_, 1001));
}

TEST_F(SkillModuleTest, ForgetSkill) {
    skill_module_->InitSkills(test_role_id_);
    skill_module_->LearnSkill(test_role_id_, 1001);
    
    EXPECT_TRUE(skill_module_->ForgetSkill(test_role_id_, 1001));
}

TEST_F(SkillModuleTest, UseSkill) {
    skill_module_->InitSkills(test_role_id_);
    skill_module_->LearnSkill(test_role_id_, 1001);
    
    EXPECT_TRUE(skill_module_->UseSkill(test_role_id_, 1001, 99999, 0.0f, 0.0f));
}

TEST_F(SkillModuleTest, GetSkills) {
    skill_module_->InitSkills(test_role_id_);
    skill_module_->LearnSkill(test_role_id_, 1001);
    skill_module_->LearnSkill(test_role_id_, 1002);
    skill_module_->LearnSkill(test_role_id_, 1003);
    
    std::vector<SkillInfo> skills;
    EXPECT_TRUE(skill_module_->GetSkills(test_role_id_, skills));
    EXPECT_EQ(skills.size(), 3);
}

TEST_F(SkillModuleTest, GetSkillInfo) {
    skill_module_->InitSkills(test_role_id_);
    skill_module_->LearnSkill(test_role_id_, 1001);
    
    SkillInfo info;
    EXPECT_TRUE(skill_module_->GetSkillInfo(test_role_id_, 1001, info));
    EXPECT_EQ(info.skill_config_id, 1001);
}

TEST_F(SkillModuleTest, CanUseSkill) {
    skill_module_->InitSkills(test_role_id_);
    skill_module_->LearnSkill(test_role_id_, 1001);
    
    EXPECT_TRUE(skill_module_->CanUseSkill(test_role_id_, 1001));
}

TEST_F(SkillModuleTest, GetSkillCooldown) {
    skill_module_->InitSkills(test_role_id_);
    skill_module_->LearnSkill(test_role_id_, 1001);
    
    int32_t cooldown = skill_module_->GetSkillCooldown(test_role_id_, 1001);
    EXPECT_GE(cooldown, 0);
}
