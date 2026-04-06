#include <gtest/gtest.h>
#include "logic_server/equip_module.h"
#include "logic_server/logic_service.h"

using namespace game_server;

class EquipModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        service_ = nullptr;
        equip_module_ = new EquipModule(service_);
        test_role_id_ = 123456;
        equip_module_->InitEquip(test_role_id_);
    }
    
    void TearDown() override {
        delete equip_module_;
    }
    
    LogicService* service_;
    EquipModule* equip_module_;
    uint64_t test_role_id_;
};

TEST_F(EquipModuleTest, InitEquip) {
    EXPECT_TRUE(equip_module_->InitEquip(test_role_id_ + 1));
}

TEST_F(EquipModuleTest, GetEquipList) {
    std::vector<EquipInfo> equips;
    EXPECT_TRUE(equip_module_->GetEquipList(test_role_id_, equips));
}

TEST_F(EquipModuleTest, WearEquip) {
    EXPECT_TRUE(equip_module_->WearEquip(test_role_id_, 1, (int32_t)EquipPosition::WEAPON));
}

TEST_F(EquipModuleTest, TakeOffEquip) {
    equip_module_->WearEquip(test_role_id_, 1, (int32_t)EquipPosition::WEAPON);
    EXPECT_TRUE(equip_module_->TakeOffEquip(test_role_id_, (int32_t)EquipPosition::WEAPON));
}

TEST_F(EquipModuleTest, GetWornEquips) {
    equip_module_->WearEquip(test_role_id_, 1, (int32_t)EquipPosition::WEAPON);
    
    std::vector<EquipInfo> equips;
    EXPECT_TRUE(equip_module_->GetWornEquips(test_role_id_, equips));
}

TEST_F(EquipModuleTest, StrengthenEquip) {
    equip_module_->WearEquip(test_role_id_, 1, (int32_t)EquipPosition::WEAPON);
    EXPECT_TRUE(equip_module_->StrengthenEquip(test_role_id_, 1, 5));
}

TEST_F(EquipModuleTest, GetStrengthenLevel) {
    equip_module_->WearEquip(test_role_id_, 1, (int32_t)EquipPosition::WEAPON);
    equip_module_->StrengthenEquip(test_role_id_, 1, 5);
    
    int32_t level = 0;
    EXPECT_TRUE(equip_module_->GetStrengthenLevel(test_role_id_, 1, level));
    EXPECT_EQ(level, 5);
}

TEST_F(EquipModuleTest, UpgradeStar) {
    equip_module_->WearEquip(test_role_id_, 1, (int32_t)EquipPosition::WEAPON);
    EXPECT_TRUE(equip_module_->UpgradeStar(test_role_id_, 1, 3));
}

TEST_F(EquipModuleTest, GetStarLevel) {
    equip_module_->WearEquip(test_role_id_, 1, (int32_t)EquipPosition::WEAPON);
    equip_module_->UpgradeStar(test_role_id_, 1, 3);
    
    int32_t star_level = 0;
    EXPECT_TRUE(equip_module_->GetStarLevel(test_role_id_, 1, star_level));
    EXPECT_EQ(star_level, 3);
}

TEST_F(EquipModuleTest, InlayGem) {
    equip_module_->WearEquip(test_role_id_, 1, (int32_t)EquipPosition::WEAPON);
    EXPECT_TRUE(equip_module_->InlayGem(test_role_id_, 1, 1001, 0));
}

TEST_F(EquipModuleTest, GetGems) {
    equip_module_->WearEquip(test_role_id_, 1, (int32_t)EquipPosition::WEAPON);
    equip_module_->InlayGem(test_role_id_, 1, 1001, 0);
    
    std::vector<int32_t> gems;
    EXPECT_TRUE(equip_module_->GetGems(test_role_id_, 1, gems));
}

TEST_F(EquipModuleTest, RemoveGem) {
    equip_module_->WearEquip(test_role_id_, 1, (int32_t)EquipPosition::WEAPON);
    equip_module_->InlayGem(test_role_id_, 1, 1001, 0);
    EXPECT_TRUE(equip_module_->RemoveGem(test_role_id_, 1, 0));
}

TEST_F(EquipModuleTest, CalculateEquipAttribute) {
    equip_module_->WearEquip(test_role_id_, 1, (int32_t)EquipPosition::WEAPON);
    
    EquipAttribute attr;
    EXPECT_TRUE(equip_module_->CalculateEquipAttribute(test_role_id_, attr));
}
