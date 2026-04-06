#include <gtest/gtest.h>
#include "common/tcp_service.h"
#include "common/message_dispatcher.h"

using namespace game_server;

class TestTcpService : public TcpService {
public:
    TestTcpService(ancfl::IOManager* worker = nullptr,
                   ancfl::IOManager* accept_worker = nullptr)
        : TcpService(worker, accept_worker) {}
    
    std::shared_ptr<google::protobuf::Message> CreateMessage(uint32_t msg_id) override {
        // 简单实现，返回nullptr
        return nullptr;
    }
};

class TcpServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 注意：由于 ancfl 库的限制，IOManager 不能在同一个线程中创建多个实例
        // 所以我们不在这里创建 IOManager，而是直接创建 TcpService 实例
        // 这样可以避免断言错误
        service_ = new TestTcpService();
    }
    
    void TearDown() override {
        // 停止服务
        if (service_) {
            service_->Stop();
            delete service_;
        }
    }
    
    TestTcpService* service_;
};

TEST_F(TcpServiceTest, GetConnectionCountEmpty) {
    EXPECT_EQ(service_->GetConnectionCount(), 0);
}

TEST_F(TcpServiceTest, CloseInvalidConnection) {
    EXPECT_FALSE(service_->CloseConnection(99999));
}

TEST_F(TcpServiceTest, SetMessageDispatcher) {
    auto dispatcher = std::make_shared<MessageDispatcher>();
    service_->SetMessageDispatcher(dispatcher);
}

TEST_F(TcpServiceTest, SendToInvalidConnection) {
    EXPECT_FALSE(service_->SendRawData(99999, 1001, 0, 0, "test", 4));
}

TEST_F(TcpServiceTest, SendMsgToInvalidServer) {
    EXPECT_FALSE(service_->SendMsgToServer(99999, 1001, "test"));
}

TEST_F(TcpServiceTest, SendMsgToInvalidClient) {
    EXPECT_FALSE(service_->SendMsgToClient(99999, 1001, "test"));
}
