#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "tcp_service.h"

using namespace game_server;
using namespace testing;

class MockTcpCallback : public TcpCallback {
public:
    MOCK_METHOD(void, OnConnect, (uint64_t conn_id), (override));
    MOCK_METHOD(void, OnDisconnect, (uint64_t conn_id), (override));
    MOCK_METHOD(void, OnMessage, (uint64_t conn_id, const char* data, size_t len), (override));
};

class TcpServiceTest : public Test {
protected:
    void SetUp() override {
        service_ = new TcpService();
    }
    
    void TearDown() override {
        delete service_;
    }
    
    TcpService* service_;
};

TEST_F(TcpServiceTest, Init) {
    EXPECT_TRUE(service_->Init("0.0.0.0", 9999));
}

TEST_F(TcpServiceTest, SetCallback) {
    auto callback = std::make_shared<MockTcpCallback>();
    service_->SetCallback(callback);
    
    EXPECT_NE(service_->GetCallback(), nullptr);
}

TEST_F(TcpServiceTest, StartStop) {
    EXPECT_TRUE(service_->Init("0.0.0.0", 9999));
    EXPECT_TRUE(service_->Start());
    service_->Stop();
}

TEST_F(TcpServiceTest, SendToInvalidConnection) {
    std::string data = "test";
    EXPECT_FALSE(service_->Send(99999, data.data(), data.size()));
}

TEST_F(TcpServiceTest, CloseInvalidConnection) {
    EXPECT_FALSE(service_->CloseConnection(99999));
}

TEST_F(TcpServiceTest, GetConnectionCount) {
    EXPECT_EQ(service_->GetConnectionCount(), 0);
}

TEST_F(TcpServiceTest, GetConnectionInfo) {
    ConnectionInfo info;
    EXPECT_FALSE(service_->GetConnectionInfo(99999, info));
}

TEST_F(TcpServiceTest, BroadcastEmpty) {
    std::string data = "test";
    EXPECT_EQ(service_->Broadcast(data.data(), data.size()), 0);
}

TEST_F(TcpServiceTest, SetMaxConnections) {
    service_->SetMaxConnections(100);
    EXPECT_EQ(service_->GetMaxConnections(), 100);
}

TEST_F(TcpServiceTest, SetBufferSize) {
    service_->SetRecvBufferSize(4096);
    service_->SetSendBufferSize(4096);
}
