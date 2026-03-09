#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "log_server/log_server.h"

using namespace game_server;
using namespace testing;

class LogServerTest : public Test {
protected:
    void SetUp() override {
        log_server_ = new LogServer();
    }
    
    void TearDown() override {
        delete log_server_;
    }
    
    LogServer* log_server_;
};

TEST_F(LogServerTest, Init) {
    EXPECT_TRUE(log_server_->Init("0.0.0.0", 9998));
}

TEST_F(LogServerTest, StartStop) {
    log_server_->Init("0.0.0.0", 9998);
    EXPECT_TRUE(log_server_->Start());
    log_server_->Stop();
}

TEST_F(LogServerTest, WriteLog) {
    log_server_->Init("0.0.0.0", 9998);
    
    LogEntry entry;
    entry.log_level = LogLevel::INFO;
    entry.server_id = 1;
    entry.server_type = "logic";
    entry.message = "Test log message";
    entry.timestamp = time(nullptr);
    
    EXPECT_TRUE(log_server_->WriteLog(entry));
}

TEST_F(LogServerTest, WriteBatchLogs) {
    log_server_->Init("0.0.0.0", 9998);
    
    std::vector<LogEntry> entries;
    for (int i = 0; i < 10; ++i) {
        LogEntry entry;
        entry.log_level = LogLevel::INFO;
        entry.server_id = 1;
        entry.server_type = "logic";
        entry.message = "Test log message " + std::to_string(i);
        entry.timestamp = time(nullptr);
        entries.push_back(entry);
    }
    
    EXPECT_TRUE(log_server_->WriteBatchLogs(entries));
}

TEST_F(LogServerTest, QueryLogs) {
    log_server_->Init("0.0.0.0", 9998);
    
    LogEntry entry;
    entry.log_level = LogLevel::INFO;
    entry.server_id = 1;
    entry.server_type = "logic";
    entry.message = "Queryable log message";
    entry.timestamp = time(nullptr);
    log_server_->WriteLog(entry);
    
    LogQueryCondition condition;
    condition.server_id = 1;
    condition.start_time = entry.timestamp - 10;
    condition.end_time = entry.timestamp + 10;
    
    std::vector<LogEntry> results;
    EXPECT_TRUE(log_server_->QueryLogs(condition, results));
    EXPECT_GT(results.size(), 0);
}

TEST_F(LogServerTest, SetLogLevel) {
    log_server_->Init("0.0.0.0", 9998);
    
    EXPECT_TRUE(log_server_->SetLogLevel(LogLevel::DEBUG));
    EXPECT_EQ(log_server_->GetLogLevel(), LogLevel::DEBUG);
}

TEST_F(LogServerTest, GetLogStats) {
    log_server_->Init("0.0.0.0", 9998);
    
    LogEntry entry;
    entry.log_level = LogLevel::INFO;
    entry.server_id = 1;
    entry.server_type = "logic";
    entry.message = "Test log message";
    entry.timestamp = time(nullptr);
    log_server_->WriteLog(entry);
    
    LogStats stats;
    EXPECT_TRUE(log_server_->GetLogStats(stats));
    EXPECT_GT(stats.total_count, 0);
}

TEST_F(LogServerTest, FlushLogs) {
    log_server_->Init("0.0.0.0", 9998);
    
    LogEntry entry;
    entry.log_level = LogLevel::INFO;
    entry.server_id = 1;
    entry.server_type = "logic";
    entry.message = "Test log message";
    entry.timestamp = time(nullptr);
    log_server_->WriteLog(entry);
    
    EXPECT_TRUE(log_server_->FlushLogs());
}

TEST_F(LogServerTest, SetMaxLogSize) {
    log_server_->Init("0.0.0.0", 9998);
    
    EXPECT_TRUE(log_server_->SetMaxLogSize(100 * 1024 * 1024));
}

TEST_F(LogServerTest, SetMaxLogFiles) {
    log_server_->Init("0.0.0.0", 9998);
    
    EXPECT_TRUE(log_server_->SetMaxLogFiles(10));
}

TEST_F(LogServerTest, ArchiveLogs) {
    log_server_->Init("0.0.0.0", 9998);
    
    EXPECT_TRUE(log_server_->ArchiveLogs());
}
