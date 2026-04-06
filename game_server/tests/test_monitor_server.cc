#include <gtest/gtest.h>
#include "monitor_server/monitor_server.h"

using namespace game_server;

class MonitorServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        monitor_server_ = new MonitorServer();
    }
    
    void TearDown() override {
        delete monitor_server_;
    }
    
    MonitorServer* monitor_server_;
};

TEST_F(MonitorServerTest, Init) {
    EXPECT_TRUE(monitor_server_->Init("config/monitor_server.yaml"));
}

TEST_F(MonitorServerTest, StartStop) {
    monitor_server_->Init("config/monitor_server.yaml");
    EXPECT_TRUE(monitor_server_->Start());
    monitor_server_->Stop();
}

TEST_F(MonitorServerTest, UpdateServerStatus) {
    monitor_server_->Init("config/monitor_server.yaml");
    
    ServerPerfData data;
    data.server_id = 1;
    data.server_name = "LogicServer1";
    data.server_type = 3;
    data.status = MonitorServerStatus::ONLINE;
    data.online_count = 100;
    data.max_online = 1000;
    data.cpu_usage = 50.0;
    data.memory_usage = 1024.0;
    data.network_in = 1000;
    data.network_out = 2000;
    data.message_queue_size = 10;
    data.db_query_count = 100;
    data.db_query_time = 5;
    data.update_time = time(nullptr);
    
    EXPECT_TRUE(monitor_server_->UpdateServerStatus(data));
}

TEST_F(MonitorServerTest, GetServerStatus) {
    monitor_server_->Init("config/monitor_server.yaml");
    
    ServerPerfData data;
    data.server_id = 1;
    data.server_name = "LogicServer1";
    data.server_type = 3;
    data.status = MonitorServerStatus::ONLINE;
    data.cpu_usage = 50.0;
    data.update_time = time(nullptr);
    monitor_server_->UpdateServerStatus(data);
    
    ServerPerfData retrieved;
    EXPECT_TRUE(monitor_server_->GetServerStatus(1, retrieved));
    EXPECT_EQ(retrieved.server_id, 1);
    EXPECT_DOUBLE_EQ(retrieved.cpu_usage, 50.0);
}

TEST_F(MonitorServerTest, GetAllServerStatus) {
    monitor_server_->Init("config/monitor_server.yaml");
    
    ServerPerfData data1;
    data1.server_id = 1;
    data1.cpu_usage = 50.0;
    data1.update_time = time(nullptr);
    monitor_server_->UpdateServerStatus(data1);
    
    ServerPerfData data2;
    data2.server_id = 2;
    data2.cpu_usage = 60.0;
    data2.update_time = time(nullptr);
    monitor_server_->UpdateServerStatus(data2);
    
    std::vector<ServerPerfData> servers;
    EXPECT_TRUE(monitor_server_->GetAllServerStatus(servers));
    EXPECT_EQ(servers.size(), 2);
}

TEST_F(MonitorServerTest, AddAlert) {
    monitor_server_->Init("config/monitor_server.yaml");
    
    AlertInfo alert;
    alert.alert_id = 0;
    alert.type = AlertType::CPU_HIGH;
    alert.server_id = 1;
    alert.message = "CPU usage is high";
    alert.alert_time = time(nullptr);
    alert.is_handled = false;
    
    EXPECT_TRUE(monitor_server_->AddAlert(alert));
}

TEST_F(MonitorServerTest, GetAlerts) {
    monitor_server_->Init("config/monitor_server.yaml");
    
    AlertInfo alert;
    alert.type = AlertType::CPU_HIGH;
    alert.server_id = 1;
    alert.message = "CPU usage is high";
    alert.alert_time = time(nullptr);
    monitor_server_->AddAlert(alert);
    
    std::vector<AlertInfo> alerts;
    EXPECT_TRUE(monitor_server_->GetAlerts(alerts));
}

TEST_F(MonitorServerTest, HandleAlert) {
    monitor_server_->Init("config/monitor_server.yaml");
    
    AlertInfo alert;
    alert.type = AlertType::CPU_HIGH;
    alert.server_id = 1;
    alert.message = "CPU usage is high";
    alert.alert_time = time(nullptr);
    monitor_server_->AddAlert(alert);
    
    std::vector<AlertInfo> alerts;
    monitor_server_->GetAlerts(alerts);
    if (!alerts.empty()) {
        EXPECT_TRUE(monitor_server_->HandleAlert(alerts[0].alert_id, "admin"));
    }
}

TEST_F(MonitorServerTest, SendCommand) {
    monitor_server_->Init("config/monitor_server.yaml");
    
    ServerPerfData data;
    data.server_id = 1;
    data.update_time = time(nullptr);
    monitor_server_->UpdateServerStatus(data);
    
    int32_t command_id = 0;
    EXPECT_TRUE(monitor_server_->SendCommand(1, ControlCommand::RELOAD, "", command_id));
    EXPECT_NE(command_id, 0);
}

TEST_F(MonitorServerTest, GetOnlineTrend) {
    monitor_server_->Init("config/monitor_server.yaml");
    
    ServerPerfData data;
    data.server_id = 1;
    data.online_count = 100;
    data.update_time = time(nullptr);
    monitor_server_->UpdateServerStatus(data);
    
    std::vector<std::pair<time_t, int32_t>> trend;
    EXPECT_TRUE(monitor_server_->GetOnlineTrend(1, trend));
}

TEST_F(MonitorServerTest, GetPerformanceHistory) {
    monitor_server_->Init("config/monitor_server.yaml");
    
    ServerPerfData data;
    data.server_id = 1;
    data.cpu_usage = 50.0;
    data.update_time = time(nullptr);
    monitor_server_->UpdateServerStatus(data);
    
    std::vector<ServerPerfData> history;
    EXPECT_TRUE(monitor_server_->GetPerformanceHistory(1, history));
}
