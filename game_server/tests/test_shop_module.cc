#include <gtest/gtest.h>
#include "logic_server/shop_module.h"
#include "logic_server/logic_service.h"

using namespace game_server;

class ShopModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        service_ = nullptr;
        shop_module_ = new ShopModule(service_);
        test_role_id_ = 12345;
    }
    
    void TearDown() override {
        delete shop_module_;
    }
    
    LogicService* service_;
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
    EXPECT_GT(shops.size(), 0);
}

TEST_F(ShopModuleTest, GetShopInfo) {
    shop_module_->InitShop(test_role_id_);
    
    ShopInfo info;
    EXPECT_TRUE(shop_module_->GetShopInfo(1, info));
    EXPECT_EQ(info.shop_id, 1);
}

TEST_F(ShopModuleTest, GetShopItems) {
    shop_module_->InitShop(test_role_id_);
    
    std::vector<ShopItem> items;
    EXPECT_TRUE(shop_module_->GetShopItems(1, items));
    EXPECT_GT(items.size(), 0);
}

TEST_F(ShopModuleTest, BuyItem) {
    shop_module_->InitShop(test_role_id_);
    
    EXPECT_TRUE(shop_module_->BuyItem(test_role_id_, 1, 1001, 1));
}

TEST_F(ShopModuleTest, CanBuyItem) {
    shop_module_->InitShop(test_role_id_);
    
    EXPECT_TRUE(shop_module_->CanBuyItem(test_role_id_, 1, 1001, 1));
}

TEST_F(ShopModuleTest, GetItemBuyCount) {
    shop_module_->InitShop(test_role_id_);
    
    EXPECT_TRUE(shop_module_->GetItemBuyCount(test_role_id_, 1, 1001));
}

TEST_F(ShopModuleTest, AddItemBuyCount) {
    shop_module_->InitShop(test_role_id_);
    
    EXPECT_TRUE(shop_module_->AddItemBuyCount(test_role_id_, 1, 1001, 1));
}

TEST_F(ShopModuleTest, GetPurchaseRecords) {
    shop_module_->InitShop(test_role_id_);
    
    std::vector<PurchaseRecord> records;
    EXPECT_TRUE(shop_module_->GetPurchaseRecords(test_role_id_, records));
}
