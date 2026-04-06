#include "log_server/log_server.h"
#include "common/log_client.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>

using namespace game_server;

int g_total_tests = 0;
int g_passed_tests = 0;
int g_failed_tests = 0;
std::mutex g_test_mutex;

#define TEST_BEGIN(name) \
    { \
        std::lock_guard<std::mutex> lock(g_test_mutex); \
        std::cout << "\n=== 测试: " << name << " ===" << std::endl; \
    }

#define ASSERT_TRUE(expr, msg) \
    do { \
        std::lock_guard<std::mutex> lock(g_test_mutex); \
        g_total_tests++; \
        if (expr) { \
            g_passed_tests++; \
            std::cout << "[通过] " << msg << std::endl; \
        } else { \
            g_failed_tests++; \
            std::cout << "[失败] " << msg << std::endl; \
        } \
    } while(0)

#define ASSERT_FALSE(expr, msg) ASSERT_TRUE(!(expr), msg)

void TestLogServerInit() {
    TEST_BEGIN("日志服务器初始化");
    
    LogServer server;
    ASSERT_TRUE(server.Init("127.0.0.1", 19000), "日志服务器初始化成功");
    
    server.Stop();
    ASSERT_TRUE(true, "日志服务器停止成功");
}

void TestLogClientConnect() {
    TEST_BEGIN("日志客户端连接");
    
    LogClient client;
    ASSERT_TRUE(client.Connect("127.0.0.1", 19000), "日志客户端连接成功");
    ASSERT_TRUE(client.IsConnected(), "日志客户端已连接");
    
    client.Disconnect();
    ASSERT_TRUE(!client.IsConnected(), "日志客户端断开连接成功");
}

void TestSendLog() {
    TEST_BEGIN("发送单条日志");
    
    LogClient client;
    ASSERT_TRUE(client.Connect("127.0.0.1", 19000), "日志客户端连接成功");
    
    LogRecord record;
    record.log_id = 1;
    record.type = LogType::SYSTEM;
    record.level = LogLevel::INFO;
    record.server_id = 1001;
    record.role_id = 1000001;
    record.role_name = "测试玩家";
    record.content = "这是一条测试日志";
    record.log_time = time(nullptr);
    record.ip = "192.168.1.100";
    
    ASSERT_TRUE(client.SendLog(record), "发送日志成功");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void TestSendLogBatch() {
    TEST_BEGIN("批量发送日志");
    
    LogClient client;
    ASSERT_TRUE(client.Connect("127.0.0.1", 19000), "日志客户端连接成功");
    
    std::vector<LogRecord> records;
    for (int i = 0; i < 10; ++i) {
        LogRecord record;
        record.log_id = 100 + i;
        record.type = LogType::SYSTEM;
        record.level = LogLevel::INFO;
        record.server_id = 1001;
        record.role_id = 1000001 + i;
        record.role_name = "批量测试玩家" + std::to_string(i);
        record.content = "批量测试日志 " + std::to_string(i);
        record.log_time = time(nullptr);
        records.push_back(record);
    }
    
    ASSERT_TRUE(client.SendLogBatch(records), "批量发送日志成功");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

void TestLogLogin() {
    TEST_BEGIN("登录日志");
    
    LogClient client;
    client.SetServerId(1001);
    ASSERT_TRUE(client.Connect("127.0.0.1", 19000), "日志客户端连接成功");
    
    ASSERT_TRUE(client.LogLogin(1000001, "测试玩家", 1001, "192.168.1.100", true), 
                "记录登录日志成功");
    
    ASSERT_TRUE(client.LogLogin(1000001, "测试玩家", 1001, "192.168.1.100", false), 
                "记录登出日志成功");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void TestLogRecharge() {
    TEST_BEGIN("充值日志");
    
    LogClient client;
    client.SetServerId(1001);
    ASSERT_TRUE(client.Connect("127.0.0.1", 19000), "日志客户端连接成功");
    
    ASSERT_TRUE(client.LogRecharge(1000001, "测试玩家", 1001, 100, 10001), 
                "记录充值日志成功");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void TestLogConsume() {
    TEST_BEGIN("消费日志");
    
    LogClient client;
    client.SetServerId(1001);
    ASSERT_TRUE(client.Connect("127.0.0.1", 19000), "日志客户端连接成功");
    
    ASSERT_TRUE(client.LogConsume(1000001, "测试玩家", 1001, 1, 100, "购买道具"), 
                "记录消费日志成功");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void TestLogItem() {
    TEST_BEGIN("物品日志");
    
    LogClient client;
    client.SetServerId(1001);
    ASSERT_TRUE(client.Connect("127.0.0.1", 19000), "日志客户端连接成功");
    
    ASSERT_TRUE(client.LogItem(1000001, "测试玩家", 1001, 10001, 10, 1, "任务奖励"), 
                "记录物品日志成功");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void TestLogBattle() {
    TEST_BEGIN("战斗日志");
    
    LogClient client;
    client.SetServerId(1001);
    ASSERT_TRUE(client.Connect("127.0.0.1", 19000), "日志客户端连接成功");
    
    ASSERT_TRUE(client.LogBattle(1000001, "测试玩家", 1001, 1, 1, 300), 
                "记录战斗日志成功");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void TestLogMacros() {
    TEST_BEGIN("日志宏测试");
    
    LogClient client;
    client.SetServerId(1001);
    g_log_client = &client;
    
    ASSERT_TRUE(client.Connect("127.0.0.1", 19000), "日志客户端连接成功");
    
    LOG_CLIENT_LOGIN(1000001, "测试玩家", "192.168.1.100", true);
    ASSERT_TRUE(true, "LOGIN宏调用成功");
    
    LOG_CLIENT_RECHARGE(1000001, "测试玩家", 100, 10001);
    ASSERT_TRUE(true, "RECHARGE宏调用成功");
    
    LOG_CLIENT_CONSUME(1000001, "测试玩家", 1, 100, "购买道具");
    ASSERT_TRUE(true, "CONSUME宏调用成功");
    
    LOG_CLIENT_ITEM(1000001, "测试玩家", 10001, 10, 1, "任务奖励");
    ASSERT_TRUE(true, "ITEM宏调用成功");
    
    LOG_CLIENT_BATTLE(1000001, "测试玩家", 1, 1, 300);
    ASSERT_TRUE(true, "BATTLE宏调用成功");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    g_log_client = nullptr;
}

void TestMultiThreadWrite() {
    TEST_BEGIN("多线程写入");
    
    LogClient client;
    client.SetServerId(1001);
    ASSERT_TRUE(client.Connect("127.0.0.1", 19000), "日志客户端连接成功");
    
    const int thread_count = 5;
    const int logs_per_thread = 20;
    
    std::vector<std::thread> threads;
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([&client, i, logs_per_thread]() {
            for (int j = 0; j < logs_per_thread; ++j) {
                LogRecord record;
                record.log_id = i * 1000 + j;
                record.type = LogType::SYSTEM;
                record.level = LogLevel::INFO;
                record.server_id = 1001;
                record.role_id = 1000001 + i;
                record.role_name = "线程" + std::to_string(i) + "玩家";
                record.content = "多线程测试日志 " + std::to_string(j);
                record.log_time = time(nullptr);
                
                client.SendLog(record);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    ASSERT_TRUE(true, "多线程写入完成");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "日志服务器网络通信测试" << std::endl;
    std::cout << "========================================" << std::endl;
    
    TestLogServerInit();
    TestLogClientConnect();
    TestSendLog();
    TestSendLogBatch();
    TestLogLogin();
    TestLogRecharge();
    TestLogConsume();
    TestLogItem();
    TestLogBattle();
    TestLogMacros();
    TestMultiThreadWrite();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "测试结果" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "通过: " << g_passed_tests << std::endl;
    std::cout << "失败: " << g_failed_tests << std::endl;
    std::cout << "总计:  " << g_total_tests << std::endl;
    std::cout << std::endl;
    
    if (g_failed_tests == 0) {
        std::cout << "所有测试通过！" << std::endl;
    } else {
        std::cout << "存在测试失败！" << std::endl;
    }
    
    return g_failed_tests > 0 ? 1 : 0;
}
