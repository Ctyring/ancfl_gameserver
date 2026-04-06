#include <gtest/gtest.h>
#include "logic_server/buff_module.h"
#include "logic_server/logic_service.h"

using namespace game_server;

class BuffModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        service_ = nullptr;
        buff_module_ = new BuffModule(service_);
        test_role_id_ = 12345;
    }
    
    void TearDown() override {
        delete buff_module_;
    }
    
    LogicService* service_;
    BuffModule* buff_module_;
    uint64_t test_role_id_;
};

TEST_F(BuffModuleTest, AddBuff) {
    EXPECT_TRUE(buff_module_->AddBuff(test_role_id_, 0, 1001));
}

TEST_F(BuffModuleTest, RemoveBuff) {
    buff_module_->AddBuff(test_role_id_, 0, 1001);
    
    EXPECT_TRUE(buff_module_->RemoveBuffByConfigId(test_role_id_, 1001));
}

TEST_F(BuffModuleTest, RemoveAllBuffs) {
    buff_module_->AddBuff(test_role_id_, 0, 1001);
    buff_module_->AddBuff(test_role_id_, 0, 1002);
    buff_module_->AddBuff(test_role_id_, 0, 1003);
    
    EXPECT_TRUE(buff_module_->RemoveAllBuffs(test_role_id_));
}

TEST_F(BuffModuleTest, GetBuffs) {
    buff_module_->AddBuff(test_role_id_, 0, 1001);
    buff_module_->AddBuff(test_role_id_, 0, 1002);
    
    std::vector<BuffInfo> buffs;
    EXPECT_TRUE(buff_module_->GetBuffs(test_role_id_, buffs));
    EXPECT_EQ(buffs.size(), 2);
}

TEST_F(BuffModuleTest, GetBuffInfo) {
    buff_module_->AddBuff(test_role_id_, 0, 1001);
    
    std::vector<BuffInfo> buffs;
    buff_module_->GetBuffs(test_role_id_, buffs);
    if (!buffs.empty()) {
        BuffInfo info;
        EXPECT_TRUE(buff_module_->GetBuffInfo(test_role_id_, buffs[0].buff_id, info));
        EXPECT_EQ(info.buff_config_id, 1001);
    }
}

TEST_F(BuffModuleTest, HasBuff) {
    buff_module_->AddBuff(test_role_id_, 0, 1001);
    EXPECT_TRUE(buff_module_->HasBuff(test_role_id_, 1001));
}

TEST_F(BuffModuleTest, RefreshBuff) {
    buff_module_->AddBuff(test_role_id_, 0, 1001);
    
    std::vector<BuffInfo> buffs;
    buff_module_->GetBuffs(test_role_id_, buffs);
    if (!buffs.empty()) {
        EXPECT_TRUE(buff_module_->RefreshBuff(test_role_id_, buffs[0].buff_id));
    }
}

TEST_F(BuffModuleTest, StackBuff) {
    buff_module_->AddBuff(test_role_id_, 0, 1001);
    
    std::vector<BuffInfo> buffs;
    buff_module_->GetBuffs(test_role_id_, buffs);
    if (!buffs.empty()) {
        EXPECT_TRUE(buff_module_->StackBuff(test_role_id_, buffs[0].buff_id));
    }
}

TEST_F(BuffModuleTest, CalculateBuffEffects) {
    buff_module_->AddBuff(test_role_id_, 0, 1001);
    
    int32_t attack_bonus = 0;
    int32_t defense_bonus = 0;
    int32_t speed_bonus = 0;
    EXPECT_TRUE(buff_module_->CalculateBuffEffects(test_role_id_, attack_bonus, defense_bonus, speed_bonus));
}
