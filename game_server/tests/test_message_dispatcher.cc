#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "message_dispatcher.h"

using namespace game_server;
using namespace testing;

class MockMessageHandler : public MessageHandler {
public:
    MOCK_METHOD(void, Handle, (uint64_t conn_id, const std::string& data), (override));
};

class MessageDispatcherTest : public Test {
protected:
    void SetUp() override {
        dispatcher_ = new MessageDispatcher();
    }
    
    void TearDown() override {
        delete dispatcher_;
    }
    
    MessageDispatcher* dispatcher_;
};

TEST_F(MessageDispatcherTest, RegisterHandler) {
    auto handler = std::make_shared<MockMessageHandler>();
    
    EXPECT_TRUE(dispatcher_->RegisterHandler(1001, handler));
    EXPECT_FALSE(dispatcher_->RegisterHandler(1001, handler));
}

TEST_F(MessageDispatcherTest, UnregisterHandler) {
    auto handler = std::make_shared<MockMessageHandler>();
    
    dispatcher_->RegisterHandler(1001, handler);
    EXPECT_TRUE(dispatcher_->UnregisterHandler(1001));
    EXPECT_FALSE(dispatcher_->UnregisterHandler(1001));
}

TEST_F(MessageDispatcherTest, DispatchMessage) {
    auto handler = std::make_shared<MockMessageHandler>();
    dispatcher_->RegisterHandler(1001, handler);
    
    EXPECT_CALL(*handler, Handle(1, testing::_))
        .Times(1);
    
    dispatcher_->DispatchMessage(1001, 1, "test_data");
}

TEST_F(MessageDispatcherTest, DispatchUnknownMessage) {
    auto handler = std::make_shared<MockMessageHandler>();
    dispatcher_->RegisterHandler(1001, handler);
    
    EXPECT_CALL(*handler, Handle(testing::_, testing::_))
        .Times(0);
    
    dispatcher_->DispatchMessage(9999, 1, "test_data");
}

TEST_F(MessageDispatcherTest, GetHandlerCount) {
    auto handler1 = std::make_shared<MockMessageHandler>();
    auto handler2 = std::make_shared<MockMessageHandler>();
    
    EXPECT_EQ(dispatcher_->GetHandlerCount(), 0);
    
    dispatcher_->RegisterHandler(1001, handler1);
    EXPECT_EQ(dispatcher_->GetHandlerCount(), 1);
    
    dispatcher_->RegisterHandler(1002, handler2);
    EXPECT_EQ(dispatcher_->GetHandlerCount(), 2);
    
    dispatcher_->UnregisterHandler(1001);
    EXPECT_EQ(dispatcher_->GetHandlerCount(), 1);
}

TEST_F(MessageDispatcherTest, ClearAllHandlers) {
    auto handler1 = std::make_shared<MockMessageHandler>();
    auto handler2 = std::make_shared<MockMessageHandler>();
    
    dispatcher_->RegisterHandler(1001, handler1);
    dispatcher_->RegisterHandler(1002, handler2);
    
    EXPECT_EQ(dispatcher_->GetHandlerCount(), 2);
    
    dispatcher_->ClearAllHandlers();
    EXPECT_EQ(dispatcher_->GetHandlerCount(), 0);
}
