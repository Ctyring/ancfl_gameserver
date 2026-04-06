#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include "center_server/center_server.h"

// 测试宏
#define TEST_BEGIN(name) std::cout << "=== 测试: " << name << " ===\n"
#define TEST_PASS(msg) std::cout << "[通过] " << msg << "\n"
#define TEST_FAIL(msg) std::cout << "[失败] " << msg << "\n"

// 测试结果统计
int g_pass_count = 0;
int g_fail_count = 0;

// 断言宏
#define ASSERT_TRUE(condition, msg) do { \
    if (condition) { \
        TEST_PASS(msg); \
        g_pass_count++; \
    } else { \
        TEST_FAIL(msg); \
        g_fail_count++; \
    } \
} while(0)

#define ASSERT_FALSE(condition, msg) ASSERT_TRUE(!(condition), msg)
#define ASSERT_EQ(expected, actual, msg) ASSERT_TRUE((expected) == (actual), msg)
#define ASSERT_NE(expected, actual, msg) ASSERT_TRUE((expected) != (actual), msg)

// 测试服务器初始化
void TestCenterServerInit() {
    TEST_BEGIN("中心服务器初始化");

    game_server::CenterServer center_server;
    
    // 测试初始化
    ASSERT_TRUE(center_server.Init("config.yaml"), "中心服务器初始化成功");
    
    // 测试启动
    ASSERT_TRUE(center_server.Start(), "中心服务器启动成功");
    ASSERT_TRUE(center_server.IsRunning(), "中心服务器运行状态正确");
    
    // 测试停止
    center_server.Stop();
    ASSERT_FALSE(center_server.IsRunning(), "中心服务器停止后状态正确");
}

// 测试服务器注册和注销
void TestServerRegister() {
    TEST_BEGIN("服务器注册和注销");

    game_server::CenterServer center_server;
    ASSERT_TRUE(center_server.Init("config.yaml"), "中心服务器初始化成功");
    ASSERT_TRUE(center_server.Start(), "中心服务器启动成功");

    // 创建测试服务器信息
    game_server::ServerInfo server1;
    server1.server_id = 1001;
    server1.type = game_server::ServerType::LOGIC;
    server1.ip = "192.168.1.1";
    server1.port = 8001;
    server1.status = game_server::ServerStatus::RUNNING;
    server1.online_count = 100;
    server1.max_online = 1000;
    server1.version = "1.0.0";

    game_server::ServerInfo server2;
    server2.server_id = 1002;
    server2.type = game_server::ServerType::LOGIC;
    server2.ip = "192.168.1.2";
    server2.port = 8002;
    server2.status = game_server::ServerStatus::RUNNING;
    server2.online_count = 200;
    server2.max_online = 1000;
    server2.version = "1.0.0";

    // 测试服务器注册
    ASSERT_TRUE(center_server.RegisterServer(server1), "服务器1注册成功");
    ASSERT_TRUE(center_server.RegisterServer(server2), "服务器2注册成功");

    // 测试重复注册
    ASSERT_TRUE(center_server.RegisterServer(server1), "重复注册服务器成功");

    // 测试获取服务器信息
    game_server::ServerInfo info;
    ASSERT_TRUE(center_server.GetServerInfo(1001, info), "获取服务器1信息成功");
    ASSERT_EQ(info.server_id, 1001, "服务器1 ID正确");
    ASSERT_EQ(info.type, game_server::ServerType::LOGIC, "服务器1类型正确");
    ASSERT_EQ(info.online_count, 100, "服务器1在线人数正确");

    // 测试获取不存在服务器信息
    ASSERT_FALSE(center_server.GetServerInfo(9999, info), "获取不存在服务器信息失败");

    // 测试服务器注销
    ASSERT_TRUE(center_server.UnregisterServer(1001), "服务器1注销成功");
    ASSERT_FALSE(center_server.GetServerInfo(1001, info), "注销后获取服务器1信息失败");

    // 测试注销不存在的服务器
    ASSERT_FALSE(center_server.UnregisterServer(9999), "注销不存在的服务器失败");

    center_server.Stop();
}

// 测试服务器心跳
void TestServerHeartbeat() {
    TEST_BEGIN("服务器心跳");

    game_server::CenterServer center_server;
    ASSERT_TRUE(center_server.Init("config.yaml"), "中心服务器初始化成功");
    ASSERT_TRUE(center_server.Start(), "中心服务器启动成功");

    // 注册服务器
    game_server::ServerInfo server;
    server.server_id = 2001;
    server.type = game_server::ServerType::LOGIC;
    server.ip = "192.168.1.1";
    server.port = 8001;
    server.status = game_server::ServerStatus::RUNNING;
    server.online_count = 100;
    server.max_online = 1000;
    server.version = "1.0.0";

    ASSERT_TRUE(center_server.RegisterServer(server), "服务器注册成功");

    // 测试更新心跳
    ASSERT_TRUE(center_server.UpdateServerHeartbeat(2001), "更新服务器心跳成功");

    // 测试更新不存在服务器心跳
    ASSERT_FALSE(center_server.UpdateServerHeartbeat(9999), "更新不存在服务器心跳失败");

    center_server.Stop();
}

// 测试获取服务器列表
void TestGetServerList() {
    TEST_BEGIN("获取服务器列表");

    game_server::CenterServer center_server;
    ASSERT_TRUE(center_server.Init("config.yaml"), "中心服务器初始化成功");
    ASSERT_TRUE(center_server.Start(), "中心服务器启动成功");

    // 注册多个服务器
    game_server::ServerInfo server1;
    server1.server_id = 3001;
    server1.type = game_server::ServerType::LOGIC;
    server1.ip = "192.168.1.1";
    server1.port = 8001;
    server1.status = game_server::ServerStatus::RUNNING;
    server1.online_count = 100;
    server1.max_online = 1000;
    server1.version = "1.0.0";

    game_server::ServerInfo server2;
    server2.server_id = 3002;
    server2.type = game_server::ServerType::LOGIC;
    server2.ip = "192.168.1.2";
    server2.port = 8002;
    server2.status = game_server::ServerStatus::RUNNING;
    server2.online_count = 200;
    server2.max_online = 1000;
    server2.version = "1.0.0";

    game_server::ServerInfo server3;
    server3.server_id = 3003;
    server3.type = game_server::ServerType::DB;
    server3.ip = "192.168.1.3";
    server3.port = 8003;
    server3.status = game_server::ServerStatus::RUNNING;
    server3.online_count = 50;
    server3.max_online = 1000;
    server3.version = "1.0.0";

    ASSERT_TRUE(center_server.RegisterServer(server1), "服务器1注册成功");
    ASSERT_TRUE(center_server.RegisterServer(server2), "服务器2注册成功");
    ASSERT_TRUE(center_server.RegisterServer(server3), "服务器3注册成功");

    // 测试获取指定类型的服务器列表
    std::vector<game_server::ServerInfo> logic_servers;
    ASSERT_TRUE(center_server.GetServerList(game_server::ServerType::LOGIC, logic_servers), "获取逻辑服务器列表成功");
    ASSERT_EQ(logic_servers.size(), 2, "逻辑服务器数量正确");

    std::vector<game_server::ServerInfo> db_servers;
    ASSERT_TRUE(center_server.GetServerList(game_server::ServerType::DB, db_servers), "获取数据库服务器列表成功");
    ASSERT_EQ(db_servers.size(), 1, "数据库服务器数量正确");

    // 测试获取所有服务器
    std::vector<game_server::ServerInfo> all_servers;
    ASSERT_TRUE(center_server.GetAllServers(all_servers), "获取所有服务器列表成功");
    ASSERT_EQ(all_servers.size(), 3, "所有服务器数量正确");

    center_server.Stop();
}

// 测试服务器负载均衡
void TestServerLoadBalance() {
    TEST_BEGIN("服务器负载均衡");

    game_server::CenterServer center_server;
    ASSERT_TRUE(center_server.Init("config.yaml"), "中心服务器初始化成功");
    ASSERT_TRUE(center_server.Start(), "中心服务器启动成功");

    // 注册多个服务器，设置不同的负载
    game_server::ServerInfo server1;
    server1.server_id = 4001;
    server1.type = game_server::ServerType::LOGIC;
    server1.ip = "192.168.1.1";
    server1.port = 8001;
    server1.status = game_server::ServerStatus::RUNNING;
    server1.online_count = 800;
    server1.max_online = 1000;
    server1.version = "1.0.0";

    game_server::ServerInfo server2;
    server2.server_id = 4002;
    server2.type = game_server::ServerType::LOGIC;
    server2.ip = "192.168.1.2";
    server2.port = 8002;
    server2.status = game_server::ServerStatus::RUNNING;
    server2.online_count = 300;
    server2.max_online = 1000;
    server2.version = "1.0.0";

    game_server::ServerInfo server3;
    server3.server_id = 4003;
    server3.type = game_server::ServerType::LOGIC;
    server3.ip = "192.168.1.3";
    server3.port = 8003;
    server3.status = game_server::ServerStatus::RUNNING;
    server3.online_count = 500;
    server3.max_online = 1000;
    server3.version = "1.0.0";

    ASSERT_TRUE(center_server.RegisterServer(server1), "服务器1注册成功");
    ASSERT_TRUE(center_server.RegisterServer(server2), "服务器2注册成功");
    ASSERT_TRUE(center_server.RegisterServer(server3), "服务器3注册成功");

    // 测试选择最低负载服务器
    int32_t best_server = center_server.SelectBestServer(game_server::ServerType::LOGIC);
    ASSERT_EQ(best_server, 4002, "选择最低负载服务器正确");

    // 测试更新服务器负载
    ASSERT_TRUE(center_server.UpdateServerLoad(4001, 900), "更新服务器1负载成功");
    ASSERT_TRUE(center_server.UpdateServerLoad(4002, 400), "更新服务器2负载成功");
    ASSERT_TRUE(center_server.UpdateServerLoad(4003, 600), "更新服务器3负载成功");

    // 验证负载更新
    int32_t load1 = center_server.GetServerLoad(4001);
    int32_t load2 = center_server.GetServerLoad(4002);
    int32_t load3 = center_server.GetServerLoad(4003);

    ASSERT_EQ(load1, 900, "服务器1负载正确");
    ASSERT_EQ(load2, 400, "服务器2负载正确");
    ASSERT_EQ(load3, 600, "服务器3负载正确");

    // 测试获取不存在服务器负载
    int32_t invalid_load = center_server.GetServerLoad(9999);
    ASSERT_EQ(invalid_load, -1, "获取不存在服务器负载返回-1");

    center_server.Stop();
}

// 测试跨服功能
void TestCrossServer() {
    TEST_BEGIN("跨服功能");

    game_server::CenterServer center_server;
    ASSERT_TRUE(center_server.Init("config.yaml"), "中心服务器初始化成功");
    ASSERT_TRUE(center_server.Start(), "中心服务器启动成功");

    // 测试玩家进入跨服
    uint64_t role_id = 1000001;
    ASSERT_TRUE(center_server.PlayerEnterCross(role_id, 1001, 1002, 
        static_cast<int32_t>(game_server::CrossType::CROSS_BATTLE), "token_123"), 
        "玩家进入跨服成功");

    // 测试检查玩家是否在跨服
    ASSERT_TRUE(center_server.IsPlayerInCross(role_id), "玩家在跨服中");

    // 测试获取跨服玩家信息
    game_server::CrossServerPlayer player;
    ASSERT_TRUE(center_server.GetCrossPlayer(role_id, player), "获取跨服玩家信息成功");
    ASSERT_EQ(player.role_id, role_id, "跨服玩家ID正确");
    ASSERT_EQ(player.src_server_id, 1001, "源服务器ID正确");
    ASSERT_EQ(player.dest_server_id, 1002, "目标服务器ID正确");
    ASSERT_EQ(player.cross_type, static_cast<int32_t>(game_server::CrossType::CROSS_BATTLE), "跨服类型正确");

    // 测试玩家离开跨服
    ASSERT_TRUE(center_server.PlayerLeaveCross(role_id), "玩家离开跨服成功");
    ASSERT_FALSE(center_server.IsPlayerInCross(role_id), "玩家不在跨服中");

    // 测试获取不存在的跨服玩家
    ASSERT_FALSE(center_server.GetCrossPlayer(role_id, player), "获取不存在的跨服玩家失败");

    center_server.Stop();
}

// 测试匹配队列
void TestMatchQueue() {
    TEST_BEGIN("匹配队列");

    game_server::CenterServer center_server;
    ASSERT_TRUE(center_server.Init("config.yaml"), "中心服务器初始化成功");
    ASSERT_TRUE(center_server.Start(), "中心服务器启动成功");

    // 注册逻辑服务器（用于匹配）
    game_server::ServerInfo logic_server;
    logic_server.server_id = 3001;
    logic_server.type = game_server::ServerType::LOGIC;
    logic_server.ip = "192.168.1.100";
    logic_server.port = 9001;
    logic_server.status = game_server::ServerStatus::RUNNING;
    logic_server.online_count = 100;
    logic_server.max_online = 1000;
    ASSERT_TRUE(center_server.RegisterServer(logic_server), "逻辑服务器注册成功");

    // 测试添加玩家到匹配队列
    ASSERT_TRUE(center_server.AddToMatchQueue(1000001, 1, 100), "玩家1加入匹配队列成功");
    ASSERT_TRUE(center_server.AddToMatchQueue(1000002, 1, 150), "玩家2加入匹配队列成功");
    ASSERT_TRUE(center_server.AddToMatchQueue(1000003, 1, 120), "玩家3加入匹配队列成功");

    // 测试重复加入匹配队列
    ASSERT_FALSE(center_server.AddToMatchQueue(1000001, 1, 100), "重复加入匹配队列失败");

    // 测试获取匹配队列
    std::vector<uint64_t> players;
    ASSERT_TRUE(center_server.GetMatchQueue(1, players), "获取匹配队列成功");
    ASSERT_EQ(players.size(), 3, "匹配队列玩家数量正确");

    // 测试处理匹配
    ASSERT_TRUE(center_server.ProcessMatch(1, 2), "处理匹配成功");

    // 验证匹配后队列剩余玩家
    ASSERT_TRUE(center_server.GetMatchQueue(1, players), "获取匹配队列成功");
    ASSERT_EQ(players.size(), 1, "匹配后剩余玩家数量正确");

    // 测试从匹配队列移除剩余玩家（分数最高的玩家1000002会被剩下）
    ASSERT_TRUE(center_server.RemoveFromMatchQueue(1000002), "从匹配队列移除玩家成功");

    // 验证移除后队列剩余玩家
    ASSERT_TRUE(center_server.GetMatchQueue(1, players), "获取匹配队列成功");
    ASSERT_EQ(players.size(), 0, "移除后匹配队列为空");

    center_server.Stop();
}

// 测试消息路由
void TestMessageRouting() {
    TEST_BEGIN("消息路由");

    game_server::CenterServer center_server;
    ASSERT_TRUE(center_server.Init("config.yaml"), "中心服务器初始化成功");
    ASSERT_TRUE(center_server.Start(), "中心服务器启动成功");

    // 注册服务器
    game_server::ServerInfo server1;
    server1.server_id = 5001;
    server1.type = game_server::ServerType::LOGIC;
    server1.ip = "192.168.1.1";
    server1.port = 8001;
    server1.status = game_server::ServerStatus::RUNNING;
    server1.online_count = 100;
    server1.max_online = 1000;
    server1.version = "1.0.0";

    game_server::ServerInfo server2;
    server2.server_id = 5002;
    server2.type = game_server::ServerType::LOGIC;
    server2.ip = "192.168.1.2";
    server2.port = 8002;
    server2.status = game_server::ServerStatus::RUNNING;
    server2.online_count = 200;
    server2.max_online = 1000;
    server2.version = "1.0.0";

    ASSERT_TRUE(center_server.RegisterServer(server1), "服务器1注册成功");
    ASSERT_TRUE(center_server.RegisterServer(server2), "服务器2注册成功");

    // 测试消息路由
    std::string msg = "test message";
    ASSERT_TRUE(center_server.RouteMessage(5001, 5002, msg), "消息路由成功");

    // 测试路由到不存在的服务器
    ASSERT_FALSE(center_server.RouteMessage(5001, 9999, msg), "路由到不存在服务器失败");

    // 测试广播消息
    ASSERT_TRUE(center_server.BroadcastMessage(game_server::ServerType::LOGIC, msg), "广播消息到逻辑服务器成功");

    // 测试广播到所有服务器
    ASSERT_TRUE(center_server.BroadcastToAll(msg), "广播消息到所有服务器成功");

    center_server.Stop();
}

int main() {
    std::cout << "========================================\n";
    std::cout << "中心服务器单元测试\n";
    std::cout << "========================================\n\n";

    // 运行测试
    TestCenterServerInit();
    TestServerRegister();
    TestServerHeartbeat();
    TestGetServerList();
    TestServerLoadBalance();
    TestCrossServer();
    TestMatchQueue();
    TestMessageRouting();

    // 输出测试结果
    std::cout << "\n========================================\n";
    std::cout << "测试结果\n";
    std::cout << "========================================\n";
    std::cout << "通过: " << g_pass_count << "\n";
    std::cout << "失败: " << g_fail_count << "\n";
    std::cout << "总计:  " << g_pass_count + g_fail_count << "\n\n";

    if (g_fail_count == 0) {
        std::cout << "所有测试通过！\n";
    } else {
        std::cout << "存在测试失败！\n";
    }

    return g_fail_count == 0 ? 0 : 1;
}
