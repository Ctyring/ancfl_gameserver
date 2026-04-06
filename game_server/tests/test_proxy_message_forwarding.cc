#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "proto/msg_account.pb.h"
#include "proto/msg_id.pb.h"

// 消息头结构体
typedef struct MessageHeader {
    uint32_t msg_id;
    uint32_t msg_len;
    uint64_t target_id;
    uint32_t user_data;
} __attribute__((packed)) MessageHeader;

int main() {
    // 创建套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    // 设置服务器地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8005); // 修改为代理服务器的端口
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 连接到服务器
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to connect to server" << std::endl;
        close(sockfd);
        return 1;
    }

    std::cout << "Connected to proxy server" << std::endl;

    // 创建登录请求
    AccountLoginReq req;
    req.set_account_name("test");
    req.set_password("test123");

    // 序列化消息
    std::string serialized_data;
    if (!req.SerializeToString(&serialized_data)) {
        std::cerr << "Failed to serialize message" << std::endl;
        close(sockfd);
        return 1;
    }

    // 构建消息头
    MessageHeader header;
    header.msg_id = htonl(static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_REQ));
    header.msg_len = htonl(serialized_data.size());
    header.target_id = htonl(0); // 设置为 0
    header.user_data = htonl(0); // 设置为 0

    // 发送消息头
    if (send(sockfd, &header, sizeof(MessageHeader), 0) < 0) {
        std::cerr << "Failed to send message header" << std::endl;
        close(sockfd);
        return 1;
    }

    // 发送消息体
    if (send(sockfd, serialized_data.c_str(), serialized_data.size(), 0) < 0) {
        std::cerr << "Failed to send message body" << std::endl;
        close(sockfd);
        return 1;
    }

    std::cout << "Sent login request" << std::endl;

    // 接收响应
    char buffer[1024];
    ssize_t recv_len = recv(sockfd, buffer, sizeof(buffer), 0);
    if (recv_len < 0) {
        std::cerr << "Failed to receive response" << std::endl;
        close(sockfd);
        return 1;
    }

    if (recv_len >= sizeof(MessageHeader)) {
        // 解析响应头
        MessageHeader resp_header;
        memcpy(&resp_header, buffer, sizeof(MessageHeader));
        uint32_t resp_msg_id = ntohl(resp_header.msg_id);
        uint32_t resp_msg_len = ntohl(resp_header.msg_len);

        std::cout << "Received response: msg_id=" << resp_msg_id << ", msg_len=" << resp_msg_len << std::endl;

        // 解析响应体
        if (recv_len >= sizeof(MessageHeader) + resp_msg_len) {
            std::string resp_data(buffer + sizeof(MessageHeader), resp_msg_len);
            if (resp_msg_id == static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_ACK)) {
                AccountLoginAck ack;
                if (ack.ParseFromString(resp_data)) {
                    std::cout << "Login response: ret_code=" << ack.ret_code() << ", account_id=" << ack.account_id() << std::endl;
                } else {
                    std::cerr << "Failed to parse login response" << std::endl;
                }
            } else {
                std::cout << "Received unexpected message type: " << resp_msg_id << std::endl;
            }
        }
    }

    // 关闭连接
    close(sockfd);

    return 0;
}
