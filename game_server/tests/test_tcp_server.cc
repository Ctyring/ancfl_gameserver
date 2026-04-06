#include <iostream>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <ctime>

#include "common/tcp_service.h"
#include "common/game_service_base.h"
#include "proto/msg_account.pb.h"
#include "proto/msg_id.pb.h"

// 日志宏，添加时间戳
#define LOG_INFO(msg) do { \
    time_t now = time(nullptr); \
    struct tm* tm_info = localtime(&now); \
    char time_str[20]; \
    strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", tm_info); \
    std::cout << "[" << time_str << "] [Server] " << msg << std::endl; \
} while(0)

#define LOG_ERROR(msg) do { \
    time_t now = time(nullptr); \
    struct tm* tm_info = localtime(&now); \
    char time_str[20]; \
    strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", tm_info); \
    std::cerr << "[" << time_str << "] [Server] ERROR: " << msg << std::endl; \
} while(0)

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
        LOG_INFO("Initializing network on " << ip << ":" << port);
        if (!Init(ip, port)) {
            LOG_ERROR("Failed to init tcp service");
            return false;
        }
        
        // 注册消息处理器
        RegisterAllHandlers();
        
        LOG_INFO("TestGameService initialized successfully");
        return true;
    }
    
    // 反初始化服务
    virtual void UninitService() override {
        LOG_INFO("TestGameService uninitialized");
    }
    
    // 注册所有消息处理器
    virtual void RegisterAllHandlers() override {
        // 获取消息分发器
        auto dispatcher = dispatcher_;
        if (!dispatcher) {
            LOG_INFO("Creating new MessageDispatcher");
            dispatcher = std::make_shared<MessageDispatcher>();
            SetMessageDispatcher(dispatcher);
            LOG_INFO("MessageDispatcher set successfully");
        }
        
        // 由于我们在 HandleRecv 中直接处理消息，这里不需要注册处理器
        // 但为了保持代码结构完整，保留此方法
        LOG_INFO("All handlers registered");
    }
    
    // 重写 startAccept 方法，使用自定义的 CustomSocket
    virtual void startAccept(ancfl::Socket::ptr sock) override {
        LOG_INFO("startAccept called, socket: " << sock->toString());
        while (!isStop()) {
            // 调用原始的 accept 方法
            LOG_INFO("Waiting for client connection...");
            int newsock = ::accept(sock->getSocket(), nullptr, nullptr);
            if (newsock >= 0) {
                LOG_INFO("accept succeeded, newsock=" << newsock);
                // 使用自定义的 CustomSocket
                auto client = std::make_shared<CustomSocket>(sock->getFamily(), sock->getType(), sock->getProtocol());
                if (client->init(newsock)) {
                    LOG_INFO("client init succeeded");
                    client->setRecvTimeout(getRecvTimeout());
                    auto self = std::static_pointer_cast<TestGameService>(shared_from_this());
                    LOG_INFO("Scheduling handleClient");
                    m_ioWorker->schedule([self, client]() {
                        LOG_INFO("Executing handleClient");
                        self->handleClient(client);
                    });
                } else {
                    LOG_ERROR("client init failed");
                    close(newsock);
                }
            } else {
                LOG_ERROR("accept failed, errno=" << errno << " errstr=" << strerror(errno));
                // 等待一段时间后重试
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        LOG_INFO("startAccept exited");
    }
            
    // 当客户端连接时调用
    virtual void handleClient(ancfl::Socket::ptr client) override {
        LOG_INFO("Executing handleClient for client: " << client->toString());
        
        // 先添加连接到 connections_ 映射
        int32_t conn_id;
        {
            ancfl::Mutex::Lock lock(conn_mutex_);
            conn_id = next_conn_id_++;
            connections_[conn_id] = client;
            last_heart_time_[conn_id] = time(nullptr);
            LOG_INFO("Added connection, conn_id=" << conn_id << ", connections size=" << connections_.size());
        }
        
        // 发送一条测试消息给客户端
        LOG_INFO("Preparing to send test message to client");
        AccountLoginAck ack;
        ack.set_ret_code(0);
        ack.set_account_id(12345);
        ack.set_last_svr_id(1);
        ack.set_last_svr_name("TestServer");
        
        std::string data;
        if (ack.SerializeToString(&data)) {
            uint32_t msg_id = static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_ACK);
            LOG_INFO("Sending message, msg_id=" << msg_id << ", data size=" << data.size());
            LOG_INFO("connections_ size: " << connections_.size());
            LOG_INFO("conn_id: " << conn_id);
            
            // 使用 SendMsgToClient 发送消息，它会正确处理消息头和大小端转换
            if (SendMsgToClient(conn_id, msg_id, data)) {
                LOG_INFO("Test message sent successfully");
                test_msg_sent_ = true;
            } else {
                LOG_ERROR("Failed to send test message");
            }
        } else {
            LOG_ERROR("Failed to serialize message");
        }
        
        // 等待一段时间，确保消息发送完成
        LOG_INFO("Waiting for message to be sent...");
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        
        // 不调用父类的 handleClient 方法，保持连接开放
        LOG_INFO("Connection kept open");
        
        // 启动接收协程，用于处理客户端的消息
        if (m_worker) {
            LOG_INFO("Scheduling HandleRecv coroutine");
            m_worker->schedule(std::bind(&TestGameService::HandleRecv, this, client, conn_id));
        } else {
            LOG_ERROR("m_worker is null, cannot schedule HandleRecv");
        }
        
        // 等待一段时间，确保客户端有足够的时间接收消息和发送消息
        LOG_INFO("Waiting for client messages for 10 seconds...");
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        
        // 手动关闭连接
        LOG_INFO("Closing connection, conn_id=" << conn_id);
        {
            ancfl::Mutex::Lock lock(conn_mutex_);
            connections_.erase(conn_id);
            last_heart_time_.erase(conn_id);
            LOG_INFO("Connection removed, connections size=" << connections_.size());
        }
    }
    
    // 处理接收消息
    void HandleRecv(ancfl::Socket::ptr client, int32_t conn_id) {
        LOG_INFO("HandleRecv started for conn_id=" << conn_id);
        std::vector<char> buffer(8192);
        while (true) {
            // 接收包头
            MessageHeader header;
            LOG_INFO("Waiting for message header...");
            int ret = client->recv(&header, sizeof(header));
            if (ret <= 0) {
                LOG_INFO("recv header failed, ret=" << ret << ", exiting HandleRecv");
                break;
            }
            LOG_INFO("Received header, ret=" << ret << " bytes");
            
            // 大小端转换
            header.msg_id = ancfl::byteswapOnLittleEndian(header.msg_id);
            header.msg_len = ancfl::byteswapOnLittleEndian(header.msg_len);
            header.target_id = ancfl::byteswapOnLittleEndian(header.target_id);
            header.user_data = ancfl::byteswapOnLittleEndian(header.user_data);
            
            LOG_INFO("Header info - msg_id=" << header.msg_id << ", msg_len=" << header.msg_len 
                     << ", target_id=" << header.target_id << ", user_data=" << header.user_data);
            
            if (header.msg_len > 32768 || header.msg_len < sizeof(header)) {
                LOG_ERROR("Invalid message length: " << header.msg_len);
                break;
            }
            
            // 接收消息体
            uint32_t body_len = header.msg_len - sizeof(header);
            if (body_len > 0) {
                if (buffer.size() < body_len) {
                    buffer.resize(body_len);
                    LOG_INFO("Resized buffer to " << body_len << " bytes");
                }
                
                LOG_INFO("Waiting for message body, body_len=" << body_len);
                ret = client->recv(buffer.data(), body_len);
                if (ret <= 0) {
                    LOG_INFO("recv body failed, ret=" << ret << ", exiting HandleRecv");
                    break;
                }
                LOG_INFO("Received body, ret=" << ret << " bytes");
                
                // 处理消息
                LOG_INFO("Received message, msg_id=" << header.msg_id << ", body_len=" << body_len);
                
                // 如果是登录请求，调用消息处理器
                if (header.msg_id == static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_REQ)) {
                    // 直接反序列化消息并处理
                    AccountLoginReq login_req;
                    if (login_req.ParseFromArray(buffer.data(), body_len)) {
                        LOG_INFO("Received login request for user: " << login_req.account_name());
                        
                        // 发送登录响应
                        AccountLoginAck ack;
                        ack.set_ret_code(0);
                        ack.set_account_id(67890);
                        ack.set_last_svr_id(1);
                        ack.set_last_svr_name("TestServer");
                        
                        std::string data;
                        if (ack.SerializeToString(&data)) {
                            LOG_INFO("Sending login response, data size=" << data.size());
                            SendMsgToClient(conn_id,
                                            static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_ACK),
                                            data);
                            LOG_INFO("Login response sent");
                        } else {
                            LOG_ERROR("Failed to serialize login response");
                        }
                    } else {
                        LOG_ERROR("Failed to parse login request");
                    }
                } else {
                    LOG_INFO("Received unknown message type: " << header.msg_id);
                }
            }
        }
        LOG_INFO("HandleRecv exited for conn_id=" << conn_id);
    }
    
    bool IsTestMessageSent() const { return test_msg_sent_; }
    
private:
    bool test_msg_sent_;
};

int main(int argc, char** argv) {
    LOG_INFO("Starting TCP server...");
    
    // 创建 IOManager
    LOG_INFO("Creating IOManager with 2 threads");
    auto iom = std::make_shared<ancfl::IOManager>(2, false, "service");
    
    // 创建服务实例
    LOG_INFO("Creating TestGameService instance");
    auto service = std::make_shared<TestGameService>();
    
    // 设置 IOManager
    LOG_INFO("Setting IOManager");
    service->SetIOManager(iom.get(), iom.get());
    
    // 初始化服务
    LOG_INFO("Initializing service");
    if (!service->InitService()) {
        LOG_ERROR("Failed to init test service");
        return 1;
    }
    
    LOG_INFO("TestGameService started on port 19005");
    
    // 运行服务
    LOG_INFO("Running service");
    service->Run();
    
    // 服务启动后，主线程需要保持运行，直到定时器触发
    // 因为 IOManager 已经在新线程中运行，所以这里需要等待
    LOG_INFO("Main thread waiting for 60 seconds...");
    std::this_thread::sleep_for(std::chrono::seconds(60));
    
    // 停止服务
    LOG_INFO("Stopping service...");
    if (service) {
        service->StopService();
    }
    if (iom) {
        iom->stop();
    }
    
    LOG_INFO("Server exiting");
    return 0;
}
