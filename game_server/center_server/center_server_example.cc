// 中心服务器使用示例
// 展示如何使用完善后的消息发送和匹配通知功能

#include "center_server/center_server.h"
#include "proto/msg_cross.pb.h"
#include "proto/msg_id.pb.h"
#include <iostream>

// 模拟网络发送函数
bool MockSendMessage(int32_t server_id, uint32_t msg_id, const google::protobuf::Message& msg) {
    std::cout << "[模拟发送] 服务器ID=" << server_id 
              << ", 消息ID=" << msg_id 
              << ", 消息类型=" << msg.GetTypeName() << std::endl;
    return true;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "      中心服务器使用示例               " << std::endl;
    std::cout << "========================================" << std::endl;

    // 创建中心服务器实例
    game_server::CenterServer center_server;

    // 设置消息发送回调
    center_server.SetMessageSendCallback(MockSendMessage);

    // 初始化并启动
    if (!center_server.Init("config.yaml")) {
        std::cerr << "中心服务器初始化失败" << std::endl;
        return -1;
    }

    if (!center_server.Start()) {
        std::cerr << "中心服务器启动失败" << std::endl;
        return -1;
    }

    std::cout << "\n1. 注册服务器" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // 注册逻辑服务器1
    game_server::ServerInfo logic_server1;
    logic_server1.server_id = 2001;
    logic_server1.type = game_server::ServerType::LOGIC;
    logic_server1.ip = "192.168.1.101";
    logic_server1.port = 9001;
    logic_server1.status = game_server::ServerStatus::RUNNING;
    logic_server1.online_count = 500;
    logic_server1.max_online = 2000;
    center_server.RegisterServer(logic_server1);
    std::cout << "注册逻辑服务器1: ID=2001, 在线人数=500" << std::endl;

    // 注册逻辑服务器2
    game_server::ServerInfo logic_server2;
    logic_server2.server_id = 2002;
    logic_server2.type = game_server::ServerType::LOGIC;
    logic_server2.ip = "192.168.1.102";
    logic_server2.port = 9002;
    logic_server2.status = game_server::ServerStatus::RUNNING;
    logic_server2.online_count = 300;
    logic_server2.max_online = 2000;
    center_server.RegisterServer(logic_server2);
    std::cout << "注册逻辑服务器2: ID=2002, 在线人数=300" << std::endl;

    std::cout << "\n2. 玩家进入跨服" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // 玩家1进入跨服
    center_server.PlayerEnterCross(1000001, 2001, 2002, 
                                   static_cast<int32_t>(game_server::CrossType::CROSS_BATTLE),
                                   "token_123456");
    std::cout << "玩家1000001从服务器2001进入跨服到服务器2002" << std::endl;

    // 玩家2进入跨服
    center_server.PlayerEnterCross(1000002, 2001, 2002,
                                   static_cast<int32_t>(game_server::CrossType::CROSS_BATTLE),
                                   "token_789012");
    std::cout << "玩家1000002从服务器2001进入跨服到服务器2002" << std::endl;

    std::cout << "\n3. 匹配队列测试" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // 添加玩家到匹配队列
    center_server.AddToMatchQueue(1000001, 1, 1500);  // 玩家1，匹配类型1，分数1500
    center_server.AddToMatchQueue(1000002, 1, 1600);  // 玩家2，匹配类型1，分数1600
    center_server.AddToMatchQueue(1000003, 1, 1550);  // 玩家3，匹配类型1，分数1550
    std::cout << "添加3个玩家到匹配队列" << std::endl;

    // 处理匹配（2人一队）
    std::cout << "\n处理匹配（2人一队）..." << std::endl;
    if (center_server.ProcessMatch(1, 2)) {
        std::cout << "匹配成功！已发送匹配成功通知" << std::endl;
    } else {
        std::cout << "匹配失败" << std::endl;
    }

    std::cout << "\n4. 消息路由测试" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // 路由消息
    std::string msg_data = "Hello, Server 2002!";
    if (center_server.RouteMessage(2001, 2002, msg_data)) {
        std::cout << "消息从服务器2001路由到服务器2002成功" << std::endl;
    }

    std::cout << "\n5. 广播消息测试" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // 广播到所有逻辑服务器
    std::string broadcast_msg = "系统公告：维护通知";
    if (center_server.BroadcastMessage(game_server::ServerType::LOGIC, broadcast_msg)) {
        std::cout << "广播消息到所有逻辑服务器成功" << std::endl;
    }

    // 广播到所有服务器
    std::string global_msg = "全局消息：活动开始";
    if (center_server.BroadcastToAll(global_msg)) {
        std::cout << "广播消息到所有服务器成功" << std::endl;
    }

    std::cout << "\n6. 玩家离开跨服" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // 玩家离开跨服
    if (center_server.PlayerLeaveCross(1000001)) {
        std::cout << "玩家1000001离开跨服" << std::endl;
    }

    if (center_server.PlayerLeaveCross(1000002)) {
        std::cout << "玩家1000002离开跨服" << std::endl;
    }

    std::cout << "\n7. 查询跨服玩家信息" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // 查询玩家是否还在跨服
    if (!center_server.IsPlayerInCross(1000001)) {
        std::cout << "玩家1000001不在跨服中（已离开）" << std::endl;
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "      示例运行完成                     " << std::endl;
    std::cout << "========================================" << std::endl;

    // 停止服务器
    center_server.Stop();

    return 0;
}
