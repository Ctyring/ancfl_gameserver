#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "logic_server/buff_module.h"

using namespace game_server;
using namespace testing;

class MockLogicServiceForBuff : public LogicService {
public:
    MOCK_METHOD(bool, SendToClient, (uint64_t role_id, int32_t msg_id, const std::string& data), (override));
};

class BuffModuleTest : public Test {
protected:
    void SetUp() override {
        mock_service_ = new MockLogicServiceForBuff();
        buff_module_ = new BuffModule(mock_service_);
        test_role_id_ = 12345;
    }
    
    void TearDown() override {
        delete buff_module_;
        delete mock_service_;
    }
    
    MockLogicServiceForBuff* mock_service_;
    BuffModule* buff_module_;
    uint64_t test_role_id_;
};

TEST_F(BuffModuleTest, AddBuff) {
    EXPECT_TRUE(buff_module_->AddBuff(test_role_id_, 1001, 10));
    
    EXPECT_TRUE(buff_module_->HasBuff(test_role_id_, 1001));
}

TEST_F(BuffModuleTest, RemoveBuff) {
    buff_module_->AddBuff(test_role_id_, 1001, 10);
    
    EXPECT_TRUE(buff_module_->RemoveBuff(test_role_id_, 1001));
    EXPECT_FALSE(buff_module_->HasBuff(test_role_id_, 1001));
}

TEST_F(BuffModuleTest, RemoveAllBuffs) {
    buff_module_->AddBuff(test_role_id_, 1001, 10);
    buff_module_->AddBuff(test_role_id_, 1002, 10);
    buff_module_->AddBuff(test_role_id_, 1003, 10);
    
    EXPECT_TRUE(buff_module_->RemoveAllBuffs(test_role_id_));
    
    EXPECT_FALSE(buff_module_->HasBuff(test_role_id_, 1001));
    EXPECT_FALSE(buff_module_->HasBuff(test_role_id_, 1002));
    EXPECT_FALSE(buff_module_->HasBuff(test_role_id_, 1003));
}

TEST_F(BuffModuleTest, GetBuffList) {
    buff_module_->AddBuff(test_role_id_, 1001, 10);
    buff_module_->AddBuff(test_role_id_, 1002, 20);
    
    std::vector<BuffInfo> buffs;
    EXPECT_TRUE(buff_module_->GetBuffList(test_role_id_, buffs));
    EXPECT_EQ(buffs.size(), 2);
}

TEST_F(BuffModuleTest, GetBuffInfo) {
    buff_module_->AddBuff(test_role_id_, 1001, 10);
    
    BuffInfo info;
    EXPECT_TRUE(buff_module_->GetBuffInfo(test_role_id_, 1001, info));
    EXPECT_EQ(info.buff_id, 1001);
    EXPECT_EQ(info.layers, 10);
}

TEST_F(BuffModuleTest, UpdateBuffLayers) {
    buff_module_->AddBuff(test_role_id_, 1001, 10);
    
    EXPECT_TRUE(buff_module_->UpdateBuffLayers(test_role_id_, 1001, 5));
    
    BuffInfo info;
    buff_module_->GetBuffInfo(test_role_id_, 1001, info);
    EXPECT_EQ(info.layers, 15);
}

TEST_F(BuffModuleTest, RefreshBuffDuration) {
    buff_module_->AddBuff(test_role_id_, 1001, 10);
    
    EXPECT_TRUE(buff_module_->RefreshBuffDuration(test_role_id_, 1001));
}

TEST_F(BuffModuleTest, GetBuffAttribute) {
    buff_module_->AddBuff(test_role_id_, 1001, 10);
    
    std::unordered_map<std::string, int32_t> attrs;
    EXPECT_TRUE(buff_module_->GetBuffAttributes(test_role_id_, attrs));
}

TEST_F(BuffModuleTest, OnTimer) {
    buff_module_->AddBuff(test_role_id_, 1001, 10);
    
    buff_module_->OnTimer();
    
    EXPECT_TRUE(buff_module_->HasBuff(test_role_id_, 1001));
}

TEST_F(BuffModuleTest, BuffStacking) {
    buff_module_->AddBuff(test_role_id_, 1001, 10);
    buff_module_->AddBuff(test_role_id_, 1001, 5);
    
    BuffInfo info;
    buff_module_->GetBuffInfo(test_role_id_, 1001, info);
    EXPECT_EQ(info.layers, 15);
}
