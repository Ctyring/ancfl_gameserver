#include <gtest/gtest.h>
#include "logic_server/bag_module.h"
#include "logic_server/logic_service.h"

using namespace game_server;

class BagModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        service_ = nullptr;
        bag_module_ = new BagModule(service_);
        test_role_id_ = 123456;
        bag_module_->InitBag(test_role_id_);
    }
    
    void TearDown() override {
        delete bag_module_;
    }
    
    LogicService* service_;
    BagModule* bag_module_;
    uint64_t test_role_id_;
};

TEST_F(BagModuleTest, InitBag) {
    EXPECT_TRUE(bag_module_->InitBag(test_role_id_ + 1));
}

TEST_F(BagModuleTest, GetBag) {
    std::vector<BagSlot> slots;
    EXPECT_TRUE(bag_module_->GetBag(test_role_id_, slots));
}

TEST_F(BagModuleTest, AddItem) {
    EXPECT_TRUE(bag_module_->AddItem(test_role_id_, 1001, 10));
}

TEST_F(BagModuleTest, GetItemCount) {
    bag_module_->AddItem(test_role_id_, 1001, 10);
    
    int32_t count = 0;
    EXPECT_TRUE(bag_module_->GetItemCount(test_role_id_, 1001, count));
    EXPECT_EQ(count, 10);
}

TEST_F(BagModuleTest, HasItem) {
    bag_module_->AddItem(test_role_id_, 1001, 10);
    
    EXPECT_TRUE(bag_module_->HasItem(test_role_id_, 1001, 5));
    EXPECT_TRUE(bag_module_->HasItem(test_role_id_, 1001, 10));
    EXPECT_FALSE(bag_module_->HasItem(test_role_id_, 1001, 20));
}

TEST_F(BagModuleTest, RemoveItem) {
    bag_module_->AddItem(test_role_id_, 1001, 10);
    
    int32_t count = 0;
    bag_module_->GetItemCount(test_role_id_, 1001, count);
    EXPECT_EQ(count, 10);
}

TEST_F(BagModuleTest, GetBagSlot) {
    bag_module_->AddItem(test_role_id_, 1001, 10);
    
    BagSlot slot;
    EXPECT_TRUE(bag_module_->GetBagSlot(test_role_id_, 0, slot));
}

TEST_F(BagModuleTest, MoveItem) {
    bag_module_->AddItem(test_role_id_, 1001, 10);
    EXPECT_TRUE(bag_module_->MoveItem(test_role_id_, 0, 1));
}

TEST_F(BagModuleTest, SwapItem) {
    bag_module_->AddItem(test_role_id_, 1001, 10);
    bag_module_->AddItem(test_role_id_, 1002, 5);
    EXPECT_TRUE(bag_module_->SwapItem(test_role_id_, 0, 1));
}

TEST_F(BagModuleTest, ExpandBag) {
    EXPECT_TRUE(bag_module_->ExpandBag(test_role_id_, 10));
}
