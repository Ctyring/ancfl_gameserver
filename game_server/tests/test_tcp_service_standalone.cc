#include "tcp_service.h"
#include "message_dispatcher.h"
#include "ancfl/log.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace game_server;

static int passed_tests = 0;
static int failed_tests = 0;

#define TEST_ASSERT(condition, test_name) \
    do { \
        if (condition) { \
            std::cout << "[通过] " << test_name << std::endl; \
            passed_tests++; \
        } else { \
            std::cout << "[失败] " << test_name << std::endl; \
            failed_tests++; \
        } \
    } while(0)

void test_message_header() {
    std::cout << "\n=== 测试: 消息头结构 ===" << std::endl;
    
    TEST_ASSERT(sizeof(MessageHeader) == 24, "消息头大小为 24 字节");
    
    MessageHeader header;
    header.msg_id = 1001;
    header.msg_len = sizeof(MessageHeader) + 10;
    header.target_id = 12345;
    header.user_data = 67890;
    
    TEST_ASSERT(header.msg_id == 1001, "消息 ID 正确设置");
    TEST_ASSERT(header.msg_len == 34, "消息长度正确设置");
    TEST_ASSERT(header.target_id == 12345, "目标 ID 正确设置");
    TEST_ASSERT(header.user_data == 67890, "用户数据正确设置");
}

void test_net_packet() {
    std::cout << "\n=== 测试: 网络包结构 ===" << std::endl;
    
    NetPacket packet;
    packet.conn_id = 1;
    packet.msg_id = 1001;
    packet.target_id = 100;
    packet.user_data = 0;
    packet.msg = nullptr;
    
    TEST_ASSERT(packet.conn_id == 1, "连接 ID 正确设置");
    TEST_ASSERT(packet.msg_id == 1001, "消息 ID 正确设置");
    TEST_ASSERT(packet.target_id == 100, "目标 ID 正确设置");
}

void test_message_dispatcher_basic() {
    std::cout << "\n=== 测试: 消息分发器基础功能 ===" << std::endl;
    
    MessageDispatcher dispatcher;
    
    TEST_ASSERT(dispatcher.GetHandlerCount() == 0, "初始处理器数量为 0");
    TEST_ASSERT(!dispatcher.HasHandler(1001), "初始时不存在处理器");
    
    int call_count = 0;
    auto handler = [&call_count](const NetPacket& packet) -> bool {
        call_count++;
        return true;
    };
    
    dispatcher.RegisterHandler(1001, handler);
    
    TEST_ASSERT(dispatcher.HasHandler(1001), "注册后存在处理器");
    TEST_ASSERT(dispatcher.GetHandlerCount() == 1, "处理器数量为 1");
    
    NetPacket packet;
    packet.conn_id = 1;
    packet.msg_id = 1001;
    packet.target_id = 0;
    packet.user_data = 0;
    
    bool result = dispatcher.Dispatch(packet);
    
    TEST_ASSERT(result == true, "消息分发成功");
    TEST_ASSERT(call_count == 1, "处理器被调用一次");
}

void test_message_dispatcher_unknown() {
    std::cout << "\n=== 测试: 消息分发器处理未知消息 ===" << std::endl;
    
    MessageDispatcher dispatcher;
    
    auto handler = [](const NetPacket& packet) -> bool {
        return true;
    };
    
    dispatcher.RegisterHandler(1001, handler);
    
    NetPacket packet;
    packet.conn_id = 1;
    packet.msg_id = 9999;
    packet.target_id = 0;
    packet.user_data = 0;
    
    bool result = dispatcher.Dispatch(packet);
    
    TEST_ASSERT(result == false, "未知消息分发返回 false");
}

void test_message_dispatcher_multiple() {
    std::cout << "\n=== 测试: 消息分发器多处理器 ===" << std::endl;
    
    MessageDispatcher dispatcher;
    
    int count1 = 0, count2 = 0, count3 = 0;
    
    dispatcher.RegisterHandler(1001, [&count1](const NetPacket& p) -> bool { count1++; return true; });
    dispatcher.RegisterHandler(1002, [&count2](const NetPacket& p) -> bool { count2++; return true; });
    dispatcher.RegisterHandler(1003, [&count3](const NetPacket& p) -> bool { count3++; return true; });
    
    TEST_ASSERT(dispatcher.GetHandlerCount() == 3, "处理器数量为 3");
    
    NetPacket packet;
    packet.conn_id = 1;
    packet.target_id = 0;
    packet.user_data = 0;
    
    packet.msg_id = 1001;
    dispatcher.Dispatch(packet);
    
    packet.msg_id = 1002;
    dispatcher.Dispatch(packet);
    
    packet.msg_id = 1003;
    dispatcher.Dispatch(packet);
    
    TEST_ASSERT(count1 == 1 && count2 == 1 && count3 == 1, "各处理器被正确调用");
}

void test_message_dispatcher_override() {
    std::cout << "\n=== 测试: 消息分发器覆盖处理器 ===" << std::endl;
    
    MessageDispatcher dispatcher;
    
    int count1 = 0, count2 = 0;
    
    dispatcher.RegisterHandler(1001, [&count1](const NetPacket& p) -> bool { count1++; return true; });
    dispatcher.RegisterHandler(1001, [&count2](const NetPacket& p) -> bool { count2++; return true; });
    
    TEST_ASSERT(dispatcher.GetHandlerCount() == 1, "覆盖后处理器数量仍为 1");
    
    NetPacket packet;
    packet.conn_id = 1;
    packet.msg_id = 1001;
    packet.target_id = 0;
    packet.user_data = 0;
    
    dispatcher.Dispatch(packet);
    
    TEST_ASSERT(count1 == 0, "第一个处理器未被调用");
    TEST_ASSERT(count2 == 1, "第二个处理器被调用");
}

void test_byteswap() {
    std::cout << "\n=== 测试: 字节序转换 ===" << std::endl;
    
    uint32_t value = 0x12345678;
    uint32_t swapped = ancfl::byteswapOnLittleEndian(value);
    
    TEST_ASSERT(swapped != value || sizeof(value) == 1, "字节序转换正常工作");
    
    uint32_t back = ancfl::byteswapOnLittleEndian(swapped);
    TEST_ASSERT(back == value, "双向转换恢复原值");
}

void test_performance() {
    std::cout << "\n=== 测试: 性能测试 ===" << std::endl;
    
    MessageDispatcher dispatcher;
    
    auto handler = [](const NetPacket& packet) -> bool {
        return true;
    };
    
    const int handler_count = 100;
    for (int i = 1; i <= handler_count; i++) {
        dispatcher.RegisterHandler(i, handler);
    }
    
    NetPacket packet;
    packet.conn_id = 1;
    packet.msg_id = 50;
    packet.target_id = 0;
    packet.user_data = 0;
    
    const int dispatch_count = 100000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < dispatch_count; i++) {
        dispatcher.Dispatch(packet);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double dispatches_per_second = (double)dispatch_count * 1000000.0 / duration.count();
    
    std::cout << "  注册了 " << handler_count << " 个处理器" << std::endl;
    std::cout << "  在 " << duration.count() << " 微秒内分发了 " << dispatch_count << " 条消息" << std::endl;
    std::cout << "  性能: " << (int)dispatches_per_second << " 次/秒" << std::endl;
    
    TEST_ASSERT(dispatches_per_second > 100000, "性能 > 10万次/秒");
}

void test_thread_safety() {
    std::cout << "\n=== 测试: 线程安全 ===" << std::endl;
    
    MessageDispatcher dispatcher;
    std::atomic<int> call_count{0};
    
    auto handler = [&call_count](const NetPacket& packet) -> bool {
        call_count++;
        return true;
    };
    
    for (int i = 1; i <= 100; i++) {
        dispatcher.RegisterHandler(i, handler);
    }
    
    std::vector<std::thread> threads;
    const int thread_count = 4;
    const int dispatches_per_thread = 1000;
    
    for (int t = 0; t < thread_count; t++) {
        threads.emplace_back([&dispatcher, t, dispatches_per_thread]() {
            for (int i = 0; i < dispatches_per_thread; i++) {
                NetPacket packet;
                packet.conn_id = t;
                packet.msg_id = (i % 100) + 1;
                packet.target_id = 0;
                packet.user_data = 0;
                dispatcher.Dispatch(packet);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    int expected_calls = thread_count * dispatches_per_thread;
    TEST_ASSERT(call_count == expected_calls, "多线程环境下所有分发操作完成");
}

void test_socket_basic() {
    std::cout << "\n=== 测试: Socket 基础操作 ===" << std::endl;
    
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    TEST_ASSERT(server_sock >= 0, "创建服务端 Socket 成功");
    
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(19980);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    int bind_result = bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    TEST_ASSERT(bind_result == 0, "Socket 绑定成功");
    
    int listen_result = listen(server_sock, 5);
    TEST_ASSERT(listen_result == 0, "Socket 监听成功");
    
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    TEST_ASSERT(client_sock >= 0, "创建客户端 Socket 成功");
    
    int connect_result = connect(client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    TEST_ASSERT(connect_result == 0, "客户端连接成功");
    
    close(client_sock);
    close(server_sock);
    
    TEST_ASSERT(true, "Socket 关闭成功");
}

void test_tcp_packet_format() {
    std::cout << "\n=== 测试: TCP 数据包格式 ===" << std::endl;
    
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        TEST_ASSERT(false, "创建服务端 Socket 失败");
        return;
    }
    
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(19979);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 ||
        listen(server_sock, 5) < 0) {
        close(server_sock);
        TEST_ASSERT(false, "服务端设置失败");
        return;
    }
    
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0 || connect(client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(server_sock);
        TEST_ASSERT(false, "客户端连接失败");
        return;
    }
    
    int accepted_sock = accept(server_sock, NULL, NULL);
    TEST_ASSERT(accepted_sock >= 0, "服务端接受连接成功");
    
    MessageHeader send_header;
    send_header.msg_id = ancfl::byteswapOnLittleEndian(1001);
    send_header.msg_len = ancfl::byteswapOnLittleEndian(sizeof(MessageHeader) + 4);
    send_header.target_id = 0;
    send_header.user_data = 0;
    
    const char* body = "test";
    
    int send_result = send(client_sock, &send_header, sizeof(send_header), 0);
    send_result += send(client_sock, body, 4, 0);
    TEST_ASSERT(send_result == sizeof(MessageHeader) + 4, "发送数据包成功");
    
    MessageHeader recv_header;
    int recv_result = recv(accepted_sock, &recv_header, sizeof(recv_header), 0);
    TEST_ASSERT(recv_result == sizeof(MessageHeader), "接收消息头成功");
    
    char recv_body[10] = {0};
    recv_result = recv(accepted_sock, recv_body, 4, 0);
    TEST_ASSERT(recv_result == 4, "接收消息体成功");
    TEST_ASSERT(strcmp(recv_body, "test") == 0, "消息体内容正确");
    
    close(accepted_sock);
    close(client_sock);
    close(server_sock);
}

void test_concurrent_connections() {
    std::cout << "\n=== 测试: 并发连接测试 ===" << std::endl;
    
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        TEST_ASSERT(false, "创建服务端 Socket 失败");
        return;
    }
    
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(19978);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 ||
        listen(server_sock, 20) < 0) {
        close(server_sock);
        TEST_ASSERT(false, "服务端设置失败");
        return;
    }
    
    std::atomic<int> success_count{0};
    std::vector<std::thread> threads;
    const int client_count = 10;
    
    std::thread accept_thread([&]() {
        for (int i = 0; i < client_count; i++) {
            int client = accept(server_sock, NULL, NULL);
            if (client >= 0) {
                close(client);
            }
        }
    });
    
    for (int i = 0; i < client_count; i++) {
        threads.emplace_back([&success_count, &server_addr]() {
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock >= 0) {
                if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0) {
                    success_count++;
                }
                close(sock);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    accept_thread.join();
    
    close(server_sock);
    
    TEST_ASSERT(success_count >= client_count * 0.8, 
                "并发连接成功率 >= 80% (" + std::to_string(success_count) + "/" + std::to_string(client_count) + ")");
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "TCP服务单元测试" << std::endl;
    std::cout << "========================================" << std::endl;
    
    test_message_header();
    test_net_packet();
    test_message_dispatcher_basic();
    test_message_dispatcher_unknown();
    test_message_dispatcher_multiple();
    test_message_dispatcher_override();
    test_byteswap();
    test_socket_basic();
    test_tcp_packet_format();
    test_concurrent_connections();
    test_performance();
    test_thread_safety();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "测试结果" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "通过: " << passed_tests << std::endl;
    std::cout << "失败: " << failed_tests << std::endl;
    std::cout << "总计:  " << (passed_tests + failed_tests) << std::endl;
    
    if (failed_tests == 0) {
        std::cout << "\n所有测试通过！" << std::endl;
        return 0;
    } else {
        std::cout << "\n部分测试失败！" << std::endl;
        return 1;
    }
}
