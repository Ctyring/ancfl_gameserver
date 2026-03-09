#ifndef __LOG_SERVER_H__
#define __LOG_SERVER_H__

#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <memory>
#include <queue>
#include <thread>
#include <condition_variable>
#include <fstream>

namespace game_server {

// 日志类型
enum class LogType {
    LOGIN = 1,          // 登录日志
    LOGOUT = 2,         // 登出日志
    CREATE_ROLE = 3,    // 创建角色
    DELETE_ROLE = 4,    // 删除角色
    RECHARGE = 5,       // 充值日志
    CONSUME = 6,        // 消费日志
    ITEM = 7,           // 物品日志
    TRADE = 8,          // 交易日志
    BATTLE = 9,         // 战斗日志
    TASK = 10,          // 任务日志
    GUILD = 11,         // 公会日志
    CHAT = 12,          // 聊天日志
    ERROR = 13,         // 错误日志
    SYSTEM = 14         // 系统日志
};

// 日志级别
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

// 日志记录
struct LogRecord {
    int64_t log_id;
    LogType type;
    LogLevel level;
    int32_t server_id;
    uint64_t role_id;
    std::string role_name;
    std::string content;
    std::string extra_data;
    time_t log_time;
    std::string ip;
};

// 日志查询条件
struct LogQueryCondition {
    LogType type;
    int32_t server_id;
    uint64_t role_id;
    time_t start_time;
    time_t end_time;
    int32_t limit;
    int32_t offset;
};

// 日志服务器类
class LogServer {
public:
    LogServer();
    ~LogServer();
    
    // 初始化和启动
    bool Init(const std::string& config_file);
    bool Start();
    void Stop();
    bool IsRunning();
    
    // 日志记录
    bool WriteLog(const LogRecord& record);
    bool WriteLogAsync(const LogRecord& record);
    
    // 批量写入
    bool WriteLogBatch(const std::vector<LogRecord>& records);
    
    // 日志查询
    bool QueryLogs(const LogQueryCondition& condition, std::vector<LogRecord>& logs);
    bool QueryLogById(int64_t log_id, LogRecord& record);
    
    // 日志统计
    int64_t GetLogCount(LogType type, time_t start_time, time_t end_time);
    int64_t GetRoleLogCount(uint64_t role_id, LogType type);
    
    // 日志清理
    bool ClearOldLogs(int32_t days);
    bool ArchiveLogs(int32_t days);
    
    // 运营日志
    bool LogLogin(uint64_t role_id, const std::string& role_name, int32_t server_id, const std::string& ip, bool is_login);
    bool LogRecharge(uint64_t role_id, const std::string& role_name, int32_t server_id, int32_t amount, int32_t product_id);
    bool LogConsume(uint64_t role_id, const std::string& role_name, int32_t server_id, int32_t type, int32_t amount, const std::string& reason);
    bool LogItem(uint64_t role_id, const std::string& role_name, int32_t server_id, int32_t item_id, int32_t count, int32_t type, const std::string& reason);
    bool LogTrade(uint64_t role_id, const std::string& role_name, int32_t server_id, uint64_t target_id, const std::string& target_name, const std::string& items);
    bool LogBattle(uint64_t role_id, const std::string& role_name, int32_t server_id, int32_t battle_type, int32_t result, int32_t duration);
    
    // 定时处理
    void OnTimer();
    
private:
    // 处理日志队列
    void ProcessLogQueue();
    
    // 写入日志文件
    bool WriteToFile(const LogRecord& record);
    bool WriteToDatabase(const LogRecord& record);
    
    // 生成日志ID
    int64_t GenerateLogId();
    
    // 获取日志文件名
    std::string GetLogFileName(LogType type, time_t log_time);
    
    // 日志队列
    std::queue<LogRecord> log_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    // 工作线程
    std::thread worker_thread_;
    bool stop_flag_;
    
    // 日志文件句柄缓存
    std::unordered_map<std::string, std::ofstream> file_handles_;
    std::mutex file_mutex_;
    
    // 运行状态
    bool is_running_;
    
    // 日志存储目录
    std::string log_dir_;
    
    // 数据库连接
    // TODO: 添加数据库连接
    
    // 日志保留天数
    int32_t log_retention_days_;
};

// 全局日志服务器实例
extern LogServer* g_log_server;

// 日志宏
#define LOG_SERVER_DEBUG(server_id, role_id, content) \
    if (g_log_server) { \
        LogRecord record; \
        record.type = LogType::SYSTEM; \
        record.level = LogLevel::DEBUG; \
        record.server_id = server_id; \
        record.role_id = role_id; \
        record.content = content; \
        record.log_time = time(nullptr); \
        g_log_server->WriteLogAsync(record); \
    }

#define LOG_SERVER_INFO(server_id, role_id, content) \
    if (g_log_server) { \
        LogRecord record; \
        record.type = LogType::SYSTEM; \
        record.level = LogLevel::INFO; \
        record.server_id = server_id; \
        record.role_id = role_id; \
        record.content = content; \
        record.log_time = time(nullptr); \
        g_log_server->WriteLogAsync(record); \
    }

#define LOG_SERVER_ERROR(server_id, role_id, content) \
    if (g_log_server) { \
        LogRecord record; \
        record.type = LogType::ERROR; \
        record.level = LogLevel::ERROR; \
        record.server_id = server_id; \
        record.role_id = role_id; \
        record.content = content; \
        record.log_time = time(nullptr); \
        g_log_server->WriteLogAsync(record); \
    }

} // namespace game_server

#endif // __LOG_SERVER_H__
