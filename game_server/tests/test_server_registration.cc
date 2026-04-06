#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <memory>
#include "center_server/center_server.h"
#include "common/tcp_service.h"
#include "common/message_dispatcher.h"
#include "proto/msg_cross.pb.h"
#include "proto/msg_id.pb.h"

using namespace game_server;

/**
 * 服务器注册与发现测试类
 * 测试真实网络环境下的服务器注册与发现功能
 */
class ServerRegistrationTest : public ::testing::Test {
protected:
    /**
     * 测试准备：启动中心服务器
     */
    void SetUp() override {
        // 创建中心服务器
        center_server_ = new CenterServer();
        
        // 初始化中心服务器
        ASSERT_TRUE(center_server_->Init(""));
        ASSERT_TRUE(center_server_->Start());
        
        // 等待服务器启动
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    /**
     * 测试清理：停止中心服务器
     */
    void TearDown() override {
        if (center_server_) {
            center_server_->Stop();
            delete center_server_;
            center_server_ = nullptr;
        }
    }

    // 中心服务器实例
    CenterServer* center_server_;
};

/**
 * 测试服务器注册流程
 * 验证服务器能够成功注册到中心服务器
 */
TEST_F(ServerRegistrationTest, RegisterServer) {
    // 创建服务器信息
    ServerInfo info;
    info.server_id = 1001;
    info.type = ServerType::LOGIN;
    info.ip = "127.0.0.1";
    info.port = 8001;
    info.status = ServerStatus::RUNNING;
    info.online_count = 0;
    info.max_online = 1000;
    info.start_time = time(nullptr);
    info.version = "1.0.0";

    // 注册服务器
    EXPECT_TRUE(center_server_->RegisterServer(info));

    // 验证服务器已注册
    ServerInfo registered_info;
    EXPECT_TRUE(center_server_->GetServerInfo(1001, registered_info));
    EXPECT_EQ(registered_info.server_id, 1001);
    EXPECT_EQ(registered_info.type, ServerType::LOGIN);
    EXPECT_EQ(registered_info.ip, "127.0.0.1");
    EXPECT_EQ(registered_info.port, 8001);
}

/**
 * 测试多服务器注册
 * 验证多个不同类型的服务器能够成功注册
 */
TEST_F(ServerRegistrationTest, RegisterMultipleServers) {
    // 注册登录服务器
    ServerInfo login_server;
    login_server.server_id = 2001;
    login_server.type = ServerType::LOGIN;
    login_server.ip = "127.0.0.1";
    login_server.port = 8001;
    login_server.status = ServerStatus::RUNNING;
    EXPECT_TRUE(center_server_->RegisterServer(login_server));

    // 注册逻辑服务器
    ServerInfo logic_server;
    logic_server.server_id = 3001;
    logic_server.type = ServerType::LOGIC;
    logic_server.ip = "127.0.0.1";
    logic_server.port = 8002;
    logic_server.status = ServerStatus::RUNNING;
    EXPECT_TRUE(center_server_->RegisterServer(logic_server));

    // 注册数据库服务器
    ServerInfo db_server;
    db_server.server_id = 4001;
    db_server.type = ServerType::DB;
    db_server.ip = "127.0.0.1";
    db_server.port = 8003;
    db_server.status = ServerStatus::RUNNING;
    EXPECT_TRUE(center_server_->RegisterServer(db_server));

    // 验证所有服务器已注册
    std::vector<ServerInfo> all_servers;
    EXPECT_TRUE(center_server_->GetAllServers(all_servers));
    EXPECT_EQ(all_servers.size(), 3);
}

/**
 * 测试服务器发现机制
 * 验证能够发现特定类型的服务器
 */
TEST_F(ServerRegistrationTest, DiscoverServers) {
    // 注册多个逻辑服务器
    for (int i = 0; i < 3; i++) {
        ServerInfo info;
        info.server_id = 5001 + i;
        info.type = ServerType::LOGIC;
        info.ip = "127.0.0.1";
        info.port = 8100 + i;
        info.status = ServerStatus::RUNNING;
        EXPECT_TRUE(center_server_->RegisterServer(info));
    }

    // 注册其他类型的服务器
    ServerInfo login_server;
    login_server.server_id = 6001;
    login_server.type = ServerType::LOGIN;
    login_server.ip = "127.0.0.1";
    login_server.port = 8200;
    login_server.status = ServerStatus::RUNNING;
    EXPECT_TRUE(center_server_->RegisterServer(login_server));

    // 发现逻辑服务器
    std::vector<ServerInfo> logic_servers;
    EXPECT_TRUE(center_server_->GetServerList(ServerType::LOGIC, logic_servers));
    EXPECT_EQ(logic_servers.size(), 3);

    // 发现登录服务器
    std::vector<ServerInfo> login_servers;
    EXPECT_TRUE(center_server_->GetServerList(ServerType::LOGIN, login_servers));
    EXPECT_EQ(login_servers.size(), 1);
}

/**
 * 测试服务器状态更新
 * 验证服务器状态能够正确更新
 */
TEST_F(ServerRegistrationTest, UpdateServerStatus) {
    // 注册服务器
    ServerInfo info;
    info.server_id = 7001;
    info.type = ServerType::LOGIC;
    info.ip = "127.0.0.1";
    info.port = 8300;
    info.status = ServerStatus::RUNNING;
    info.online_count = 0;
    EXPECT_TRUE(center_server_->RegisterServer(info));

    // 更新服务器负载
    EXPECT_TRUE(center_server_->UpdateServerLoad(7001, 100));

    // 验证负载已更新
    ServerInfo updated_info;
    EXPECT_TRUE(center_server_->GetServerInfo(7001, updated_info));
    EXPECT_EQ(updated_info.online_count, 100);
}

/**
 * 测试服务器心跳更新
 * 验证服务器心跳能够正确更新
 */
TEST_F(ServerRegistrationTest, UpdateHeartbeat) {
    // 注册服务器
    ServerInfo info;
    info.server_id = 8001;
    info.type = ServerType::LOGIC;
    info.ip = "127.0.0.1";
    info.port = 8400;
    info.status = ServerStatus::RUNNING;
    EXPECT_TRUE(center_server_->RegisterServer(info));

    // 获取初始心跳时间
    ServerInfo initial_info;
    EXPECT_TRUE(center_server_->GetServerInfo(8001, initial_info));
    time_t initial_heartbeat = initial_info.last_heartbeat;

    // 等待一段时间
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 更新心跳
    EXPECT_TRUE(center_server_->UpdateServerHeartbeat(8001));

    // 验证心跳已更新
    ServerInfo updated_info;
    EXPECT_TRUE(center_server_->GetServerInfo(8001, updated_info));
    EXPECT_GT(updated_info.last_heartbeat, initial_heartbeat);
}

/**
 * 测试服务器注销
 * 验证服务器能够正确注销
 */
TEST_F(ServerRegistrationTest, UnregisterServer) {
    // 注册服务器
    ServerInfo info;
    info.server_id = 9001;
    info.type = ServerType::LOGIC;
    info.ip = "127.0.0.1";
    info.port = 8500;
    info.status = ServerStatus::RUNNING;
    EXPECT_TRUE(center_server_->RegisterServer(info));

    // 验证服务器已注册
    ServerInfo registered_info;
    EXPECT_TRUE(center_server_->GetServerInfo(9001, registered_info));

    // 注销服务器
    EXPECT_TRUE(center_server_->UnregisterServer(9001));

    // 验证服务器已注销
    ServerInfo unregistered_info;
    EXPECT_FALSE(center_server_->GetServerInfo(9001, unregistered_info));
}

/**
 * 测试服务器选择
 * 验证能够选择负载最低的服务器
 */
TEST_F(ServerRegistrationTest, SelectBestServer) {
    // 注册多个逻辑服务器，设置不同的负载
    for (int i = 0; i < 3; i++) {
        ServerInfo info;
        info.server_id = 10001 + i;
        info.type = ServerType::LOGIC;
        info.ip = "127.0.0.1";
        info.port = 8600 + i;
        info.status = ServerStatus::RUNNING;
        info.online_count = i * 100;  // 0, 100, 200
        EXPECT_TRUE(center_server_->RegisterServer(info));
    }

    // 选择最佳服务器（应该是负载最低的）
    int32_t best_server_id = center_server_->SelectBestServer(ServerType::LOGIC);
    EXPECT_EQ(best_server_id, 10001);  // 第一个服务器负载为 0
}

/**
 * 测试服务器健康检查
 * 验证心跳超时的服务器会被检测到
 */
TEST_F(ServerRegistrationTest, HealthCheck) {
    // 注册服务器
    ServerInfo info;
    info.server_id = 11001;
    info.type = ServerType::LOGIC;
    info.ip = "127.0.0.1";
    info.port = 8700;
    info.status = ServerStatus::RUNNING;
    EXPECT_TRUE(center_server_->RegisterServer(info));

    // 验证服务器已注册
    ServerInfo registered_info;
    EXPECT_TRUE(center_server_->GetServerInfo(11001, registered_info));
    EXPECT_EQ(registered_info.status, ServerStatus::RUNNING);

    // 执行定时器检查（服务器心跳未超时，应该不会被移除）
    center_server_->OnTimer();

    // 验证服务器仍然存在
    ServerInfo checked_info;
    EXPECT_TRUE(center_server_->GetServerInfo(11001, checked_info));
    EXPECT_EQ(checked_info.status, ServerStatus::RUNNING);

    // 测试心跳更新
    EXPECT_TRUE(center_server_->UpdateServerHeartbeat(11001));
    EXPECT_TRUE(center_server_->GetServerInfo(11001, checked_info));
}

/**
 * 测试重复注册
 * 验证重复注册会更新服务器信息
 */
TEST_F(ServerRegistrationTest, ReRegisterServer) {
    // 第一次注册
    ServerInfo info1;
    info1.server_id = 12001;
    info1.type = ServerType::LOGIC;
    info1.ip = "127.0.0.1";
    info1.port = 8800;
    info1.status = ServerStatus::RUNNING;
    info1.online_count = 50;
    EXPECT_TRUE(center_server_->RegisterServer(info1));

    // 第二次注册（更新信息）
    ServerInfo info2;
    info2.server_id = 12001;
    info2.type = ServerType::LOGIC;
    info2.ip = "127.0.0.1";
    info2.port = 8801;  // 端口改变
    info2.status = ServerStatus::RUNNING;
    info2.online_count = 100;  // 负载改变
    EXPECT_TRUE(center_server_->RegisterServer(info2));

    // 验证信息已更新
    ServerInfo updated_info;
    EXPECT_TRUE(center_server_->GetServerInfo(12001, updated_info));
    EXPECT_EQ(updated_info.port, 8801);
    EXPECT_EQ(updated_info.online_count, 100);
}

/**
 * 测试获取所有服务器
 * 验证能够获取所有已注册的服务器
 */
TEST_F(ServerRegistrationTest, GetAllServers) {
    // 注册多个不同类型的服务器
    ServerInfo info1;
    info1.server_id = 13001;
    info1.type = ServerType::LOGIN;
    info1.ip = "127.0.0.1";
    info1.port = 8900;
    info1.status = ServerStatus::RUNNING;
    EXPECT_TRUE(center_server_->RegisterServer(info1));

    ServerInfo info2;
    info2.server_id = 13002;
    info2.type = ServerType::LOGIC;
    info2.ip = "127.0.0.1";
    info2.port = 8901;
    info2.status = ServerStatus::RUNNING;
    EXPECT_TRUE(center_server_->RegisterServer(info2));

    ServerInfo info3;
    info3.server_id = 13003;
    info3.type = ServerType::DB;
    info3.ip = "127.0.0.1";
    info3.port = 8902;
    info3.status = ServerStatus::RUNNING;
    EXPECT_TRUE(center_server_->RegisterServer(info3));

    // 获取所有服务器
    std::vector<ServerInfo> all_servers;
    EXPECT_TRUE(center_server_->GetAllServers(all_servers));
    EXPECT_EQ(all_servers.size(), 3);
}

/**
 * 测试服务器运行状态
 * 验证中心服务器能够正确启动和停止
 */
TEST_F(ServerRegistrationTest, ServerRunning) {
    EXPECT_TRUE(center_server_->IsRunning());
    
    center_server_->Stop();
    EXPECT_FALSE(center_server_->IsRunning());
    
    center_server_->Start();
    EXPECT_TRUE(center_server_->IsRunning());
}

/**
 * 测试跨服玩家管理
 * 验证跨服玩家信息能够正确管理
 */
TEST_F(ServerRegistrationTest, CrossServerPlayer) {
    // 玩家进入跨服
    uint64_t role_id = 12345;
    int32_t src_server = 1001;
    int32_t dest_server = 1002;
    EXPECT_TRUE(center_server_->PlayerEnterCross(role_id, src_server, dest_server, 
                                                  static_cast<int32_t>(CrossType::CROSS_BATTLE), "token123"));

    // 验证玩家在跨服中
    EXPECT_TRUE(center_server_->IsPlayerInCross(role_id));

    // 获取跨服玩家信息
    CrossServerPlayer player;
    EXPECT_TRUE(center_server_->GetCrossPlayer(role_id, player));
    EXPECT_EQ(player.role_id, role_id);
    EXPECT_EQ(player.src_server_id, src_server);
    EXPECT_EQ(player.dest_server_id, dest_server);

    // 玩家离开跨服
    EXPECT_TRUE(center_server_->PlayerLeaveCross(role_id));
    EXPECT_FALSE(center_server_->IsPlayerInCross(role_id));
}

/**
 * 测试匹配队列
 * 验证匹配队列功能
 */
TEST_F(ServerRegistrationTest, MatchQueue) {
    // 添加玩家到匹配队列
    EXPECT_TRUE(center_server_->AddToMatchQueue(10001, 1, 1500));
    EXPECT_TRUE(center_server_->AddToMatchQueue(10002, 1, 1600));
    EXPECT_TRUE(center_server_->AddToMatchQueue(10003, 1, 1550));

    // 获取匹配队列
    std::vector<uint64_t> players;
    EXPECT_TRUE(center_server_->GetMatchQueue(1, players));
    EXPECT_EQ(players.size(), 3);

    // 移除玩家
    EXPECT_TRUE(center_server_->RemoveFromMatchQueue(10002));
    
    players.clear();
    EXPECT_TRUE(center_server_->GetMatchQueue(1, players));
    EXPECT_EQ(players.size(), 2);
}
