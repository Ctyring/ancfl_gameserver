#include "message_dispatcher.h"
#include "ancfl/log.h"
#include <iostream>
#include <cassert>
#include <chrono>
#include <vector>
#include <thread>

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

void test_register_handler() {
    std::cout << "\n=== 测试: 注册处理器 ===" << std::endl;
    
    MessageDispatcher dispatcher;
    int call_count = 0;
    
    auto handler = [&call_count](const NetPacket& packet) -> bool {
        call_count++;
        return true;
    };
    
    dispatcher.RegisterHandler(1001, handler);
    
    TEST_ASSERT(dispatcher.HasHandler(1001), "注册后 HasHandler 返回 true");
    TEST_ASSERT(dispatcher.GetHandlerCount() == 1, "注册后处理器数量为 1");
}

void test_register_multiple_handlers() {
    std::cout << "\n=== 测试: 注册多个处理器 ===" << std::endl;
    
    MessageDispatcher dispatcher;
    
    auto handler1 = [](const NetPacket& packet) -> bool { return true; };
    auto handler2 = [](const NetPacket& packet) -> bool { return true; };
    
    dispatcher.RegisterHandler(1001, handler1);
    dispatcher.RegisterHandler(1002, handler2);
    
    TEST_ASSERT(dispatcher.HasHandler(1001), "存在消息 ID 1001 的处理器");
    TEST_ASSERT(dispatcher.HasHandler(1002), "存在消息 ID 1002 的处理器");
    TEST_ASSERT(dispatcher.GetHandlerCount() == 2, "处理器数量为 2");
}

void test_dispatch_message() {
    std::cout << "\n=== 测试: 分发消息 ===" << std::endl;
    
    MessageDispatcher dispatcher;
    int call_count = 0;
    uint32_t received_msg_id = 0;
    
    auto handler = [&call_count, &received_msg_id](const NetPacket& packet) -> bool {
        call_count++;
        received_msg_id = packet.msg_id;
        return true;
    };
    
    dispatcher.RegisterHandler(1001, handler);
    
    NetPacket packet;
    packet.conn_id = 1;
    packet.msg_id = 1001;
    packet.target_id = 0;
    packet.user_data = 0;
    
    bool result = dispatcher.Dispatch(packet);
    
    TEST_ASSERT(result == true, "已注册消息的分发返回 true");
    TEST_ASSERT(call_count == 1, "处理器被调用一次");
    TEST_ASSERT(received_msg_id == 1001, "接收到正确的消息 ID");
}

void test_dispatch_unknown_message() {
    std::cout << "\n=== 测试: 分发未知消息 ===" << std::endl;
    
    MessageDispatcher dispatcher;
    int call_count = 0;
    
    auto handler = [&call_count](const NetPacket& packet) -> bool {
        call_count++;
        return true;
    };
    
    dispatcher.RegisterHandler(1001, handler);
    
    NetPacket packet;
    packet.conn_id = 1;
    packet.msg_id = 9999;
    packet.target_id = 0;
    packet.user_data = 0;
    
    bool result = dispatcher.Dispatch(packet);
    
    TEST_ASSERT(result == false, "未知消息的分发返回 false");
    TEST_ASSERT(call_count == 0, "未知消息不调用处理器");
}

void test_handler_returns_false() {
    std::cout << "\n=== 测试: 处理器返回 false ===" << std::endl;
    
    MessageDispatcher dispatcher;
    
    auto handler = [](const NetPacket& packet) -> bool {
        return false;
    };
    
    dispatcher.RegisterHandler(1001, handler);
    
    NetPacket packet;
    packet.conn_id = 1;
    packet.msg_id = 1001;
    packet.target_id = 0;
    packet.user_data = 0;
    
    bool result = dispatcher.Dispatch(packet);
    
    TEST_ASSERT(result == false, "当处理器返回 false 时分发返回 false");
}

void test_has_handler() {
    std::cout << "\n=== 测试: HasHandler 检查 ===" << std::endl;
    
    MessageDispatcher dispatcher;
    auto handler = [](const NetPacket& packet) -> bool { return true; };
    
    TEST_ASSERT(!dispatcher.HasHandler(1001), "注册前 HasHandler 返回 false");
    
    dispatcher.RegisterHandler(1001, handler);
    TEST_ASSERT(dispatcher.HasHandler(1001), "注册后 HasHandler 返回 true");
    
    TEST_ASSERT(!dispatcher.HasHandler(1002), "未注册消息 ID 对应的 HasHandler 返回 false");
}

void test_get_handler_count() {
    std::cout << "\n=== 测试: 获取处理器数量 ===" << std::endl;
    
    MessageDispatcher dispatcher;
    auto handler1 = [](const NetPacket& packet) -> bool { return true; };
    auto handler2 = [](const NetPacket& packet) -> bool { return true; };
    
    TEST_ASSERT(dispatcher.GetHandlerCount() == 0, "初始处理器数量为 0");
    
    dispatcher.RegisterHandler(1001, handler1);
    TEST_ASSERT(dispatcher.GetHandlerCount() == 1, "第一次注册后处理器数量为 1");
    
    dispatcher.RegisterHandler(1002, handler2);
    TEST_ASSERT(dispatcher.GetHandlerCount() == 2, "第二次注册后处理器数量为 2");
}

void test_override_handler() {
    std::cout << "\n=== 测试: 覆盖处理器 ===" << std::endl;
    
    MessageDispatcher dispatcher;
    int call_count1 = 0;
    int call_count2 = 0;
    
    auto handler1 = [&call_count1](const NetPacket& packet) -> bool {
        call_count1++;
        return true;
    };
    
    auto handler2 = [&call_count2](const NetPacket& packet) -> bool {
        call_count2++;
        return true;
    };
    
    dispatcher.RegisterHandler(1001, handler1);
    dispatcher.RegisterHandler(1001, handler2);
    
    NetPacket packet;
    packet.conn_id = 1;
    packet.msg_id = 1001;
    packet.target_id = 0;
    packet.user_data = 0;
    
    dispatcher.Dispatch(packet);
    
    TEST_ASSERT(call_count1 == 0, "第一个处理器未被调用（已被覆盖）");
    TEST_ASSERT(call_count2 == 1, "第二个处理器被调用");
    TEST_ASSERT(dispatcher.GetHandlerCount() == 1, "覆盖后处理器数量仍为 1");
}

void test_performance() {
    std::cout << "\n=== 测试: 性能测试 ===" << std::endl;
    
    MessageDispatcher dispatcher;
    
    auto handler = [](const NetPacket& packet) -> bool {
        return true;
    };
    
    const int handler_count = 1000;
    for (int i = 1; i <= handler_count; i++) {
        dispatcher.RegisterHandler(i, handler);
    }
    
    NetPacket packet;
    packet.conn_id = 1;
    packet.msg_id = 500;
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

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "消息分发器单元测试" << std::endl;
    std::cout << "========================================" << std::endl;
    
    test_register_handler();
    test_register_multiple_handlers();
    test_dispatch_message();
    test_dispatch_unknown_message();
    test_handler_returns_false();
    test_has_handler();
    test_get_handler_count();
    test_override_handler();
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
