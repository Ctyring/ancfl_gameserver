#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "message_dispatcher.h"
#include "proto/msg_base.pb.h"

using namespace game_server;
using namespace testing;

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
    int call_count = 0;
    auto handler = [&call_count](const NetPacket& packet) -> bool {
        call_count++;
        return true;
    };
    
    dispatcher_->RegisterHandler(1001, handler);
    EXPECT_TRUE(dispatcher_->HasHandler(1001));
    EXPECT_EQ(dispatcher_->GetHandlerCount(), 1);
}

TEST_F(MessageDispatcherTest, RegisterMultipleHandlers) {
    auto handler1 = [](const NetPacket& packet) -> bool { return true; };
    auto handler2 = [](const NetPacket& packet) -> bool { return true; };
    
    dispatcher_->RegisterHandler(1001, handler1);
    dispatcher_->RegisterHandler(1002, handler2);
    
    EXPECT_TRUE(dispatcher_->HasHandler(1001));
    EXPECT_TRUE(dispatcher_->HasHandler(1002));
    EXPECT_EQ(dispatcher_->GetHandlerCount(), 2);
}

TEST_F(MessageDispatcherTest, DispatchMessage) {
    int call_count = 0;
    uint32_t received_msg_id = 0;
    
    auto handler = [&call_count, &received_msg_id](const NetPacket& packet) -> bool {
        call_count++;
        received_msg_id = packet.msg_id;
        return true;
    };
    
    dispatcher_->RegisterHandler(1001, handler);
    
    NetPacket packet;
    packet.conn_id = 1;
    packet.msg_id = 1001;
    packet.target_id = 0;
    packet.user_data = 0;
    
    EXPECT_TRUE(dispatcher_->Dispatch(packet));
    EXPECT_EQ(call_count, 1);
    EXPECT_EQ(received_msg_id, 1001);
}

TEST_F(MessageDispatcherTest, DispatchUnknownMessage) {
    int call_count = 0;
    auto handler = [&call_count](const NetPacket& packet) -> bool {
        call_count++;
        return true;
    };
    
    dispatcher_->RegisterHandler(1001, handler);
    
    NetPacket packet;
    packet.conn_id = 1;
    packet.msg_id = 9999;
    packet.target_id = 0;
    packet.user_data = 0;
    
    EXPECT_FALSE(dispatcher_->Dispatch(packet));
    EXPECT_EQ(call_count, 0);
}

TEST_F(MessageDispatcherTest, HandlerReturnsFalse) {
    auto handler = [](const NetPacket& packet) -> bool {
        return false;
    };
    
    dispatcher_->RegisterHandler(1001, handler);
    
    NetPacket packet;
    packet.conn_id = 1;
    packet.msg_id = 1001;
    packet.target_id = 0;
    packet.user_data = 0;
    
    EXPECT_FALSE(dispatcher_->Dispatch(packet));
}

TEST_F(MessageDispatcherTest, HasHandler) {
    auto handler = [](const NetPacket& packet) -> bool { return true; };
    
    EXPECT_FALSE(dispatcher_->HasHandler(1001));
    
    dispatcher_->RegisterHandler(1001, handler);
    EXPECT_TRUE(dispatcher_->HasHandler(1001));
    
    EXPECT_FALSE(dispatcher_->HasHandler(1002));
}

TEST_F(MessageDispatcherTest, GetHandlerCount) {
    auto handler1 = [](const NetPacket& packet) -> bool { return true; };
    auto handler2 = [](const NetPacket& packet) -> bool { return true; };
    
    EXPECT_EQ(dispatcher_->GetHandlerCount(), 0);
    
    dispatcher_->RegisterHandler(1001, handler1);
    EXPECT_EQ(dispatcher_->GetHandlerCount(), 1);
    
    dispatcher_->RegisterHandler(1002, handler2);
    EXPECT_EQ(dispatcher_->GetHandlerCount(), 2);
}

TEST_F(MessageDispatcherTest, OverrideHandler) {
    int call_count1 = 0;
    int call_count2 = 0;
    
    auto handler1 = [&call_count1](const NetPacket& packet) -> bool {
        call_count1++;
        return true;
    };
    
    auto handler2 = [&call_count2](const NetPacket& packet) -> bool {
        call_count2++;
        return true;
    };
    
    dispatcher_->RegisterHandler(1001, handler1);
    dispatcher_->RegisterHandler(1001, handler2);
    
    NetPacket packet;
    packet.conn_id = 1;
    packet.msg_id = 1001;
    packet.target_id = 0;
    packet.user_data = 0;
    
    dispatcher_->Dispatch(packet);
    
    EXPECT_EQ(call_count1, 0);
    EXPECT_EQ(call_count2, 1);
}
