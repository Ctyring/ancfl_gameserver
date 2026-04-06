#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <cstring>

#include "ancfl/ancfl.h"
#include "common/message_dispatcher.h"

using namespace std;
using namespace ancfl;
using namespace game_server;

// 自定义 Socket 子类，重写 init 方法
class CustomSocket : public ancfl::Socket {
public:
    typedef std::shared_ptr<CustomSocket> ptr;
    
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
    
    virtual ancfl::Socket::ptr accept() override {
        CustomSocket::ptr sock(new CustomSocket(m_family, m_type, m_protocol));
        // 异步等待连接(hook)
        int newsock = ::accept(m_sock, nullptr, nullptr);
        if (newsock == -1) {
            std::cerr << "[Server] accept errno=" << errno << " errstr=" << strerror(errno) << std::endl;
            return nullptr;
        }
        if (sock->init(newsock)) {
            return sock;
        }
        return nullptr;
    }
};

// 服务器线程
void run_server() {
    // 创建服务器 socket
    CustomSocket::ptr server_socket(new CustomSocket(ancfl::Socket::IPv4, ancfl::Socket::TCP, 0));
    if (!server_socket) {
        cerr << "[Server] Failed to create socket" << endl;
        return;
    }
    
    // 绑定地址
    ancfl::Address::ptr address = ancfl::Address::LookupAnyIPAddress("0.0.0.0:19009");
    if (!address) {
        cerr << "[Server] Failed to create address" << endl;
        return;
    }
    
    if (!server_socket->bind(address)) {
        cerr << "[Server] Failed to bind: " << strerror(errno) << endl;
        return;
    }
    
    // 监听
    if (!server_socket->listen()) {
        cerr << "[Server] Failed to listen: " << strerror(errno) << endl;
        return;
    }
    
    cout << "[Server] Server started on port 19009" << endl;
    
    // 等待客户端连接
    cout << "[Server] Waiting for client connection..." << endl;
    
    // 接受连接
    ancfl::Socket::ptr client_socket = server_socket->accept();
    if (!client_socket) {
        cerr << "[Server] Failed to accept connection: " << strerror(errno) << endl;
        return;
    }
    
    cout << "[Server] Client connected" << endl;
    cout << "[Server] Client socket fd: " << client_socket->getSocket() << endl;
    cout << "[Server] Client socket is connected: " << client_socket->isConnected() << endl;
    
    // 发送测试消息
    string message = "Hello, client!";
    int message_len = message.size();
    
    // 构造消息头
    MessageHeader header;
    header.msg_id = 100006;
    header.msg_len = sizeof(MessageHeader) + message_len;
    header.target_id = 0;
    header.user_data = 0;
    
    // 字节序转换
    header.msg_id = ancfl::byteswapOnLittleEndian(header.msg_id);
    header.msg_len = ancfl::byteswapOnLittleEndian(header.msg_len);
    header.target_id = ancfl::byteswapOnLittleEndian(header.target_id);
    header.user_data = ancfl::byteswapOnLittleEndian(header.user_data);
    
    // 发送消息头
    int sent_header = client_socket->send(&header, sizeof(MessageHeader), 0);
    if (sent_header < 0) {
        cerr << "[Server] Failed to send header: " << strerror(errno) << endl;
    } else {
        cout << "[Server] Sent header, size: " << sent_header << endl;
    }
    
    // 发送消息体
    int sent_data = client_socket->send(message.c_str(), message_len, 0);
    if (sent_data < 0) {
        cerr << "[Server] Failed to send data: " << strerror(errno) << endl;
    } else {
        cout << "[Server] Sent data, size: " << sent_data << endl;
    }
    
    // 等待一段时间后关闭连接
    this_thread::sleep_for(chrono::seconds(5));
    client_socket->close();
    server_socket->close();
    cout << "[Server] Server stopped" << endl;
}

// 客户端函数
void run_client() {
    // 等待服务器启动
    this_thread::sleep_for(chrono::milliseconds(500));
    
    // 创建客户端 socket
    ancfl::Socket::ptr client_socket = ancfl::Socket::CreateTCPSocket();
    if (!client_socket) {
        cerr << "[Client] Failed to create socket" << endl;
        return;
    }
    
    // 连接到服务器
    ancfl::Address::ptr address = ancfl::Address::LookupAnyIPAddress("127.0.0.1:19009");
    if (!address) {
        cerr << "[Client] Failed to create address" << endl;
        return;
    }
    
    if (!client_socket->connect(address)) {
        cerr << "[Client] Failed to connect: " << strerror(errno) << endl;
        return;
    }
    
    cout << "[Client] Connected to server" << endl;
    cout << "[Client] Socket fd: " << client_socket->getSocket() << endl;
    cout << "[Client] Socket is connected: " << client_socket->isConnected() << endl;
    
    // 为 socket 文件描述符创建 FdCtx 条目
    int sock_fd = client_socket->getSocket();
    auto ctx = ancfl::FdMgr::GetInstance()->get(sock_fd, true);
    if (ctx) {
        cout << "[Client] FdCtx created successfully, is_socket: " << ctx->isSocket() << ", is_close: " << ctx->isClose() << endl;
    } else {
        cout << "[Client] Failed to create FdCtx" << endl;
    }
    
    // 等待服务器发送消息
    cout << "[Client] Waiting for server to send message..." << endl;
    this_thread::sleep_for(chrono::milliseconds(1000));
    
    // 测试直接使用系统调用
    cout << "[Client] Testing direct system call recv..." << endl;
    char test_buffer[1024] = {0};
    int test_ret = ::recv(sock_fd, test_buffer, sizeof(test_buffer), 0);
    cout << "[Client] Direct recv ret: " << test_ret << " errno: " << errno << " errstr: " << strerror(errno) << endl;
    if (test_ret > 0) {
        cout << "[Client] Direct recv received: " << test_buffer << endl;
    }
    
    // 测试使用 ancfl Socket::recv 方法，使用 IOManager 注册事件监听器
    cout << "[Client] Testing ancfl Socket::recv with IOManager..." << endl;
    char buffer[1024] = {0};
    
    // 获取 IOManager 实例
    ancfl::IOManager* iom = ancfl::IOManager::GetThis();
    if (iom) {
        // 注册一个事件监听器，当 socket 可读时进行读取
        iom->addEvent(sock_fd, ancfl::IOManager::READ, [&client_socket, &buffer]() {
            int ret = client_socket->recv(buffer, sizeof(buffer), 0);
            cout << "[Client] Socket::recv in event callback ret: " << ret << " errno: " << errno << " errstr: " << strerror(errno) << endl;
            if (ret > 0) {
                cout << "[Client] Socket::recv received: " << buffer << endl;
            }
        });
        
        // 等待一段时间，让事件监听器有机会执行
        this_thread::sleep_for(chrono::milliseconds(1000));
    } else {
        cout << "[Client] Failed to get IOManager instance" << endl;
    }
    
    // 等待一段时间后关闭连接
    this_thread::sleep_for(chrono::seconds(2));
    client_socket->close();
    cout << "[Client] Client stopped" << endl;
}

int main() {
    // 创建 IOManager
    ancfl::IOManager iom;
    
    // 启动服务器线程
    thread server_thread(run_server);
    
    // 启动客户端
    run_client();
    
    // 等待服务器线程退出
    server_thread.join();
    
    return 0;
}
