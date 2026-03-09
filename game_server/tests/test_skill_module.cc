#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "logic_server/skill_module.h"

using namespace game_server;
using namespace testing;

class MockLogicServiceForSkill : public LogicService {
public:
    MOCK_METHOD(bool, SendToClient, (uint64_t role_id, int32_t msg_id, const std::string& data), (override));
};

class SkillModuleTest : public Test {
protected:
    void SetUp() override {
        mock_service_ = new MockLogicServiceForSkill();
        skill_module_ = new SkillModule(mock_service_);
        test_role_id_ = 12345;
    }
    
    void TearDown() override {
        delete skill_module_;
        delete mock_service_;
    }
    
    MockLogicServiceForSkill* mock_service_;
    SkillModule* skill_module_;
    uint64_t test_role_id_;
};

TEST_F(SkillModuleTest, InitSkill) {
    EXPECT_TRUE(skill_module_->InitSkill(test_role_id_));
}

TEST_F(SkillModuleTest, LearnSkill) {
    skill_module_->InitSkill(test_role_id_);
    
    EXPECT_TRUE(skill_module_->LearnSkill(test_role_id_, 1001));
    
    EXPECT_TRUE(skill_module_->HasSkill(test_role_id_, 1001));
}

TEST_F(SkillModuleTest, LearnDuplicateSkill) {
    skill_module_->InitSkill(test_role_id_);
    skill_module_->LearnSkill(test_role_id_, 1001);
    
    EXPECT_FALSE(skill_module_->LearnSkill(test_role_id_, 1001));
}

TEST_F(SkillModuleTest, UpgradeSkill) {
    skill_module_->InitSkill(test_role_id_);
    skill_module_->LearnSkill(test_role_id_, 1001);
    
    EXPECT_TRUE(skill_module_->UpgradeSkill(test_role_id_, 1001));
    
    SkillInfo info;
    skill_module_->GetSkillInfo(test_role_id_, 1001, info);
    EXPECT_EQ(info.level, 2);
}

TEST_F(SkillModuleTest, ForgetSkill) {
    skill_module_->InitSkill(test_role_id_);
    skill_module_->LearnSkill(test_role_id_, 1001);
    
    EXPECT_TRUE(skill_module_->ForgetSkill(test_role_id_, 1001));
    EXPECT_FALSE(skill_module_->HasSkill(test_role_id_, 1001));
}

TEST_F(SkillModuleTest, UseSkill) {
    skill_module_->InitSkill(test_role_id_);
    skill_module_->LearnSkill(test_role_id_, 1001);
    
    EXPECT_TRUE(skill_module_->UseSkill(test_role_id_, 1001, 99999));
}

TEST_F(SkillModuleTest, UseSkillNotLearned) {
    skill_module_->InitSkill(test_role_id_);
    
    EXPECT_FALSE(skill_module_->UseSkill(test_role_id_, 1001, 99999));
}

TEST_F(SkillModuleTest, GetSkillList) {
    skill_module_->InitSkill(test_role_id_);
    skill_module_->LearnSkill(test_role_id_, 1001);
    skill_module_->LearnSkill(test_role_id_, 1002);
    skill_module_->LearnSkill(test_role_id_, 1003);
    
    std::vector<SkillInfo> skills;
    EXPECT_TRUE(skill_module_->GetSkillList(test_role_id_, skills));
    EXPECT_EQ(skills.size(), 3);
}

TEST_F(SkillModuleTest, SetSkillSlot) {
    skill_module_->InitSkill(test_role_id_);
    skill_module_->LearnSkill(test_role_id_, 1001);
    
    EXPECT_TRUE(skill_module_->SetSkillSlot(test_role_id_, 1, 1001));
    
    int32_t skill_id = 0;
    skill_module_->GetSkillInSlot(test_role_id_, 1, skill_id);
    EXPECT_EQ(skill_id, 1001);
}

TEST_F(SkillModuleTest, GetCooldown) {
    skill_module_->InitSkill(test_role_id_);
    skill_module_->LearnSkill(test_role_id_, 1001);
    skill_module_->UseSkill(test_role_id_, 1001, 99999);
    
    int32_t cooldown = 0;
    EXPECT_TRUE(skill_module_->GetCooldown(test_role_id_, 1001, cooldown));
}

TEST_F(SkillModuleTest, CheckMpCost) {
    skill_module_->InitSkill(test_role_id_);
    skill_module_->LearnSkill(test_role_id_, 1001);
    
    int32_t mp_cost = 0;
    EXPECT_TRUE(skill_module_->GetSkillMpCost(test_role_id_, 1001, mp_cost));
}
