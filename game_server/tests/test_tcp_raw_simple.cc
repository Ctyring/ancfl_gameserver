#include <iostream>
#include <thread>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

// 消息头结构体
typedef struct MessageHeader {
    uint32_t msg_id;
    uint32_t msg_len;
    uint64_t target_id;
    uint32_t user_data;
} MessageHeader;

// 服务器线程
void run_server() {
    // 创建服务器 socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        cerr << "[Server] Failed to create socket: " << strerror(errno) << endl;
        return;
    }
    
    // 设置 socket 选项
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        cerr << "[Server] Failed to set socket options: " << strerror(errno) << endl;
        close(server_fd);
        return;
    }
    
    // 绑定地址
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(19009);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        cerr << "[Server] Failed to bind: " << strerror(errno) << endl;
        close(server_fd);
        return;
    }
    
    // 监听
    if (listen(server_fd, 3) < 0) {
        cerr << "[Server] Failed to listen: " << strerror(errno) << endl;
        close(server_fd);
        return;
    }
    
    cout << "[Server] Server started on port 19009" << endl;
    cout << "[Server] Waiting for client connection..." << endl;
    
    // 接受连接
    int client_socket;
    struct sockaddr_in client_address;
    int addrlen = sizeof(client_address);
    
    client_socket = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t*)&addrlen);
    if (client_socket < 0) {
        cerr << "[Server] Failed to accept connection: " << strerror(errno) << endl;
        close(server_fd);
        return;
    }
    
    cout << "[Server] Client connected" << endl;
    cout << "[Server] Client socket fd: " << client_socket << endl;
    
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
    header.msg_id = htonl(header.msg_id);
    header.msg_len = htonl(header.msg_len);
    header.target_id = htobe64(header.target_id);
    header.user_data = htonl(header.user_data);
    
    // 发送消息头
    int sent_header = send(client_socket, &header, sizeof(MessageHeader), 0);
    if (sent_header < 0) {
        cerr << "[Server] Failed to send header: " << strerror(errno) << endl;
    } else {
        cout << "[Server] Sent header, size: " << sent_header << endl;
    }
    
    // 发送消息体
    int sent_data = send(client_socket, message.c_str(), message_len, 0);
    if (sent_data < 0) {
        cerr << "[Server] Failed to send data: " << strerror(errno) << endl;
    } else {
        cout << "[Server] Sent data, size: " << sent_data << endl;
    }
    
    // 等待一段时间后关闭连接
    this_thread::sleep_for(chrono::seconds(5));
    close(client_socket);
    close(server_fd);
    cout << "[Server] Server stopped" << endl;
}

// 客户端函数
void run_client() {
    // 等待服务器启动
    this_thread::sleep_for(chrono::milliseconds(500));
    
    // 创建客户端 socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        cerr << "[Client] Failed to create socket: " << strerror(errno) << endl;
        return;
    }
    
    // 连接到服务器
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(19009);
    
    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) <= 0) {
        cerr << "[Client] Invalid address" << endl;
        close(client_socket);
        return;
    }
    
    if (connect(client_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        cerr << "[Client] Failed to connect: " << strerror(errno) << endl;
        close(client_socket);
        return;
    }
    
    cout << "[Client] Connected to server" << endl;
    cout << "[Client] Socket fd: " << client_socket << endl;
    
    // 等待服务器发送消息
    cout << "[Client] Waiting for server to send message..." << endl;
    this_thread::sleep_for(chrono::milliseconds(1000));
    
    // 接收消息头
    MessageHeader header;
    int recv_header = recv(client_socket, &header, sizeof(MessageHeader), 0);
    if (recv_header < 0) {
        cerr << "[Client] Failed to receive header: " << strerror(errno) << endl;
    } else {
        cout << "[Client] Received header, size: " << recv_header << endl;
        
        // 字节序转换
        header.msg_id = ntohl(header.msg_id);
        header.msg_len = ntohl(header.msg_len);
        header.target_id = be64toh(header.target_id);
        header.user_data = ntohl(header.user_data);
        
        cout << "[Client] Header: msg_id=" << header.msg_id << ", msg_len=" << header.msg_len << ", target_id=" << header.target_id << ", user_data=" << header.user_data << endl;
        
        // 接收消息体
        int data_len = header.msg_len - sizeof(MessageHeader);
        char* data = new char[data_len + 1];
        int recv_data = recv(client_socket, data, data_len, 0);
        if (recv_data < 0) {
            cerr << "[Client] Failed to receive data: " << strerror(errno) << endl;
        } else {
            data[recv_data] = '\0';
            cout << "[Client] Received data, size: " << recv_data << endl;
            cout << "[Client] Message: " << data << endl;
        }
        delete[] data;
    }
    
    // 等待一段时间后关闭连接
    this_thread::sleep_for(chrono::seconds(2));
    close(client_socket);
    cout << "[Client] Client stopped" << endl;
}

int main() {
    // 启动服务器线程
    thread server_thread(run_server);
    
    // 启动客户端
    run_client();
    
    // 等待服务器线程退出
    server_thread.join();
    
    return 0;
}
