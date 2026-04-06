#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// 消息头结构
struct MessageHeader {
    uint32_t msg_id;
    uint32_t msg_len;
    uint64_t target_id;
    uint32_t user_data;
} __attribute__((packed));

// 服务器线程
void run_server() {
    // 创建 socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "[Server] Failed to create socket: " << strerror(errno) << std::endl;
        return;
    }

    // 设置地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(19006);

    // 绑定
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[Server] Failed to bind: " << strerror(errno) << std::endl;
        close(server_fd);
        return;
    }

    // 监听
    if (listen(server_fd, 5) < 0) {
        std::cerr << "[Server] Failed to listen: " << strerror(errno) << std::endl;
        close(server_fd);
        return;
    }

    std::cout << "[Server] Server listening on port 19006" << std::endl;

    // 接受连接
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        std::cerr << "[Server] Failed to accept: " << strerror(errno) << std::endl;
        close(server_fd);
        return;
    }

    std::cout << "[Server] Client connected" << std::endl;

    // 构造消息头
    MessageHeader header;
    header.msg_id = htonl(100006); // MSG_ACCOUNT_LOGIN_ACK
    header.msg_len = htonl(17 + sizeof(header)); // 消息体大小 + 消息头大小
    header.target_id = htonl(0);
    header.user_data = htonl(0);

    // 发送消息头
    int ret = send(client_fd, &header, sizeof(header), 0);
    if (ret > 0) {
        std::cout << "[Server] Header sent successfully, ret=" << ret << std::endl;
    } else {
        std::cerr << "[Server] Failed to send header: " << strerror(errno) << std::endl;
    }

    // 发送消息体（模拟 AccountLoginAck）
    // 实际的 protobuf 序列化数据
    std::string body_data = "\x08\x00\x10\x91\xa3\x01\x18\x01\x22\x0cTestServer";
    std::cout << "[Server] Body data size: " << body_data.size() << std::endl;
    ret = send(client_fd, body_data.data(), body_data.size(), 0);
    if (ret > 0) {
        std::cout << "[Server] Body sent successfully, ret=" << ret << std::endl;
    } else {
        std::cerr << "[Server] Failed to send body: " << strerror(errno) << std::endl;
    }

    // 等待一段时间
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    // 关闭连接
    close(client_fd);
    close(server_fd);
    std::cout << "[Server] Server closed" << std::endl;
}

// 客户端函数
void run_client() {
    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // 创建 socket
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        std::cerr << "[Client] Failed to create socket: " << strerror(errno) << std::endl;
        return;
    }

    // 设置地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(19006);

    // 连接
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[Client] Failed to connect: " << strerror(errno) << std::endl;
        close(client_fd);
        return;
    }

    std::cout << "[Client] Connected to server" << std::endl;

    // 接收消息头
    MessageHeader header;
    int ret = recv(client_fd, &header, sizeof(header), 0);
    if (ret != sizeof(header)) {
        std::cerr << "[Client] Failed to receive header, ret: " << ret << std::endl;
        close(client_fd);
        return;
    }

    // 解析消息头
    uint32_t msg_id = ntohl(header.msg_id);
    uint32_t msg_len = ntohl(header.msg_len);
    uint64_t target_id = ntohl(header.target_id);
    uint32_t user_data = ntohl(header.user_data);

    std::cout << "[Client] Header received: " << std::endl;
    std::cout << "  msg_id: " << msg_id << std::endl;
    std::cout << "  msg_len: " << msg_len << std::endl;
    std::cout << "  target_id: " << target_id << std::endl;
    std::cout << "  user_data: " << user_data << std::endl;

    // 接收消息体
    uint32_t body_len = msg_len - sizeof(header);
    if (body_len > 0) {
        char* body_data = new char[body_len];
        ret = recv(client_fd, body_data, body_len, 0);
        if (ret != body_len) {
            std::cerr << "[Client] Failed to receive body, ret: " << ret << std::endl;
            delete[] body_data;
            close(client_fd);
            return;
        }

        std::cout << "[Client] Body received, size: " << ret << std::endl;
        delete[] body_data;
    }

    // 关闭连接
    close(client_fd);
    std::cout << "[Client] Client closed" << std::endl;
}

int main() {
    // 启动服务器线程
    std::thread server_thread(run_server);
    
    // 启动客户端线程
    std::thread client_thread(run_client);

    // 等待线程结束
    server_thread.join();
    client_thread.join();

    return 0;
}
