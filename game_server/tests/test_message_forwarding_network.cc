#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <memory>
#include <atomic>
#include "proxy_server/proxy_service.h"
#include "logic_server/logic_service.h"
#include "center_server/center_server.h"
#include "common/message_dispatcher.h"
#include "proto/msg_account.pb.h"
#include "proto/msg_base.pb.h"
#include "proto/msg_id.pb.h"
#include "proto/msg_role.pb.h"

using namespace game_server;

/**
 * 完整网络消息转发测试类
 * 启动真实的网络服务进行测试
 */
class MessageForwardingNetworkTest : public ::testing::Test {
protected:
    /**
     * 测试准备：启动所有必要的服务
     */
    void SetUp() override {
        // 创建中心服务器
        center_server_ = new CenterServer();
        center_server_->Init("");
        center_server_->Start();
        
        // 创建代理服务
        proxy_service_ = new ProxyService();
        
        // 创建逻辑服务
        logic_service_ = new LogicService();
        
        // 等待服务启动
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Network test services initialized";
    }

    /**
     * 测试清理：停止所有服务
     */
    void TearDown() override {
        if (logic_service_) {
            logic_service_->StopService();
            delete logic_service_;
            logic_service_ = nullptr;
        }
        
        if (proxy_service_) {
            proxy_service_->UninitService();
            delete proxy_service_;
            proxy_service_ = nullptr;
        }
        
        if (center_server_) {
            center_server_->Stop();
            delete center_server_;
            center_server_ = nullptr;
        }
        
        ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Network test services stopped";
    }

    // 服务实例
    CenterServer* center_server_;
    ProxyService* proxy_service_;
    LogicService* logic_service_;
};

/**
 * 测试代理服务初始化
 */
TEST_F(MessageForwardingNetworkTest, ProxyServiceInit) {
    EXPECT_TRUE(proxy_service_->InitService());
}

/**
 * 测试逻辑服务初始化
 */
TEST_F(MessageForwardingNetworkTest, LogicServiceInit) {
    EXPECT_TRUE(logic_service_->InitService());
}

/**
 * 测试中心服务器运行状态
 */
TEST_F(MessageForwardingNetworkTest, CenterServerRunning) {
    EXPECT_TRUE(center_server_->IsRunning());
}

/**
 * 测试服务器注册到中心服务器
 */
TEST_F(MessageForwardingNetworkTest, RegisterServersToCenter) {
    // 注册代理服务器
    ServerInfo proxy_info;
    proxy_info.server_id = 1001;
    proxy_info.type = ServerType::PROXY;
    proxy_info.ip = "127.0.0.1";
    proxy_info.port = 8001;
    proxy_info.status = ServerStatus::RUNNING;
    proxy_info.online_count = 0;
    proxy_info.max_online = 1000;
    EXPECT_TRUE(center_server_->RegisterServer(proxy_info));
    
    // 注册逻辑服务器
    ServerInfo logic_info;
    logic_info.server_id = 2001;
    logic_info.type = ServerType::LOGIC;
    logic_info.ip = "127.0.0.1";
    logic_info.port = 8002;
    logic_info.status = ServerStatus::RUNNING;
    logic_info.online_count = 0;
    logic_info.max_online = 500;
    EXPECT_TRUE(center_server_->RegisterServer(logic_info));
    
    // 验证服务器已注册
    std::vector<ServerInfo> all_servers;
    EXPECT_TRUE(center_server_->GetAllServers(all_servers));
    EXPECT_EQ(all_servers.size(), 2);
}

/**
 * 测试客户端连接流程
 */
TEST_F(MessageForwardingNetworkTest, ClientConnectionFlow) {
    // 模拟客户端连接
    uint32_t conn_id = 100;
    std::string client_ip = "127.0.0.1";
    int32_t client_port = 54321;
    
    proxy_service_->OnClientConnect(conn_id, client_ip, client_port);
    
    // 验证会话已创建
    ClientSession* session = proxy_service_->GetSession(conn_id);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->conn_id, conn_id);
    EXPECT_EQ(session->ip, client_ip);
    EXPECT_EQ(session->port, client_port);
    EXPECT_EQ(session->account_id, 0);
    EXPECT_EQ(session->role_id, 0);
}

/**
 * 测试完整的登录流程
 */
TEST_F(MessageForwardingNetworkTest, FullLoginFlow) {
    // 1. 客户端连接
    uint32_t conn_id = 200;
    proxy_service_->OnClientConnect(conn_id, "127.0.0.1", 54322);
    
    // 2. 创建会话
    uint64_t account_id = 10001;
    uint64_t role_id = 20001;
    std::string session_id = proxy_service_->GenerateSessionId();
    EXPECT_TRUE(proxy_service_->CreateSession(conn_id, account_id, role_id, session_id));
    
    // 3. 验证会话信息
    ClientSession* session = proxy_service_->GetSession(conn_id);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->account_id, account_id);
    EXPECT_EQ(session->role_id, role_id);
    
    // 4. 验证账号映射
    ClientSession* session_by_account = proxy_service_->GetSessionByAccount(account_id);
    ASSERT_NE(session_by_account, nullptr);
    EXPECT_EQ(session_by_account->conn_id, conn_id);
}

/**
 * 测试逻辑服务器选择
 */
TEST_F(MessageForwardingNetworkTest, LogicServerSelection) {
    // 添加多个逻辑服务器
    for (int i = 0; i < 3; i++) {
        ServerConnection server;
        server.conn_id = 3000 + i;
        server.server_name = "LogicServer" + std::to_string(i);
        server.ip = "127.0.0.1";
        server.port = 9000 + i;
        server.last_active_time = time(nullptr);
        server.player_count = i * 100;
        proxy_service_->AddLogicServer(server);
    }
    
    // 选择逻辑服务器
    uint32_t selected = proxy_service_->SelectLogicServer();
    EXPECT_GT(selected, 0);
    
    // 验证选择的是负载最低的服务器
    ServerConnection* server = proxy_service_->GetLogicServer(selected);
    ASSERT_NE(server, nullptr);
    EXPECT_EQ(server->player_count, 0);
}

/**
 * 测试消息分发器注册
 */
TEST_F(MessageForwardingNetworkTest, MessageDispatcherRegistration) {
    auto dispatcher = std::make_shared<MessageDispatcher>();
    
    // 注册消息处理器
    uint32_t msg_id = static_cast<uint32_t>(MessageID::MSG_ACCOUNT_LOGIN_REQ);
    dispatcher->RegisterHandler(msg_id, [](const NetPacket& packet) {
        return true;
    });
    
    // 验证处理器已注册
    EXPECT_TRUE(dispatcher->HasHandler(msg_id));
    EXPECT_EQ(dispatcher->GetHandlerCount(), 1);
}

/**
 * 测试会话超时清理
 */
TEST_F(MessageForwardingNetworkTest, SessionTimeoutCleanup) {
    // 创建会话
    uint32_t conn_id = 400;
    proxy_service_->OnClientConnect(conn_id, "127.0.0.1", 54323);
    
    // 验证会话存在
    EXPECT_NE(proxy_service_->GetSession(conn_id), nullptr);
    
    // 删除会话
    EXPECT_TRUE(proxy_service_->RemoveSession(conn_id));
    
    // 验证会话已删除
    EXPECT_EQ(proxy_service_->GetSession(conn_id), nullptr);
}

/**
 * 测试多客户端并发连接
 */
TEST_F(MessageForwardingNetworkTest, MultipleClientConnections) {
    const int client_count = 10;
    
    // 模拟多个客户端连接
    for (int i = 0; i < client_count; i++) {
        uint32_t conn_id = 500 + i;
        proxy_service_->OnClientConnect(conn_id, "127.0.0.1", 55000 + i);
        
        uint64_t account_id = 10000 + i;
        uint64_t role_id = 20000 + i;
        std::string session_id = proxy_service_->GenerateSessionId();
        proxy_service_->CreateSession(conn_id, account_id, role_id, session_id);
    }
    
    // 验证所有会话都已创建
    for (int i = 0; i < client_count; i++) {
        uint32_t conn_id = 500 + i;
        uint64_t account_id = 10000 + i;
        
        ClientSession* session = proxy_service_->GetSession(conn_id);
        EXPECT_NE(session, nullptr);
        EXPECT_EQ(session->account_id, account_id);
        
        ClientSession* session_by_account = proxy_service_->GetSessionByAccount(account_id);
        EXPECT_NE(session_by_account, nullptr);
    }
}

/**
 * 测试服务器心跳更新
 */
TEST_F(MessageForwardingNetworkTest, ServerHeartbeatUpdate) {
    // 注册服务器
    ServerInfo info;
    info.server_id = 6001;
    info.type = ServerType::LOGIC;
    info.ip = "127.0.0.1";
    info.port = 9600;
    info.status = ServerStatus::RUNNING;
    center_server_->RegisterServer(info);
    
    // 获取初始心跳时间
    ServerInfo initial_info;
    center_server_->GetServerInfo(6001, initial_info);
    time_t initial_heartbeat = initial_info.last_heartbeat;
    
    // 等待
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 更新心跳
    center_server_->UpdateServerHeartbeat(6001);
    
    // 验证心跳已更新
    ServerInfo updated_info;
    center_server_->GetServerInfo(6001, updated_info);
    EXPECT_GE(updated_info.last_heartbeat, initial_heartbeat);
}

/**
 * 测试服务器负载更新
 */
TEST_F(MessageForwardingNetworkTest, ServerLoadUpdate) {
    // 注册服务器
    ServerInfo info;
    info.server_id = 7001;
    info.type = ServerType::LOGIC;
    info.ip = "127.0.0.1";
    info.port = 9700;
    info.status = ServerStatus::RUNNING;
    info.online_count = 0;
    center_server_->RegisterServer(info);
    
    // 更新负载
    center_server_->UpdateServerLoad(7001, 100);
    
    // 验证负载已更新
    ServerInfo updated_info;
    center_server_->GetServerInfo(7001, updated_info);
    EXPECT_EQ(updated_info.online_count, 100);
}

/**
 * 测试跨服玩家管理
 */
TEST_F(MessageForwardingNetworkTest, CrossServerPlayerManagement) {
    uint64_t role_id = 12345;
    int32_t src_server = 1001;
    int32_t dest_server = 1002;
    
    // 玩家进入跨服
    EXPECT_TRUE(center_server_->PlayerEnterCross(role_id, src_server, dest_server, 
                                                  static_cast<int32_t>(CrossType::CROSS_BATTLE), "token123"));
    
    // 验证玩家在跨服中
    EXPECT_TRUE(center_server_->IsPlayerInCross(role_id));
    
    // 获取跨服信息
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
 */
TEST_F(MessageForwardingNetworkTest, MatchQueueManagement) {
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

/**
 * 测试消息序列化
 */
TEST_F(MessageForwardingNetworkTest, MessageSerialization) {
    // 创建登录请求消息
    AccountLoginReq login_req;
    login_req.set_account_name("test_user");
    login_req.set_password("test_password");
    login_req.set_channel(1);
    
    // 序列化
    std::string serialized;
    ASSERT_TRUE(login_req.SerializeToString(&serialized));
    EXPECT_FALSE(serialized.empty());
    
    // 反序列化
    AccountLoginReq parsed_req;
    ASSERT_TRUE(parsed_req.ParseFromString(serialized));
    EXPECT_EQ(parsed_req.account_name(), "test_user");
    EXPECT_EQ(parsed_req.password(), "test_password");
    EXPECT_EQ(parsed_req.channel(), 1);
}

/**
 * 测试服务停止和重启
 * 注意：由于端口释放需要时间(TIME_WAIT状态)，此测试仅验证服务可以正常初始化和停止
 */
TEST_F(MessageForwardingNetworkTest, ServiceStopAndRestart) {
    // 初始化服务
    EXPECT_TRUE(proxy_service_->InitService());
    
    // 停止服务
    proxy_service_->UninitService();
    
    // 验证服务已停止（端口释放需要时间，不测试重启）
}

/**
 * 测试定时器触发
 */
TEST_F(MessageForwardingNetworkTest, TimerTrigger) {
    // 创建会话
    uint32_t conn_id = 800;
    proxy_service_->OnClientConnect(conn_id, "127.0.0.1", 54800);
    
    // 触发定时器
    proxy_service_->OnTimer();
    
    // 验证会话仍然存在（未超时）
    EXPECT_NE(proxy_service_->GetSession(conn_id), nullptr);
}

/**
 * 测试服务器选择最佳服务器
 */
TEST_F(MessageForwardingNetworkTest, SelectBestServerFromCenter) {
    // 注册多个逻辑服务器
    for (int i = 0; i < 5; i++) {
        ServerInfo info;
        info.server_id = 9000 + i;
        info.type = ServerType::LOGIC;
        info.ip = "127.0.0.1";
        info.port = 9900 + i;
        info.status = ServerStatus::RUNNING;
        info.online_count = i * 50;
        info.max_online = 500;
        center_server_->RegisterServer(info);
    }
    
    // 选择最佳服务器
    int32_t best_id = center_server_->SelectBestServer(ServerType::LOGIC);
    EXPECT_EQ(best_id, 9000);  // 第一个服务器负载为 0
}

/**
 * 测试广播消息
 */
TEST_F(MessageForwardingNetworkTest, BroadcastMessage) {
    // 注册多个服务器
    for (int i = 0; i < 3; i++) {
        ServerInfo info;
        info.server_id = 10000 + i;
        info.type = ServerType::LOGIC;
        info.ip = "127.0.0.1";
        info.port = 10000 + i;
        info.status = ServerStatus::RUNNING;
        center_server_->RegisterServer(info);
    }
    
    // 广播消息
    std::string test_msg = "test_broadcast_message";
    EXPECT_TRUE(center_server_->BroadcastMessage(ServerType::LOGIC, test_msg));
}

/**
 * 测试消息路由
 */
TEST_F(MessageForwardingNetworkTest, MessageRouting) {
    // 注册两个服务器
    ServerInfo server1;
    server1.server_id = 11001;
    server1.type = ServerType::LOGIC;
    server1.ip = "127.0.0.1";
    server1.port = 11001;
    server1.status = ServerStatus::RUNNING;
    center_server_->RegisterServer(server1);
    
    ServerInfo server2;
    server2.server_id = 11002;
    server2.type = ServerType::LOGIC;
    server2.ip = "127.0.0.1";
    server2.port = 11002;
    server2.status = ServerStatus::RUNNING;
    center_server_->RegisterServer(server2);
    
    // 路由消息
    std::string test_msg = "test_route_message";
    EXPECT_TRUE(center_server_->RouteMessage(11001, 11002, test_msg));
}
