#include <gtest/gtest.h>
#include "log_server/log_server.h"

using namespace game_server;

class LogServerTest : public ::testing::Test {
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

TEST_F(LogServerTest, WriteLog) {
    log_server_->Init("0.0.0.0", 9998);
    
    LogRecord record;
    record.log_id = 0;
    record.type = LogType::SYSTEM;
    record.level = LogLevel::INFO;
    record.server_id = 1;
    record.role_id = 0;
    record.content = "Test log message";
    record.log_time = time(nullptr);
    
    EXPECT_TRUE(log_server_->WriteLog(record));
}

TEST_F(LogServerTest, WriteLogAsync) {
    log_server_->Init("0.0.0.0", 9998);
    
    LogRecord record;
    record.type = LogType::SYSTEM;
    record.level = LogLevel::INFO;
    record.server_id = 1;
    record.content = "Async test log message";
    record.log_time = time(nullptr);
    
    EXPECT_TRUE(log_server_->WriteLogAsync(record));
}

TEST_F(LogServerTest, WriteLogBatch) {
    log_server_->Init("0.0.0.0", 9998);
    
    std::vector<LogRecord> records;
    for (int i = 0; i < 10; ++i) {
        LogRecord record;
        record.type = LogType::SYSTEM;
        record.level = LogLevel::INFO;
        record.server_id = 1;
        record.content = "Batch test log message " + std::to_string(i);
        record.log_time = time(nullptr);
        records.push_back(record);
    }
    
    EXPECT_TRUE(log_server_->WriteLogBatch(records));
}

TEST_F(LogServerTest, QueryLogs) {
    log_server_->Init("0.0.0.0", 9998);
    
    LogRecord record;
    record.type = LogType::SYSTEM;
    record.level = LogLevel::INFO;
    record.server_id = 1;
    record.content = "Queryable log message";
    record.log_time = time(nullptr);
    log_server_->WriteLog(record);
    
    LogQueryCondition condition;
    condition.type = LogType::SYSTEM;
    condition.server_id = 1;
    condition.start_time = record.log_time - 10;
    condition.end_time = record.log_time + 10;
    condition.limit = 10;
    condition.offset = 0;
    
    std::vector<LogRecord> results;
    EXPECT_TRUE(log_server_->QueryLogs(condition, results));
}

TEST_F(LogServerTest, GetLogCount) {
    log_server_->Init("0.0.0.0", 9998);
    
    LogRecord record;
    record.type = LogType::SYSTEM;
    record.level = LogLevel::INFO;
    record.server_id = 1;
    record.content = "Count test log message";
    record.log_time = time(nullptr);
    log_server_->WriteLog(record);
    
    int64_t count = log_server_->GetLogCount(LogType::SYSTEM, record.log_time - 10, record.log_time + 10);
    EXPECT_GE(count, 0);
}

TEST_F(LogServerTest, LogLogin) {
    log_server_->Init("0.0.0.0", 9998);
    
    EXPECT_TRUE(log_server_->LogLogin(12345, "TestPlayer", 1, "127.0.0.1", true));
    EXPECT_TRUE(log_server_->LogLogin(12345, "TestPlayer", 1, "127.0.0.1", false));
}

TEST_F(LogServerTest, LogRecharge) {
    log_server_->Init("0.0.0.0", 9998);
    
    EXPECT_TRUE(log_server_->LogRecharge(12345, "TestPlayer", 1, 100, 1001));
}

TEST_F(LogServerTest, LogConsume) {
    log_server_->Init("0.0.0.0", 9998);
    
    EXPECT_TRUE(log_server_->LogConsume(12345, "TestPlayer", 1, 1, 100, "Buy item"));
}

TEST_F(LogServerTest, LogItem) {
    log_server_->Init("0.0.0.0", 9998);
    
    EXPECT_TRUE(log_server_->LogItem(12345, "TestPlayer", 1, 1001, 10, 1, "Quest reward"));
}

TEST_F(LogServerTest, LogBattle) {
    log_server_->Init("0.0.0.0", 9998);
    
    EXPECT_TRUE(log_server_->LogBattle(12345, "TestPlayer", 1, 1, 1, 60));
}

TEST_F(LogServerTest, ClearOldLogs) {
    log_server_->Init("0.0.0.0", 9998);
    
    EXPECT_TRUE(log_server_->ClearOldLogs(30));
}

TEST_F(LogServerTest, ArchiveLogs) {
    log_server_->Init("0.0.0.0", 9998);
    
    EXPECT_TRUE(log_server_->ArchiveLogs(7));
}
