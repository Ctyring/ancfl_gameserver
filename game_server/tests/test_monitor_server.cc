#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "monitor_server/monitor_server.h"

using namespace game_server;
using namespace testing;

class MonitorServerTest : public Test {
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
    EXPECT_TRUE(monitor_server_->Init("0.0.0.0", 9997));
}

TEST_F(MonitorServerTest, StartStop) {
    monitor_server_->Init("0.0.0.0", 9997);
    EXPECT_TRUE(monitor_server_->Start());
    monitor_server_->Stop();
}

TEST_F(MonitorServerTest, RegisterServer) {
    monitor_server_->Init("0.0.0.0", 9997);
    
    EXPECT_TRUE(monitor_server_->RegisterServer(1, "logic", "127.0.0.1", 8001));
}

TEST_F(MonitorServerTest, UnregisterServer) {
    monitor_server_->Init("0.0.0.0", 9997);
    monitor_server_->RegisterServer(1, "logic", "127.0.0.1", 8001);
    
    EXPECT_TRUE(monitor_server_->UnregisterServer(1));
}

TEST_F(MonitorServerTest, UpdateServerMetrics) {
    monitor_server_->Init("0.0.0.0", 9997);
    monitor_server_->RegisterServer(1, "logic", "127.0.0.1", 8001);
    
    ServerMetrics metrics;
    metrics.cpu_usage = 50.0;
    metrics.memory_usage = 1024 * 1024 * 100;
    metrics.connection_count = 100;
    metrics.qps = 1000;
    metrics.latency = 10;
    
    EXPECT_TRUE(monitor_server_->UpdateServerMetrics(1, metrics));
}

TEST_F(MonitorServerTest, GetServerMetrics) {
    monitor_server_->Init("0.0.0.0", 9997);
    monitor_server_->RegisterServer(1, "logic", "127.0.0.1", 8001);
    
    ServerMetrics metrics;
    metrics.cpu_usage = 50.0;
    metrics.memory_usage = 1024 * 1024 * 100;
    metrics.connection_count = 100;
    metrics.qps = 1000;
    metrics.latency = 10;
    monitor_server_->UpdateServerMetrics(1, metrics);
    
    ServerMetrics retrieved;
    EXPECT_TRUE(monitor_server_->GetServerMetrics(1, retrieved));
    EXPECT_DOUBLE_EQ(retrieved.cpu_usage, 50.0);
}

TEST_F(MonitorServerTest, GetAllServerMetrics) {
    monitor_server_->Init("0.0.0.0", 9997);
    monitor_server_->RegisterServer(1, "logic", "127.0.0.1", 8001);
    monitor_server_->RegisterServer(2, "logic", "127.0.0.1", 8002);
    
    ServerMetrics metrics;
    metrics.cpu_usage = 50.0;
    monitor_server_->UpdateServerMetrics(1, metrics);
    metrics.cpu_usage = 60.0;
    monitor_server_->UpdateServerMetrics(2, metrics);
    
    std::map<int32_t, ServerMetrics> all_metrics;
    EXPECT_TRUE(monitor_server_->GetAllServerMetrics(all_metrics));
    EXPECT_EQ(all_metrics.size(), 2);
}

TEST_F(MonitorServerTest, SetAlertThreshold) {
    monitor_server_->Init("0.0.0.0", 9997);
    
    AlertThreshold threshold;
    threshold.cpu_threshold = 80.0;
    threshold.memory_threshold = 1024 * 1024 * 1024;
    threshold.latency_threshold = 100;
    
    EXPECT_TRUE(monitor_server_->SetAlertThreshold(threshold));
}

TEST_F(MonitorServerTest, CheckAlerts) {
    monitor_server_->Init("0.0.0.0", 9997);
    monitor_server_->RegisterServer(1, "logic", "127.0.0.1", 8001);
    
    AlertThreshold threshold;
    threshold.cpu_threshold = 80.0;
    monitor_server_->SetAlertThreshold(threshold);
    
    ServerMetrics metrics;
    metrics.cpu_usage = 90.0;
    monitor_server_->UpdateServerMetrics(1, metrics);
    
    std::vector<AlertInfo> alerts;
    EXPECT_TRUE(monitor_server_->CheckAlerts(alerts));
    EXPECT_GT(alerts.size(), 0);
}

TEST_F(MonitorServerTest, GetServerHistory) {
    monitor_server_->Init("0.0.0.0", 9997);
    monitor_server_->RegisterServer(1, "logic", "127.0.0.1", 8001);
    
    ServerMetrics metrics;
    metrics.cpu_usage = 50.0;
    monitor_server_->UpdateServerMetrics(1, metrics);
    
    std::vector<ServerMetrics> history;
    EXPECT_TRUE(monitor_server_->GetServerHistory(1, 3600, history));
}

TEST_F(MonitorServerTest, GetSystemOverview) {
    monitor_server_->Init("0.0.0.0", 9997);
    monitor_server_->RegisterServer(1, "logic", "127.0.0.1", 8001);
    
    SystemOverview overview;
    EXPECT_TRUE(monitor_server_->GetSystemOverview(overview));
    EXPECT_EQ(overview.server_count, 1);
}

TEST_F(MonitorServerTest, SetCollectInterval) {
    monitor_server_->Init("0.0.0.0", 9997);
    
    EXPECT_TRUE(monitor_server_->SetCollectInterval(10));
}
