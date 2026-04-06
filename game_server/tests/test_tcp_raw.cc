#include <gtest/gtest.h>
#include <thread>
#include <string>
#include <vector>
#include "ancfl/ancfl.h"

using namespace ancfl;

// 简单的服务器线程函数
void run_server() {
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
    address.sin_port = htons(19006);

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

    std::cout << "[Server] Server listening on port 19006" << std::endl;

    // 接受连接并处理
    while (true) {
        int client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (client_fd < 0) {
            std::cerr << "[Server] accept failed: " << strerror(errno) << std::endl;
            continue;
        }

        std::cout << "[Server] Client connected" << std::endl;

        // 发送测试消息
        std::string test_message = "Hello, client! This is a test message.";
        int ret = send(client_fd, test_message.data(), test_message.length(), 0);
        if (ret > 0) {
            std::cout << "[Server] Test message sent successfully" << std::endl;
        } else {
            std::cerr << "[Server] Failed to send test message: " << strerror(errno) << std::endl;
        }

        // 关闭客户端连接
        close(client_fd);
        std::cout << "[Server] Client disconnected" << std::endl;
    }

    // 关闭服务器 socket
    close(server_fd);
}

// 简单的客户端函数
bool run_client() {
    // 创建 socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "[Client] socket creation failed: " << strerror(errno) << std::endl;
        return false;
    }

    // 设置服务器地址
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(19006);

    // 转换 IP 地址
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "[Client] invalid address" << std::endl;
        close(sock);
        return false;
    }

    // 连接到服务器
    std::cout << "[Client] Connecting to server..." << std::endl;
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "[Client] connection failed: " << strerror(errno) << std::endl;
        close(sock);
        return false;
    }

    std::cout << "[Client] Connected to server" << std::endl;

    // 接收服务器消息
    char buffer[1024] = {0};
    int valread = read(sock, buffer, 1024);
    if (valread > 0) {
        std::cout << "[Client] Received message: " << buffer << std::endl;
    } else {
        std::cerr << "[Client] Failed to receive message: " << strerror(errno) << std::endl;
        close(sock);
        return false;
    }

    // 关闭连接
    close(sock);
    std::cout << "[Client] Connection closed" << std::endl;
    return true;
}

// 原始 TCP 通信测试
TEST(TcpRawTest, ClientConnectAndReceiveMessage) {
    // 启动服务器线程
    std::thread server_thread(run_server);
    
    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 运行客户端
    bool success = run_client();
    
    // 等待服务器线程结束
    server_thread.detach();
    
    // 验证测试结果
    ASSERT_TRUE(success) << "Client failed to receive message";
}
