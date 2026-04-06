#include "monitor_server/monitor_server.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <vector>

using namespace game_server;

int total_tests = 0;
int passed_tests = 0;
int failed_tests = 0;

#define TEST_BEGIN(name) \
    std::cout << "\n========== " << name << " ==========" << std::endl; \
    total_tests++

#define ASSERT_TRUE(expr, msg) \
    do { \
        if (expr) { \
            std::cout << "[通过] " << msg << std::endl; \
            passed_tests++; \
        } else { \
            std::cout << "[失败] " << msg << std::endl; \
            failed_tests++; \
        } \
    } while(0)

#define ASSERT_FALSE(expr, msg) ASSERT_TRUE(!(expr), msg)

#define ASSERT_EQ(a, b, msg) \
    do { \
        if ((a) == (b)) { \
            std::cout << "[通过] " << msg << std::endl; \
            passed_tests++; \
        } else { \
            std::cout << "[失败] " << msg << " (期望:" << (b) << ", 实际:" << (a) << ")" << std::endl; \
            failed_tests++; \
        } \
    } while(0)

#define ASSERT_NE(a, b, msg) \
    do { \
        if ((a) != (b)) { \
            std::cout << "[通过] " << msg << std::endl; \
            passed_tests++; \
        } else { \
            std::cout << "[失败] " << msg << std::endl; \
            failed_tests++; \
        } \
    } while(0)

void TestInitAndStart() {
    TEST_BEGIN("监控服务器初始化和启动");
    
    MonitorServer server;
    ASSERT_TRUE(server.Init(""), "监控服务器初始化成功");
    ASSERT_TRUE(server.Start(), "监控服务器启动成功");
    ASSERT_TRUE(server.IsRunning(), "监控服务器运行中");
    
    server.Stop();
    ASSERT_FALSE(server.IsRunning(), "监控服务器停止成功");
}

void TestUpdateServerStatus() {
    TEST_BEGIN("更新服务器状态");
    
    MonitorServer server;
    server.Init("");
    server.Start();
    
    ServerPerfData data;
    data.server_id = 1001;
    data.server_name = "登录服务器1";
    data.server_type = 1;
    data.status = MonitorServerStatus::ONLINE;
    data.online_count = 100;
    data.max_online = 500;
    data.cpu_usage = 45.5;
    data.memory_usage = 60.2;
    data.network_in = 1024;
    data.network_out = 2048;
    data.message_queue_size = 10;
    data.db_query_count = 50;
    data.db_query_time = 100;
    data.update_time = time(nullptr);
    
    ASSERT_TRUE(server.UpdateServerStatus(data), "更新服务器状态成功");
    
    ServerPerfData retrieved;
    ASSERT_TRUE(server.GetServerStatus(1001, retrieved), "获取服务器状态成功");
    ASSERT_EQ(retrieved.server_id, 1001, "服务器ID正确");
    ASSERT_EQ(retrieved.server_name, "登录服务器1", "服务器名称正确");
    ASSERT_EQ(retrieved.online_count, 100, "在线人数正确");
    ASSERT_EQ(static_cast<int>(retrieved.cpu_usage * 10), 455, "CPU使用率正确");
    
    ServerPerfData not_exist;
    ASSERT_FALSE(server.GetServerStatus(9999, not_exist), "获取不存在的服务器状态失败");
    
    server.Stop();
}

void TestGetAllServerStatus() {
    TEST_BEGIN("获取所有服务器状态");
    
    MonitorServer server;
    server.Init("");
    server.Start();
    
    std::vector<ServerPerfData> servers;
    ASSERT_TRUE(server.GetAllServerStatus(servers), "获取所有服务器状态成功");
    ASSERT_EQ(static_cast<int>(servers.size()), 0, "初始状态服务器列表为空");
    
    ServerPerfData data1;
    data1.server_id = 1001;
    data1.server_name = "登录服务器1";
    data1.status = MonitorServerStatus::ONLINE;
    data1.online_count = 100;
    data1.cpu_usage = 30.0;
    data1.memory_usage = 40.0;
    server.UpdateServerStatus(data1);
    
    ServerPerfData data2;
    data2.server_id = 1002;
    data2.server_name = "逻辑服务器1";
    data2.status = MonitorServerStatus::ONLINE;
    data2.online_count = 200;
    data2.cpu_usage = 50.0;
    data2.memory_usage = 60.0;
    server.UpdateServerStatus(data2);
    
    ServerPerfData data3;
    data3.server_id = 1003;
    data3.server_name = "数据库服务器1";
    data3.status = MonitorServerStatus::BUSY;
    data3.online_count = 0;
    data3.cpu_usage = 70.0;
    data3.memory_usage = 80.0;
    server.UpdateServerStatus(data3);
    
    ASSERT_TRUE(server.GetAllServerStatus(servers), "获取所有服务器状态成功");
    ASSERT_EQ(static_cast<int>(servers.size()), 3, "服务器数量正确");
    
    bool found1 = false, found2 = false, found3 = false;
    for (const auto& s : servers) {
        if (s.server_id == 1001) found1 = true;
        if (s.server_id == 1002) found2 = true;
        if (s.server_id == 1003) found3 = true;
    }
    ASSERT_TRUE(found1 && found2 && found3, "所有服务器都在列表中");
    
    server.Stop();
}

void TestAlertManagement() {
    TEST_BEGIN("告警管理");
    
    MonitorServer server;
    server.Init("");
    server.Start();
    
    AlertInfo alert1;
    alert1.alert_id = 1;
    alert1.type = AlertType::CPU_HIGH;
    alert1.server_id = 1001;
    alert1.message = "CPU使用率过高";
    alert1.alert_time = time(nullptr);
    alert1.is_handled = false;
    
    ASSERT_TRUE(server.AddAlert(alert1), "添加告警成功");
    
    AlertInfo alert2;
    alert2.alert_id = 2;
    alert2.type = AlertType::MEMORY_HIGH;
    alert2.server_id = 1002;
    alert2.message = "内存使用率过高";
    alert2.alert_time = time(nullptr);
    alert2.is_handled = false;
    
    ASSERT_TRUE(server.AddAlert(alert2), "添加第二个告警成功");
    
    std::vector<AlertInfo> alerts;
    ASSERT_TRUE(server.GetAlerts(alerts, false), "获取未处理告警成功");
    ASSERT_EQ(static_cast<int>(alerts.size()), 2, "未处理告警数量正确");
    
    ASSERT_TRUE(server.HandleAlert(1, "管理员"), "处理告警成功");
    
    ASSERT_TRUE(server.GetAlerts(alerts, false), "再次获取未处理告警");
    ASSERT_EQ(static_cast<int>(alerts.size()), 1, "处理后告警数量减少");
    
    ASSERT_TRUE(server.GetAlerts(alerts, true), "获取所有告警（包括已处理）");
    ASSERT_EQ(static_cast<int>(alerts.size()), 2, "所有告警数量正确");
    
    ASSERT_TRUE(server.ClearAlerts(), "清空告警成功");
    ASSERT_TRUE(server.GetAlerts(alerts, true), "清空后获取告警");
    ASSERT_EQ(static_cast<int>(alerts.size()), 0, "告警已清空");
    
    server.Stop();
}

void TestAutoAlertCheck() {
    TEST_BEGIN("自动告警检测");
    
    MonitorServer server;
    server.Init("");
    server.Start();
    
    ServerPerfData data;
    data.server_id = 1001;
    data.server_name = "测试服务器";
    data.status = MonitorServerStatus::ONLINE;
    data.online_count = 460;
    data.max_online = 500;
    data.cpu_usage = 85.0;
    data.memory_usage = 90.0;
    data.update_time = time(nullptr);
    
    server.UpdateServerStatus(data);
    
    server.OnTimer();
    
    std::vector<AlertInfo> alerts;
    server.GetAlerts(alerts, false);
    
    bool has_cpu_alert = false;
    bool has_memory_alert = false;
    bool has_online_alert = false;
    
    for (const auto& alert : alerts) {
        if (alert.type == AlertType::CPU_HIGH) has_cpu_alert = true;
        if (alert.type == AlertType::MEMORY_HIGH) has_memory_alert = true;
        if (alert.type == AlertType::ONLINE_HIGH) has_online_alert = true;
    }
    
    ASSERT_TRUE(has_cpu_alert, "检测到CPU高使用率告警");
    ASSERT_TRUE(has_memory_alert, "检测到内存高使用率告警");
    ASSERT_TRUE(has_online_alert, "检测到在线人数过高告警");
    
    server.Stop();
}

void TestServerOfflineDetection() {
    TEST_BEGIN("服务器离线检测");
    
    MonitorServer server;
    server.Init("");
    server.Start();
    
    ServerPerfData data;
    data.server_id = 1001;
    data.server_name = "测试服务器";
    data.status = MonitorServerStatus::ONLINE;
    data.online_count = 100;
    data.cpu_usage = 30.0;
    data.memory_usage = 40.0;
    data.update_time = time(nullptr) - 35;
    
    server.UpdateServerStatus(data);
    
    server.OnTimer();
    
    ServerPerfData retrieved;
    server.GetServerStatus(1001, retrieved);
    ASSERT_EQ(static_cast<int>(retrieved.status), static_cast<int>(MonitorServerStatus::OFFLINE), "服务器状态变为离线");
    
    std::vector<AlertInfo> alerts;
    server.GetAlerts(alerts, false);
    
    bool has_offline_alert = false;
    for (const auto& alert : alerts) {
        if (alert.type == AlertType::SERVER_DOWN) {
            has_offline_alert = true;
        }
    }
    
    ASSERT_TRUE(has_offline_alert, "检测到服务器离线告警");
    
    server.Stop();
}

void TestControlCommand() {
    TEST_BEGIN("控制命令");
    
    MonitorServer server;
    server.Init("");
    server.Start();
    
    int32_t command_id = 0;
    ASSERT_TRUE(server.SendCommand(1001, ControlCommand::RESTART, "", command_id), "发送重启命令成功");
    ASSERT_NE(command_id, 0, "命令ID已生成");
    
    CommandResult result;
    ASSERT_TRUE(server.GetCommandResult(command_id, result), "获取命令结果成功");
    ASSERT_EQ(result.command_id, command_id, "命令ID正确");
    ASSERT_EQ(static_cast<int>(result.command), static_cast<int>(ControlCommand::RESTART), "命令类型正确");
    ASSERT_TRUE(result.success, "命令执行成功");
    
    int32_t command_id2 = 0;
    server.SendCommand(1002, ControlCommand::STOP, "force", command_id2);
    ASSERT_NE(command_id2, command_id, "不同命令生成不同ID");
    
    CommandResult not_exist;
    ASSERT_FALSE(server.GetCommandResult(99999, not_exist), "获取不存在的命令结果失败");
    
    server.Stop();
}

void TestPerformanceHistory() {
    TEST_BEGIN("性能历史数据");
    
    MonitorServer server;
    server.Init("");
    server.Start();
    
    for (int i = 0; i < 10; ++i) {
        ServerPerfData data;
        data.server_id = 1001;
        data.server_name = "测试服务器";
        data.status = MonitorServerStatus::ONLINE;
        data.online_count = 100 + i * 10;
        data.cpu_usage = 30.0 + i * 2.0;
        data.memory_usage = 40.0 + i * 1.5;
        data.update_time = time(nullptr) + i;
        
        server.UpdateServerStatus(data);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::vector<ServerPerfData> history;
    ASSERT_TRUE(server.GetPerformanceHistory(1001, history), "获取性能历史成功");
    ASSERT_EQ(static_cast<int>(history.size()), 10, "历史数据数量正确");
    
    std::vector<std::pair<time_t, int32_t>> trend;
    ASSERT_TRUE(server.GetOnlineTrend(1001, trend), "获取在线趋势成功");
    ASSERT_EQ(static_cast<int>(trend.size()), 10, "趋势数据数量正确");
    
    if (!trend.empty()) {
        ASSERT_EQ(trend[0].second, 100, "第一条趋势数据正确");
        ASSERT_EQ(trend[9].second, 190, "最后一条趋势数据正确");
    }
    
    std::vector<ServerPerfData> not_exist_history;
    ASSERT_FALSE(server.GetPerformanceHistory(9999, not_exist_history), "获取不存在服务器的历史失败");
    
    server.Stop();
}

void TestWebRequest() {
    TEST_BEGIN("WEB接口");
    
    MonitorServer server;
    server.Init("");
    server.Start();
    
    ServerPerfData data;
    data.server_id = 1001;
    data.server_name = "登录服务器";
    data.server_type = 1;
    data.status = MonitorServerStatus::ONLINE;
    data.online_count = 100;
    data.cpu_usage = 30.0;
    data.memory_usage = 40.0;
    server.UpdateServerStatus(data);
    
    std::string response;
    
    ASSERT_TRUE(server.HandleWebRequest("/api/servers", "GET", "", response), "获取服务器列表接口成功");
    ASSERT_TRUE(response.find("\"server_id\":1001") != std::string::npos, "响应包含服务器ID");
    ASSERT_TRUE(response.find("\"server_name\":\"登录服务器\"") != std::string::npos, "响应包含服务器名称");
    ASSERT_TRUE(response.find("\"online_count\":100") != std::string::npos, "响应包含在线人数");
    
    AlertInfo alert;
    alert.alert_id = 1;
    alert.type = AlertType::CPU_HIGH;
    alert.server_id = 1001;
    alert.message = "CPU使用率过高";
    alert.alert_time = time(nullptr);
    alert.is_handled = false;
    server.AddAlert(alert);
    
    ASSERT_TRUE(server.HandleWebRequest("/api/alerts", "GET", "", response), "获取告警列表接口成功");
    ASSERT_TRUE(response.find("\"alert_id\":1") != std::string::npos, "响应包含告警ID");
    ASSERT_TRUE(response.find("\"message\":\"CPU使用率过高\"") != std::string::npos, "响应包含告警消息");
    
    ASSERT_TRUE(server.HandleWebRequest("/api/command", "POST", "{\"server_id\":1001,\"command\":\"restart\"}", response), "发送命令接口成功");
    ASSERT_TRUE(response.find("\"result\":0") != std::string::npos, "命令发送成功");
    
    ASSERT_TRUE(server.HandleWebRequest("/api/unknown", "GET", "", response), "未知路径接口");
    ASSERT_TRUE(response.find("\"error\"") != std::string::npos, "未知路径返回错误");
    
    server.Stop();
}

void TestMultiThreadUpdate() {
    TEST_BEGIN("多线程更新测试");
    
    MonitorServer server;
    server.Init("");
    server.Start();
    
    const int thread_count = 5;
    const int updates_per_thread = 20;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([&server, t, updates_per_thread]() {
            for (int i = 0; i < updates_per_thread; ++i) {
                ServerPerfData data;
                data.server_id = 1001 + t;
                data.server_name = "服务器" + std::to_string(1001 + t);
                data.status = MonitorServerStatus::ONLINE;
                data.online_count = 100 + i;
                data.cpu_usage = 30.0 + i;
                data.memory_usage = 40.0 + i;
                data.update_time = time(nullptr);
                
                server.UpdateServerStatus(data);
                
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::vector<ServerPerfData> servers;
    server.GetAllServerStatus(servers);
    
    ASSERT_EQ(static_cast<int>(servers.size()), thread_count, "所有服务器状态已更新");
    
    for (const auto& s : servers) {
        ASSERT_TRUE(s.server_id >= 1001 && s.server_id < 1001 + thread_count, "服务器ID正确");
    }
    
    server.Stop();
}

void TestHistoryDataLimit() {
    TEST_BEGIN("历史数据限制测试");
    
    MonitorServer server;
    server.Init("");
    server.Start();
    
    for (int i = 0; i < 1100; ++i) {
        ServerPerfData data;
        data.server_id = 1001;
        data.server_name = "测试服务器";
        data.status = MonitorServerStatus::ONLINE;
        data.online_count = i;
        data.cpu_usage = 30.0;
        data.memory_usage = 40.0;
        data.update_time = time(nullptr);
        
        server.UpdateServerStatus(data);
    }
    
    std::vector<ServerPerfData> history;
    server.GetPerformanceHistory(1001, history);
    
    ASSERT_EQ(static_cast<int>(history.size()), 1000, "历史数据限制在1000条");
    
    server.Stop();
}

void TestAlertIdGeneration() {
    TEST_BEGIN("告警ID生成");
    
    MonitorServer server;
    server.Init("");
    server.Start();
    
    std::vector<int32_t> alert_ids;
    
    for (int i = 0; i < 10; ++i) {
        AlertInfo alert;
        alert.alert_id = 0;
        alert.type = AlertType::CPU_HIGH;
        alert.server_id = 1001;
        alert.message = "测试告警" + std::to_string(i);
        alert.alert_time = time(nullptr);
        alert.is_handled = false;
        
        server.AddAlert(alert);
        
        std::vector<AlertInfo> alerts;
        server.GetAlerts(alerts, true);
        if (!alerts.empty()) {
            alert_ids.push_back(alerts.back().alert_id);
        }
    }
    
    bool all_unique = true;
    for (size_t i = 0; i < alert_ids.size(); ++i) {
        for (size_t j = i + 1; j < alert_ids.size(); ++j) {
            if (alert_ids[i] == alert_ids[j]) {
                all_unique = false;
                break;
            }
        }
    }
    
    ASSERT_TRUE(all_unique, "所有告警ID唯一");
    
    server.Stop();
}

void TestCommandIdGeneration() {
    TEST_BEGIN("命令ID生成");
    
    MonitorServer server;
    server.Init("");
    server.Start();
    
    std::vector<int32_t> command_ids;
    
    for (int i = 0; i < 10; ++i) {
        int32_t command_id = 0;
        server.SendCommand(1001, ControlCommand::RESTART, "", command_id);
        command_ids.push_back(command_id);
    }
    
    bool all_unique = true;
    for (size_t i = 0; i < command_ids.size(); ++i) {
        for (size_t j = i + 1; j < command_ids.size(); ++j) {
            if (command_ids[i] == command_ids[j]) {
                all_unique = false;
                break;
            }
        }
    }
    
    ASSERT_TRUE(all_unique, "所有命令ID唯一");
    
    server.Stop();
}

void TestMultipleAlertTypes() {
    TEST_BEGIN("多种告警类型测试");
    
    MonitorServer server;
    server.Init("");
    server.Start();
    
    std::vector<AlertType> types = {
        AlertType::CPU_HIGH,
        AlertType::MEMORY_HIGH,
        AlertType::NETWORK_HIGH,
        AlertType::ONLINE_HIGH,
        AlertType::SERVER_DOWN,
        AlertType::DB_SLOW
    };
    
    for (size_t i = 0; i < types.size(); ++i) {
        AlertInfo alert;
        alert.alert_id = 0;
        alert.type = types[i];
        alert.server_id = 1001 + i;
        alert.message = "测试告警" + std::to_string(i);
        alert.alert_time = time(nullptr);
        alert.is_handled = false;
        
        ASSERT_TRUE(server.AddAlert(alert), "添加告警类型" + std::to_string(i) + "成功");
    }
    
    std::vector<AlertInfo> alerts;
    server.GetAlerts(alerts, true);
    ASSERT_EQ(static_cast<int>(alerts.size()), static_cast<int>(types.size()), "所有类型告警已添加");
    
    server.Stop();
}

void TestMultipleControlCommands() {
    TEST_BEGIN("多种控制命令测试");
    
    MonitorServer server;
    server.Init("");
    server.Start();
    
    std::vector<ControlCommand> commands = {
        ControlCommand::START,
        ControlCommand::STOP,
        ControlCommand::RESTART,
        ControlCommand::RELOAD,
        ControlCommand::KICK_ALL,
        ControlCommand::BROADCAST,
        ControlCommand::MAINTENANCE
    };
    
    for (size_t i = 0; i < commands.size(); ++i) {
        int32_t command_id = 0;
        ASSERT_TRUE(server.SendCommand(1001, commands[i], "", command_id), 
                   "发送命令类型" + std::to_string(i) + "成功");
        ASSERT_NE(command_id, 0, "命令ID已生成");
    }
    
    server.Stop();
}

void TestServerStatusEnum() {
    TEST_BEGIN("服务器状态枚举测试");
    
    MonitorServer server;
    server.Init("");
    server.Start();
    
    std::vector<MonitorServerStatus> statuses = {
        MonitorServerStatus::UNKNOWN,
        MonitorServerStatus::ONLINE,
        MonitorServerStatus::OFFLINE,
        MonitorServerStatus::BUSY,
        MonitorServerStatus::ERROR
    };
    
    for (size_t i = 0; i < statuses.size(); ++i) {
        ServerPerfData data;
        data.server_id = 1001 + i;
        data.server_name = "测试服务器" + std::to_string(i);
        data.status = statuses[i];
        data.online_count = 100;
        data.cpu_usage = 30.0;
        data.memory_usage = 40.0;
        data.update_time = time(nullptr);
        
        ASSERT_TRUE(server.UpdateServerStatus(data), "更新状态" + std::to_string(i) + "成功");
        
        ServerPerfData retrieved;
        server.GetServerStatus(1001 + i, retrieved);
        ASSERT_EQ(static_cast<int>(retrieved.status), static_cast<int>(statuses[i]), 
                 "状态" + std::to_string(i) + "正确");
    }
    
    server.Stop();
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "    监控服务器独立测试（不依赖GTest）   " << std::endl;
    std::cout << "========================================" << std::endl;
    
    TestInitAndStart();
    TestUpdateServerStatus();
    TestGetAllServerStatus();
    TestAlertManagement();
    TestAutoAlertCheck();
    TestServerOfflineDetection();
    TestControlCommand();
    TestPerformanceHistory();
    TestWebRequest();
    TestMultiThreadUpdate();
    TestHistoryDataLimit();
    TestAlertIdGeneration();
    TestCommandIdGeneration();
    TestMultipleAlertTypes();
    TestMultipleControlCommands();
    TestServerStatusEnum();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "测试结果统计：" << std::endl;
    std::cout << "  总测试数: " << total_tests << std::endl;
    std::cout << "  通过数: " << passed_tests << std::endl;
    std::cout << "  失败数: " << failed_tests << std::endl;
    std::cout << "  通过率: " << (total_tests > 0 ? (passed_tests * 100 / total_tests) : 0) << "%" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return failed_tests > 0 ? 1 : 0;
}
