#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "logic_server/equip_module.h"

using namespace game_server;
using namespace testing;

class MockLogicServiceForEquip : public LogicService {
public:
    MOCK_METHOD(bool, SendToClient, (uint64_t role_id, int32_t msg_id, const std::string& data), (override));
    MOCK_METHOD(bool, SendToDB, (int32_t msg_id, const std::string& data), (override));
};

class EquipModuleTest : public Test {
protected:
    void SetUp() override {
        mock_service_ = new MockLogicServiceForEquip();
        equip_module_ = new EquipModule(mock_service_);
        test_role_id_ = 12345;
    }
    
    void TearDown() override {
        delete equip_module_;
        delete mock_service_;
    }
    
    MockLogicServiceForEquip* mock_service_;
    EquipModule* equip_module_;
    uint64_t test_role_id_;
};

TEST_F(EquipModuleTest, InitEquip) {
    EXPECT_TRUE(equip_module_->InitEquip(test_role_id_));
}

TEST_F(EquipModuleTest, EquipItem) {
    equip_module_->InitEquip(test_role_id_);
    
    EquipInfo equip;
    equip.equip_id = 1001;
    equip.equip_config_id = 2001;
    equip.slot = EquipSlot::WEAPON;
    equip.level = 1;
    equip.star = 0;
    
    EXPECT_TRUE(equip_module_->EquipItem(test_role_id_, equip));
    
    EquipInfo equipped;
    EXPECT_TRUE(equip_module_->GetEquipBySlot(test_role_id_, EquipSlot::WEAPON, equipped));
    EXPECT_EQ(equipped.equip_config_id, 2001);
}

TEST_F(EquipModuleTest, UnequipItem) {
    equip_module_->InitEquip(test_role_id_);
    
    EquipInfo equip;
    equip.equip_id = 1001;
    equip.equip_config_id = 2001;
    equip.slot = EquipSlot::WEAPON;
    
    equip_module_->EquipItem(test_role_id_, equip);
    
    EXPECT_TRUE(equip_module_->UnequipItem(test_role_id_, EquipSlot::WEAPON));
    
    EquipInfo equipped;
    EXPECT_FALSE(equip_module_->GetEquipBySlot(test_role_id_, EquipSlot::WEAPON, equipped));
}

TEST_F(EquipModuleTest, ReplaceEquip) {
    equip_module_->InitEquip(test_role_id_);
    
    EquipInfo equip1;
    equip1.equip_id = 1001;
    equip1.equip_config_id = 2001;
    equip1.slot = EquipSlot::WEAPON;
    
    EquipInfo equip2;
    equip2.equip_id = 1002;
    equip2.equip_config_id = 2002;
    equip2.slot = EquipSlot::WEAPON;
    
    equip_module_->EquipItem(test_role_id_, equip1);
    EXPECT_TRUE(equip_module_->EquipItem(test_role_id_, equip2));
    
    EquipInfo equipped;
    equip_module_->GetEquipBySlot(test_role_id_, EquipSlot::WEAPON, equipped);
    EXPECT_EQ(equipped.equip_config_id, 2002);
}

TEST_F(EquipModuleTest, EnhanceEquip) {
    equip_module_->InitEquip(test_role_id_);
    
    EquipInfo equip;
    equip.equip_id = 1001;
    equip.equip_config_id = 2001;
    equip.slot = EquipSlot::WEAPON;
    equip.level = 1;
    
    equip_module_->EquipItem(test_role_id_, equip);
    
    EXPECT_TRUE(equip_module_->EnhanceEquip(test_role_id_, 1001));
    
    EquipInfo enhanced;
    equip_module_->GetEquipById(test_role_id_, 1001, enhanced);
    EXPECT_EQ(enhanced.level, 2);
}

TEST_F(EquipModuleTest, StarUpEquip) {
    equip_module_->InitEquip(test_role_id_);
    
    EquipInfo equip;
    equip.equip_id = 1001;
    equip.equip_config_id = 2001;
    equip.slot = EquipSlot::WEAPON;
    equip.star = 0;
    
    equip_module_->EquipItem(test_role_id_, equip);
    
    EXPECT_TRUE(equip_module_->StarUpEquip(test_role_id_, 1001));
    
    EquipInfo starred;
    equip_module_->GetEquipById(test_role_id_, 1001, starred);
    EXPECT_EQ(starred.star, 1);
}

TEST_F(EquipModuleTest, GetAllEquips) {
    equip_module_->InitEquip(test_role_id_);
    
    EquipInfo equip1;
    equip1.equip_id = 1001;
    equip1.slot = EquipSlot::WEAPON;
    
    EquipInfo equip2;
    equip2.equip_id = 1002;
    equip2.slot = EquipSlot::ARMOR;
    
    equip_module_->EquipItem(test_role_id_, equip1);
    equip_module_->EquipItem(test_role_id_, equip2);
    
    std::vector<EquipInfo> equips;
    EXPECT_TRUE(equip_module_->GetAllEquips(test_role_id_, equips));
    EXPECT_EQ(equips.size(), 2);
}

TEST_F(EquipModuleTest, GetEquipAttributes) {
    equip_module_->InitEquip(test_role_id_);
    
    EquipInfo equip;
    equip.equip_id = 1001;
    equip.equip_config_id = 2001;
    equip.slot = EquipSlot::WEAPON;
    equip.level = 5;
    equip.star = 3;
    
    equip_module_->EquipItem(test_role_id_, equip);
    
    std::unordered_map<std::string, int32_t> attrs;
    EXPECT_TRUE(equip_module_->GetEquipAttributes(test_role_id_, attrs));
    EXPECT_GT(attrs.size(), 0);
}

TEST_F(EquipModuleTest, CheckSuit) {
    equip_module_->InitEquip(test_role_id_);
    
    EquipInfo equip1;
    equip1.equip_id = 1001;
    equip1.suit_id = 1;
    equip1.slot = EquipSlot::WEAPON;
    
    EquipInfo equip2;
    equip2.equip_id = 1002;
    equip2.suit_id = 1;
    equip2.slot = EquipSlot::ARMOR;
    
    equip_module_->EquipItem(test_role_id_, equip1);
    equip_module_->EquipItem(test_role_id_, equip2);
    
    int32_t suit_count = 0;
    EXPECT_TRUE(equip_module_->GetSuitCount(test_role_id_, 1, suit_count));
    EXPECT_EQ(suit_count, 2);
}
