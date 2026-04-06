#include <gtest/gtest.h>
#include <thread>
#include <string>
#include <vector>
#include "ancfl/ancfl.h"
#include "common/message_dispatcher.h"

using namespace ancfl;
using namespace game_server;

// 简单的服务器线程函数
void run_socket_server() {
    // 创建 socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "[Server] socket creation failed: " << strerror(errno) << std::endl;
        return;
    }

    // 设置 socket 选项
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        std::cerr << "[Server] setsockopt failed: " << strerror(errno) << std::endl;
        close(server_fd);
        return;
    }

    // 绑定地址
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(19007);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "[Server] bind failed: " << strerror(errno) << std::endl;
        close(server_fd);
        return;
    }

    // 监听连接
    if (listen(server_fd, 3) < 0) {
        std::cerr << "[Server] listen failed: " << strerror(errno) << std::endl;
        close(server_fd);
        return;
    }

    std::cout << "[Server] Server listening on port 19007" << std::endl;

    // 接受连接并处理
    while (true) {
        int client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (client_fd < 0) {
            std::cerr << "[Server] accept failed: " << strerror(errno) << std::endl;
            continue;
        }

        std::cout << "[Server] Client connected" << std::endl;

        // 构造消息头
        MessageHeader header;
        header.msg_id = 100006; // MSG_ACCOUNT_LOGIN_ACK
        std::string body_data = "\x08\x00\x10\x91\xa3\x01\x18\x01\x22\x0cTestServer";
        header.msg_len = body_data.size(); // 消息体大小
        header.target_id = 0;
        header.user_data = 0;

        // 发送消息头
        int ret = send(client_fd, &header, sizeof(header), 0);
        if (ret > 0) {
            std::cout << "[Server] Header sent successfully, ret=" << ret << std::endl;
        } else {
            std::cerr << "[Server] Failed to send header: " << strerror(errno) << std::endl;
        }

        // 发送消息体（模拟 AccountLoginAck）
        // 实际的 protobuf 序列化数据
        std::cout << "[Server] Body data size: " << body_data.size() << std::endl;
        ret = send(client_fd, body_data.data(), body_data.size(), 0);
        if (ret > 0) {
            std::cout << "[Server] Body sent successfully, ret=" << ret << std::endl;
        } else {
            std::cerr << "[Server] Failed to send body: " << strerror(errno) << std::endl;
        }

        // 等待一段时间，然后关闭连接
        std::this_thread::sleep_for(std::chrono::seconds(2));
        close(client_fd);
        std::cout << "[Server] Client disconnected" << std::endl;
    }

    // 关闭服务器 socket
    close(server_fd);
}

// 使用 ancfl Socket 类的客户端函数
bool run_ancfl_client() {
    // 创建 socket
    Socket::ptr sock = Socket::CreateTCP(Address::LookupAnyIPAddress("127.0.0.1:19007"));
    if (!sock) {
        std::cerr << "[Client] Failed to create socket" << std::endl;
        return false;
    }

    // 连接到服务器
    std::cout << "[Client] Connecting to server..." << std::endl;
    if (!sock->connect(Address::LookupAnyIPAddress("127.0.0.1:19007"))) {
        std::cerr << "[Client] Connection failed" << std::endl;
        return false;
    }

    std::cout << "[Client] Connected to server" << std::endl;

    // 接收消息头
    MessageHeader header;
    int ret = sock->recv(&header, sizeof(header), 0);
    if (ret != sizeof(header)) {
        std::cerr << "[Client] Failed to receive header, ret: " << ret << std::endl;
        return false;
    }

    std::cout << "[Client] Header received, ret=" << ret << std::endl;
    std::cout << "  msg_id: " << header.msg_id << std::endl;
    std::cout << "  msg_len: " << header.msg_len << std::endl;
    std::cout << "  target_id: " << header.target_id << std::endl;
    std::cout << "  user_data: " << header.user_data << std::endl;

    // 接收消息体
    std::vector<char> body(header.msg_len);
    ret = sock->recv(body.data(), body.size(), 0);
    if (ret != static_cast<int>(body.size())) {
        std::cerr << "[Client] Failed to receive body, ret: " << ret << std::endl;
        return false;
    }

    std::cout << "[Client] Body received, ret=" << ret << std::endl;
    std::cout << "  Body data: ";
    for (char c : body) {
        std::cout << "0x" << std::hex << (unsigned char)c << " ";
    }
    std::cout << std::endl;

    // 关闭连接
    sock->close();
    std::cout << "[Client] Connection closed" << std::endl;
    return true;
}

// 原始 TCP 通信测试
TEST(TcpSocketTest, ClientConnectAndReceiveMessage) {
    // 启动服务器线程
    std::thread server_thread(run_socket_server);
    
    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 运行客户端
    bool success = run_ancfl_client();
    
    // 等待服务器线程结束
    server_thread.detach();
    
    // 验证测试结果
    ASSERT_TRUE(success) << "Client failed to receive message";
}
