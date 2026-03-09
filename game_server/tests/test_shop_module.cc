#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "logic_server/shop_module.h"

using namespace game_server;
using namespace testing;

class MockLogicServiceForShop : public LogicService {
public:
    MOCK_METHOD(bool, SendToClient, (uint64_t role_id, int32_t msg_id, const std::string& data), (override));
    MOCK_METHOD(bool, SendToDB, (int32_t msg_id, const std::string& data), (override));
};

class ShopModuleTest : public Test {
protected:
    void SetUp() override {
        mock_service_ = new MockLogicServiceForShop();
        shop_module_ = new ShopModule(mock_service_);
        test_role_id_ = 12345;
    }
    
    void TearDown() override {
        delete shop_module_;
        delete mock_service_;
    }
    
    MockLogicServiceForShop* mock_service_;
    ShopModule* shop_module_;
    uint64_t test_role_id_;
};

TEST_F(ShopModuleTest, InitShop) {
    EXPECT_TRUE(shop_module_->InitShop(test_role_id_));
}

TEST_F(ShopModuleTest, GetShopList) {
    shop_module_->InitShop(test_role_id_);
    
    std::vector<ShopInfo> shops;
    EXPECT_TRUE(shop_module_->GetShopList(shops));
}

TEST_F(ShopModuleTest, GetShopItems) {
    shop_module_->InitShop(test_role_id_);
    
    std::vector<ShopItem> items;
    EXPECT_TRUE(shop_module_->GetShopItems(1, items));
}

TEST_F(ShopModuleTest, BuyItem) {
    shop_module_->InitShop(test_role_id_);
    
    EXPECT_TRUE(shop_module_->BuyItem(test_role_id_, 1, 1001, 1));
}

TEST_F(ShopModuleTest, BuyItemLimit) {
    shop_module_->InitShop(test_role_id_);
    
    EXPECT_TRUE(shop_module_->BuyItem(test_role_id_, 1, 1001, 10));
    
    int32_t count = 0;
    shop_module_->GetItemBuyCount(test_role_id_, 1, 1001, count);
    EXPECT_EQ(count, 10);
}

TEST_F(ShopModuleTest, RefreshShop) {
    shop_module_->InitShop(test_role_id_);
    
    EXPECT_TRUE(shop_module_->RefreshShop(test_role_id_, 1));
}

TEST_F(ShopModuleTest, OpenMysteryShop) {
    shop_module_->InitShop(test_role_id_);
    
    EXPECT_TRUE(shop_module_->OpenMysteryShop(test_role_id_));
}

TEST_F(ShopModuleTest, GetMysteryShop) {
    shop_module_->InitShop(test_role_id_);
    shop_module_->OpenMysteryShop(test_role_id_);
    
    MysteryShopInfo info;
    EXPECT_TRUE(shop_module_->GetMysteryShop(test_role_id_, info));
}

TEST_F(ShopModuleTest, RefreshMysteryShop) {
    shop_module_->InitShop(test_role_id_);
    shop_module_->OpenMysteryShop(test_role_id_);
    
    EXPECT_TRUE(shop_module_->RefreshMysteryShop(test_role_id_));
}

TEST_F(ShopModuleTest, BuyMysteryItem) {
    shop_module_->InitShop(test_role_id_);
    shop_module_->OpenMysteryShop(test_role_id_);
    
    EXPECT_TRUE(shop_module_->BuyMysteryItem(test_role_id_, 0));
}

TEST_F(ShopModuleTest, GetPurchaseRecord) {
    shop_module_->InitShop(test_role_id_);
    shop_module_->BuyItem(test_role_id_, 1, 1001, 1);
    
    std::vector<PurchaseRecord> records;
    EXPECT_TRUE(shop_module_->GetPurchaseRecord(test_role_id_, records));
    EXPECT_GT(records.size(), 0);
}

TEST_F(ShopModuleTest, ResetBuyCount) {
    shop_module_->InitShop(test_role_id_);
    shop_module_->BuyItem(test_role_id_, 1, 1001, 5);
    
    EXPECT_TRUE(shop_module_->ResetBuyCount(test_role_id_, 1));
    
    int32_t count = 0;
    shop_module_->GetItemBuyCount(test_role_id_, 1, 1001, count);
    EXPECT_EQ(count, 0);
}
