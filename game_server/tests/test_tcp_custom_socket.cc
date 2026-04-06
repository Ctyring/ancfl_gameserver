#include <iostream>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#include "common/tcp_service.h"
#include "common/tcp_client.h"
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

// 简单的测试服务器
class TestTcpServer : public TcpService {
public:
    TestTcpServer(ancfl::IOManager* worker = nullptr,
                  ancfl::IOManager* io_worker = nullptr,
                  ancfl::IOManager* accept_worker = nullptr)
        : TcpService(worker, io_worker, accept_worker),
          test_msg_sent_(false) {}
            
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
                    auto self = std::static_pointer_cast<TestTcpServer>(shared_from_this());
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
            
            // 直接使用 socket->send 发送消息
            MessageHeader header;
            uint32_t msg_len_value = static_cast<uint32_t>(data.size() + sizeof(header));
            std::cout << "[Server] msg_len_value: " << msg_len_value << std::endl;
            header.msg_id = ancfl::byteswapOnLittleEndian(msg_id);
            header.msg_len = ancfl::byteswapOnLittleEndian(msg_len_value);
            header.target_id = ancfl::byteswapOnLittleEndian(0ULL);
            header.user_data = ancfl::byteswapOnLittleEndian(0U);
            
            // 输出消息头的十六进制表示
            std::cout << "[Server] Header hex: ";
            char* header_ptr = reinterpret_cast<char*>(&header);
            for (int i = 0; i < sizeof(header); ++i) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') << (static_cast<unsigned int>(header_ptr[i]) & 0xFF) << " ";
            }
            std::cout << std::dec << std::endl;
            
            // 发送包头
            std::cout << "[Server] Sending header, size: " << sizeof(header) << std::endl;
            int ret = client->send(&header, sizeof(header), 0);
            if (ret <= 0) {
                std::cout << "[Server] Failed to send header, ret: " << ret << " errno: " << errno << " errstr: " << strerror(errno) << std::endl;
            } else {
                std::cout << "[Server] Header sent successfully, ret: " << ret << std::endl;
                
                // 发送消息体
                std::cout << "[Server] Sending data, size: " << data.size() << std::endl;
                ret = client->send(data.data(), data.size(), 0);
                if (ret <= 0) {
                    std::cout << "[Server] Failed to send data, ret: " << ret << " errno: " << errno << " errstr: " << strerror(errno) << std::endl;
                } else {
                    std::cout << "[Server] Data sent successfully, ret: " << ret << std::endl;
                    test_msg_sent_ = true;
                }
            }
        } else {
            std::cout << "[Server] Failed to serialize message" << std::endl;
        }
        
        // 等待一段时间，确保消息发送完成
        std::cout << "[Server] Waiting for message to be sent..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        
        // 不调用父类的 handleClient 方法，保持连接开放
        std::cout << "[Server] Connection kept open" << std::endl;
        
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
    
    bool IsTestMessageSent() const { return test_msg_sent_; }
    
protected:
    virtual std::shared_ptr<google::protobuf::Message> CreateMessage(uint32_t msg_id) override {
        switch (msg_id) {
            case static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_REQ):
                return std::make_shared<AccountLoginReq>();
            case static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_ACK):
                return std::make_shared<AccountLoginAck>();
            default:
                return nullptr;
        }
    }
    
private:
    bool test_msg_sent_;
};

// 全局服务器指针
std::shared_ptr<TestTcpServer> g_test_server;
std::shared_ptr<ancfl::IOManager> g_iom;
std::atomic<bool> g_server_ready(false);

// 服务器线程函数
void RunServer() {
    // 使用 use_caller=false，让 IOManager 创建新线程运行
    // threads=2，创建两个工作线程
    g_iom = std::make_shared<ancfl::IOManager>(2, false, "server");
    
    // 直接在构造函数中传递 IOManager 参数
    g_test_server = std::make_shared<TestTcpServer>(g_iom.get(), g_iom.get(), g_iom.get());
    
    if (!g_test_server->Init("0.0.0.0", 19007)) {
        std::cerr << "[Server] Failed to init test server" << std::endl;
        return;
    }
    
    std::cout << "[Server] Test server started on port 19007" << std::endl;
    g_server_ready = true;
    
    g_test_server->Run();
    
    // 服务器启动后，主线程需要保持运行，直到定时器触发
    // 因为 IOManager 已经在新线程中运行，所以这里需要等待
    std::this_thread::sleep_for(std::chrono::seconds(12));
    
    // 停止服务器
    std::cout << "[Server] Stopping server..." << std::endl;
    if (g_test_server) {
        g_test_server->Stop();
    }
    if (g_iom) {
        g_iom->stop();
    }
    
    std::cout << "[Server] Thread exiting" << std::endl;
}

// 客户端函数
bool RunClient() {
    // 创建客户端
    TcpClient client;
    
    // 连接到服务器
    std::cout << "[Client] Connecting to server..." << std::endl;
    if (!client.Connect("127.0.0.1:19007")) {
        std::cerr << "[Client] Failed to connect to server" << std::endl;
        return false;
    }
    std::cout << "[Client] Connected to server" << std::endl;
    
    // 等待服务器处理连接并发送消息
    std::cout << "[Client] Waiting for server to send message..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    
    // 尝试接收消息
    std::cout << "[Client] Attempting to receive message..." << std::endl;
    std::cout << "[Client] Client connected: " << client.IsConnected() << std::endl;
    std::cout << "[Client] Socket pointer: " << client.GetSocket() << std::endl;
    if (client.GetSocket()) {
        std::cout << "[Client] Socket fd: " << client.GetSocket()->getSocket() << std::endl;
        std::cout << "[Client] Socket is connected: " << client.GetSocket()->isConnected() << std::endl;
    }
    NetPacket packet;
    if (client.RecvMessage(packet)) {
        std::cout << "[Client] Message received successfully!" << std::endl;
        
        // 验证消息
        std::cout << "[Client] Message received, msg_id=" << packet.msg_id << std::endl;
        if (packet.msg_id != static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_ACK)) {
            std::cerr << "[Client] Invalid message id" << std::endl;
            return false;
        }
        
        if (!packet.msg) {
            std::cerr << "[Client] Message object is null" << std::endl;
            return false;
        }
        
        auto login_ack = dynamic_cast<AccountLoginAck*>(packet.msg.get());
        if (!login_ack) {
            std::cerr << "[Client] Failed to cast to AccountLoginAck" << std::endl;
            return false;
        }
        
        std::cout << "[Client] AccountLoginAck received, ret_code=" << login_ack->ret_code() 
                  << ", account_id=" << login_ack->account_id() 
                  << ", last_svr_id=" << login_ack->last_svr_id() 
                  << ", last_svr_name=" << login_ack->last_svr_name() << std::endl;
        
        return true;
    } else {
        std::cout << "[Client] Failed to receive message" << std::endl;
        return false;
    }
}

int main() {
    // 启动服务器线程
    std::thread server_thread(RunServer);
    
    // 等待服务器启动
    while (!g_server_ready) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 运行客户端
    bool success = RunClient();
    
    // 等待服务器线程退出
    if (server_thread.joinable()) {
        server_thread.join();
    }
    
    return success ? 0 : 1;
}
