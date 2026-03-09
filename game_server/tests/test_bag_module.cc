#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "logic_server/bag_module.h"

using namespace game_server;
using namespace testing;

class MockLogicServiceForBag : public LogicService {
public:
    MOCK_METHOD(bool, SendToClient, (uint64_t role_id, int32_t msg_id, const std::string& data), (override));
    MOCK_METHOD(bool, SendToDB, (int32_t msg_id, const std::string& data), (override));
};

class BagModuleTest : public Test {
protected:
    void SetUp() override {
        mock_service_ = new MockLogicServiceForBag();
        bag_module_ = new BagModule(mock_service_);
        test_role_id_ = 12345;
    }
    
    void TearDown() override {
        delete bag_module_;
        delete mock_service_;
    }
    
    MockLogicServiceForBag* mock_service_;
    BagModule* bag_module_;
    uint64_t test_role_id_;
};

TEST_F(BagModuleTest, InitBag) {
    EXPECT_TRUE(bag_module_->InitBag(test_role_id_));
}

TEST_F(BagModuleTest, AddItem) {
    bag_module_->InitBag(test_role_id_);
    
    EXPECT_TRUE(bag_module_->AddItem(test_role_id_, 1001, 10));
    
    int32_t count = 0;
    EXPECT_TRUE(bag_module_->GetItemCount(test_role_id_, 1001, count));
    EXPECT_EQ(count, 10);
}

TEST_F(BagModuleTest, AddItemOverflow) {
    bag_module_->InitBag(test_role_id_);
    
    EXPECT_TRUE(bag_module_->AddItem(test_role_id_, 1001, 100));
    
    int32_t count = 0;
    bag_module_->GetItemCount(test_role_id_, 1001, count);
    EXPECT_EQ(count, 100);
}

TEST_F(BagModuleTest, RemoveItem) {
    bag_module_->InitBag(test_role_id_);
    bag_module_->AddItem(test_role_id_, 1001, 10);
    
    EXPECT_TRUE(bag_module_->RemoveItem(test_role_id_, 1001, 5));
    
    int32_t count = 0;
    bag_module_->GetItemCount(test_role_id_, 1001, count);
    EXPECT_EQ(count, 5);
}

TEST_F(BagModuleTest, RemoveItemNotEnough) {
    bag_module_->InitBag(test_role_id_);
    bag_module_->AddItem(test_role_id_, 1001, 10);
    
    EXPECT_FALSE(bag_module_->RemoveItem(test_role_id_, 1001, 20));
    
    int32_t count = 0;
    bag_module_->GetItemCount(test_role_id_, 1001, count);
    EXPECT_EQ(count, 10);
}

TEST_F(BagModuleTest, RemoveNonExistingItem) {
    bag_module_->InitBag(test_role_id_);
    
    EXPECT_FALSE(bag_module_->RemoveItem(test_role_id_, 9999, 1));
}

TEST_F(BagModuleTest, GetItemList) {
    bag_module_->InitBag(test_role_id_);
    bag_module_->AddItem(test_role_id_, 1001, 10);
    bag_module_->AddItem(test_role_id_, 1002, 20);
    bag_module_->AddItem(test_role_id_, 1003, 30);
    
    std::vector<ItemInfo> items;
    EXPECT_TRUE(bag_module_->GetItemList(test_role_id_, items));
    EXPECT_EQ(items.size(), 3);
}

TEST_F(BagModuleTest, HasItem) {
    bag_module_->InitBag(test_role_id_);
    bag_module_->AddItem(test_role_id_, 1001, 10);
    
    EXPECT_TRUE(bag_module_->HasItem(test_role_id_, 1001, 5));
    EXPECT_TRUE(bag_module_->HasItem(test_role_id_, 1001, 10));
    EXPECT_FALSE(bag_module_->HasItem(test_role_id_, 1001, 15));
    EXPECT_FALSE(bag_module_->HasItem(test_role_id_, 9999, 1));
}

TEST_F(BagModuleTest, ClearBag) {
    bag_module_->InitBag(test_role_id_);
    bag_module_->AddItem(test_role_id_, 1001, 10);
    bag_module_->AddItem(test_role_id_, 1002, 20);
    
    EXPECT_TRUE(bag_module_->ClearBag(test_role_id_));
    
    std::vector<ItemInfo> items;
    bag_module_->GetItemList(test_role_id_, items);
    EXPECT_EQ(items.size(), 0);
}

TEST_F(BagModuleTest, UseItem) {
    bag_module_->InitBag(test_role_id_);
    bag_module_->AddItem(test_role_id_, 1001, 10);
    
    EXPECT_TRUE(bag_module_->UseItem(test_role_id_, 1001, 1));
    
    int32_t count = 0;
    bag_module_->GetItemCount(test_role_id_, 1001, count);
    EXPECT_EQ(count, 9);
}

TEST_F(BagModuleTest, SortBag) {
    bag_module_->InitBag(test_role_id_);
    bag_module_->AddItem(test_role_id_, 1003, 10);
    bag_module_->AddItem(test_role_id_, 1001, 10);
    bag_module_->AddItem(test_role_id_, 1002, 10);
    
    EXPECT_TRUE(bag_module_->SortBag(test_role_id_));
    
    std::vector<ItemInfo> items;
    bag_module_->GetItemList(test_role_id_, items);
    
    EXPECT_EQ(items[0].item_config_id, 1001);
    EXPECT_EQ(items[1].item_config_id, 1002);
    EXPECT_EQ(items[2].item_config_id, 1003);
}

TEST_F(BagModuleTest, ExpandBag) {
    bag_module_->InitBag(test_role_id_);
    
    int32_t old_capacity = bag_module_->GetBagCapacity(test_role_id_);
    EXPECT_TRUE(bag_module_->ExpandBag(test_role_id_, 10));
    int32_t new_capacity = bag_module_->GetBagCapacity(test_role_id_);
    
    EXPECT_EQ(new_capacity, old_capacity + 10);
}
