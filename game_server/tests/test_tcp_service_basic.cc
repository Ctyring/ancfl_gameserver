#include <gtest/gtest.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

#include "common/game_service_base.h"
#include "common/tcp_client.h"
#include "common/game_service_base.h"
#include "proto/msg_account.pb.h"
#include "proto/msg_id.pb.h"

using namespace game_server;

// 自定义 Socket 子类，重写 init 方法
class CustomSocket : public ancfl::Socket {
public:
    CustomSocket(int family, int type, int protocol)
        : ancfl::Socket(family, type, protocol) {}
    
    virtual bool init(int sock) override {
        // 先创建 FdCtx
        auto ctx = ancfl::FdMgr::GetInstance()->get(sock, true);
        if (!ctx || !ctx->isSocket() || ctx->isClose()) {
            return false;
        }
        
        // 然后调用父类的 init 方法
        return ancfl::Socket::init(sock);
    }
};

// 测试游戏服务类，使用 GameServiceBase
class TestGameService : public GameServiceBase {
public:
    TestGameService() : GameServiceBase("test_game_service"),
                       test_msg_sent_(false) {}
    
    // 初始化服务
    virtual bool InitService() override {
        // 初始化网络
        std::string ip = "0.0.0.0";
        int port = 19005;
        if (!Init(ip, port)) {
            std::cerr << "[Server] Failed to init tcp service" << std::endl;
            return false;
        }
        
        // 注册消息处理器
        RegisterAllHandlers();
        
        std::cout << "[Server] TestGameService initialized successfully" << std::endl;
        return true;
    }
    
    // 反初始化服务
    virtual void UninitService() override {
        std::cout << "[Server] TestGameService uninitialized" << std::endl;
    }
    
    // 注册所有消息处理器
    virtual void RegisterAllHandlers() override {
        // 获取消息分发器
        auto dispatcher = dispatcher_;
        if (!dispatcher) {
            dispatcher = std::make_shared<MessageDispatcher>();
            SetMessageDispatcher(dispatcher);
        }
        
        // 注册消息处理器
        dispatcher->RegisterHandler(static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_REQ),
                        std::bind(&TestGameService::OnAccountLoginReq, this,
                                  std::placeholders::_1));
    }
    
    // 重写 startAccept 方法，使用自定义的 CustomSocket
    virtual void startAccept(ancfl::Socket::ptr sock) override {
        std::cout << "[Server] startAccept called" << std::endl;
        while (!isStop()) {
            // 调用原始的 accept 方法
            int newsock = ::accept(sock->getSocket(), nullptr, nullptr);
            if (newsock >= 0) {
                std::cout << "[Server] accept succeeded, newsock=" << newsock << std::endl;
                // 使用自定义的 CustomSocket
                auto client = std::make_shared<CustomSocket>(sock->getFamily(), sock->getType(), sock->getProtocol());
                if (client->init(newsock)) {
                    std::cout << "[Server] client init succeeded" << std::endl;
                    client->setRecvTimeout(getRecvTimeout());
                    auto self = std::static_pointer_cast<TestGameService>(shared_from_this());
                    std::cout << "[Server] Scheduling handleClient" << std::endl;
                    m_ioWorker->schedule([self, client]() {
                        std::cout << "[Server] Executing handleClient" << std::endl;
                        self->handleClient(client);
                    });
                } else {
                    std::cout << "[Server] client init failed" << std::endl;
                    close(newsock);
                }
            } else {
                std::cout << "[Server] accept failed, errno=" << errno << " errstr=" << strerror(errno) << std::endl;
                // 等待一段时间后重试
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
            
    // 当客户端连接时调用
    virtual void handleClient(ancfl::Socket::ptr client) override {
        std::cout << "[Server] Executing handleClient" << std::endl;
        
        // 先添加连接到 connections_ 映射
        int32_t conn_id;
        {
            ancfl::Mutex::Lock lock(conn_mutex_);
            conn_id = next_conn_id_++;
            connections_[conn_id] = client;
            last_heart_time_[conn_id] = time(nullptr);
        }
        
        // 发送一条测试消息给客户端
        AccountLoginAck ack;
        ack.set_ret_code(0);
        ack.set_account_id(12345);
        ack.set_last_svr_id(1);
        ack.set_last_svr_name("TestServer");
        
        std::string data;
        if (ack.SerializeToString(&data)) {
            uint32_t msg_id = static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_ACK);
            std::cout << "[Server] Sending message, msg_id=" << msg_id << ", data size=" << data.size() << std::endl;
            std::cout << "[Server] connections_ size: " << connections_.size() << std::endl;
            std::cout << "[Server] conn_id: " << conn_id << std::endl;
            
            // 使用 SendRawData 发送消息，它会正确处理消息头和大小端转换
            if (SendRawData(conn_id, msg_id, 0, 0, data.data(), data.size())) {
                std::cout << "[Server] Test message sent successfully" << std::endl;
                test_msg_sent_ = true;
            } else {
                std::cout << "[Server] Failed to send test message" << std::endl;
            }
        } else {
            std::cout << "[Server] Failed to serialize message" << std::endl;
        }
        
        // 等待一段时间，确保消息发送完成
        std::cout << "[Server] Waiting for message to be sent..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        
        // 不调用父类的 handleClient 方法，保持连接开放
        std::cout << "[Server] Connection kept open" << std::endl;
        
        // 启动接收协程，用于处理客户端的消息
        if (m_worker) {
            m_worker->schedule(std::bind(&TestGameService::HandleRecv, this, client, conn_id));
        }
        
        // 等待一段时间，确保客户端有足够的时间接收消息
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        
        // 手动关闭连接
        std::cout << "[Server] Closing connection" << std::endl;
        {
            ancfl::Mutex::Lock lock(conn_mutex_);
            connections_.erase(conn_id);
            last_heart_time_.erase(conn_id);
        }
    }
    
    // 处理接收消息
    void HandleRecv(ancfl::Socket::ptr client, int32_t conn_id) {
        std::vector<char> buffer(8192);
        while (true) {
            // 接收包头
            MessageHeader header;
            int ret = client->recv(&header, sizeof(header));
            if (ret <= 0) {
                break;
            }
            
            // 大小端转换
            header.msg_id = ancfl::byteswapOnLittleEndian(header.msg_id);
            header.msg_len = ancfl::byteswapOnLittleEndian(header.msg_len);
            header.target_id = ancfl::byteswapOnLittleEndian(header.target_id);
            header.user_data = ancfl::byteswapOnLittleEndian(header.user_data);
            
            if (header.msg_len > 32768 || header.msg_len < sizeof(header)) {
                break;
            }
            
            // 接收消息体
            uint32_t body_len = header.msg_len - sizeof(header);
            if (body_len > 0) {
                if (buffer.size() < body_len) {
                    buffer.resize(body_len);
                }
                
                ret = client->recv(buffer.data(), body_len);
                if (ret <= 0) {
                    break;
                }
            }
        }
    }
    
    // 处理账号登录请求
    bool OnAccountLoginReq(const NetPacket& packet) {
        std::cout << "[Server] OnAccountLoginReq called" << std::endl;
        
        // 发送登录响应
        AccountLoginAck ack;
        ack.set_ret_code(0);
        ack.set_account_id(67890);
        ack.set_last_svr_id(1);
        ack.set_last_svr_name("TestServer");
        
        std::string data;
        if (ack.SerializeToString(&data)) {
            SendMsgToClient(packet.conn_id,
                            static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_ACK),
                            data);
        }
        
        return true;
    }
    
    bool IsTestMessageSent() const { return test_msg_sent_; }
    
private:
    bool test_msg_sent_;
};

// 全局服务指针
std::shared_ptr<TestGameService> g_test_service;
std::shared_ptr<ancfl::IOManager> g_iom;
std::atomic<bool> g_service_ready(false);

// 服务线程函数
void RunService() {
    // 使用 use_caller=false，让 IOManager 创建新线程运行
    // threads=2，创建两个工作线程
    g_iom = std::make_shared<ancfl::IOManager>(2, false, "service");
    
    // 创建服务实例
    g_test_service = std::make_shared<TestGameService>();
    
    // 设置 IOManager
    g_test_service->SetIOManager(g_iom.get());
    g_test_service->SetWorkerPool(g_iom.get());
    
    // 初始化服务
    if (!g_test_service->InitService()) {
        std::cerr << "[Server] Failed to init test service" << std::endl;
        return;
    }
    
    std::cout << "[Server] TestGameService started on port 19005" << std::endl;
    g_service_ready = true;
    
    // 运行服务
    g_test_service->Run();
    
    // 服务启动后，主线程需要保持运行，直到定时器触发
    // 因为 IOManager 已经在新线程中运行，所以这里需要等待
    std::this_thread::sleep_for(std::chrono::seconds(12));
    
    // 停止服务
    std::cout << "[Server] Stopping service..." << std::endl;
    if (g_test_service) {
        g_test_service->StopService();
    }
    if (g_iom) {
        g_iom->stop();
    }
    
    std::cout << "[Server] Thread exiting" << std::endl;
}

// TCP 服务基础测试
class TcpServiceBasicTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 启动服务线程
        service_thread_ = std::thread(RunService);
        
        // 等待服务启动
        while (!g_service_ready) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    void TearDown() override {
        // 停止服务
        if (g_test_service) {
            g_test_service->StopService();
        }
        if (g_iom) {
            g_iom->stop();
        }
        
        if (service_thread_.joinable()) {
            service_thread_.join();
        }
    }
    
    std::thread service_thread_;
};

// 测试客户端连接并接收消息
TEST_F(TcpServiceBasicTest, ClientConnectAndReceiveMessage) {
    // 创建客户端
    TcpClient client;
    
    // 连接到服务
    std::cout << "[Client] Connecting to service..." << std::endl;
    ASSERT_TRUE(client.Connect("127.0.0.1:19005")) << "Failed to connect to service";
    std::cout << "[Client] Connected to service" << std::endl;
    
    // 等待服务处理连接并发送消息
    std::cout << "[Client] Waiting for service to send message..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    
    // 尝试接收消息
    std::cout << "[Client] Attempting to receive message..." << std::endl;
    NetPacket packet;
    bool received = false;
    
    // 尝试多次接收消息，最多尝试 5 次
    for (int i = 0; i < 5; ++i) {
        std::cout << "[Client] Attempt " << i+1 << " to receive message" << std::endl;
        if (client.RecvMessage(packet)) {
            std::cout << "[Client] Message received successfully!" << std::endl;
            received = true;
            break;
        }
        std::cout << "[Client] Retrying to receive message..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
    ASSERT_TRUE(received) << "Failed to receive message";
    
    // 验证消息
    std::cout << "[Client] Message received, msg_id=" << packet.msg_id << std::endl;
    EXPECT_EQ(packet.msg_id, static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_ACK));
    
    ASSERT_TRUE(packet.msg != nullptr) << "Message object is null";
    auto login_ack = dynamic_cast<AccountLoginAck*>(packet.msg.get());
    ASSERT_TRUE(login_ack != nullptr) << "Failed to cast to AccountLoginAck";
    
    // 验证消息内容
    EXPECT_EQ(login_ack->ret_code(), 0);
    EXPECT_EQ(login_ack->account_id(), 12345);
    EXPECT_EQ(login_ack->last_svr_id(), 1);
    EXPECT_EQ(login_ack->last_svr_name(), "TestServer");
    
    std::cout << "[Client] Test message verified successfully!" << std::endl;
    std::cout << "  ret_code: " << login_ack->ret_code() << std::endl;
    std::cout << "  account_id: " << login_ack->account_id() << std::endl;
    std::cout << "  last_svr_id: " << login_ack->last_svr_id() << std::endl;
    std::cout << "  last_svr_name: " << login_ack->last_svr_name() << std::endl;
    
    // 断开连接
    client.Disconnect();
    std::cout << "[Client] Disconnected" << std::endl;
}

// 测试服务发送消息
TEST_F(TcpServiceBasicTest, ServiceSendMessage) {
    // 创建客户端
    TcpClient client;
    
    // 连接到服务
    ASSERT_TRUE(client.Connect("127.0.0.1:19005")) << "Failed to connect to service";
    
    // 等待服务发送消息
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 验证服务是否发送了消息
    EXPECT_TRUE(g_test_service->IsTestMessageSent()) << "Service did not send test message";
    
    client.Disconnect();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
