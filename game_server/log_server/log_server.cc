#include "log_server.h"
#include <sstream>
#include <iomanip>
#include <ctime>

namespace game_server {

LogServer* g_log_server = nullptr;

LogServer::LogServer() : is_running_(false), stop_flag_(false), log_retention_days_(30) {
}

LogServer::~LogServer() {
    Stop();
}

bool LogServer::Init(const std::string& config_file) {
    // TODO: 从配置文件加载配置
    log_dir_ = "logs";
    
    // 创建日志目录
    // TODO: 创建目录
    
    LOG_INFO("Log server initializing: config=%s, log_dir=%s", config_file.c_str(), log_dir_.c_str());
    return true;
}

bool LogServer::Start() {
    if (is_running_) {
        return true;
    }
    
    is_running_ = true;
    stop_flag_ = false;
    
    // 启动工作线程
    worker_thread_ = std::thread(&LogServer::ProcessLogQueue, this);
    
    g_log_server = this;
    
    LOG_INFO("Log server started");
    return true;
}

void LogServer::Stop() {
    if (!is_running_) {
        return;
    }
    
    stop_flag_ = true;
    queue_cv_.notify_all();
    
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    
    // 关闭所有文件句柄
    std::lock_guard<std::mutex> lock(file_mutex_);
    for (auto& pair : file_handles_) {
        if (pair.second.is_open()) {
            pair.second.close();
        }
    }
    file_handles_.clear();
    
    g_log_server = nullptr;
    is_running_ = false;
    
    LOG_INFO("Log server stopped");
}

bool LogServer::IsRunning() {
    return is_running_;
}

bool LogServer::WriteLog(const LogRecord& record) {
    // 写入文件
    if (!WriteToFile(record)) {
        return false;
    }
    
    // 写入数据库
    if (!WriteToDatabase(record)) {
        LOG_ERROR("Failed to write log to database: log_id=%lld", record.log_id);
    }
    
    return true;
}

bool LogServer::WriteLogAsync(const LogRecord& record) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    log_queue_.push(record);
    queue_cv_.notify_one();
    return true;
}

bool LogServer::WriteLogBatch(const std::vector<LogRecord>& records) {
    for (const auto& record : records) {
        WriteLogAsync(record);
    }
    return true;
}

bool LogServer::QueryLogs(const LogQueryCondition& condition, std::vector<LogRecord>& logs) {
    // TODO: 从数据库查询日志
    logs.clear();
    return true;
}

bool LogServer::QueryLogById(int64_t log_id, LogRecord& record) {
    // TODO: 从数据库查询单条日志
    return false;
}

int64_t LogServer::GetLogCount(LogType type, time_t start_time, time_t end_time) {
    // TODO: 从数据库统计日志数量
    return 0;
}

int64_t LogServer::GetRoleLogCount(uint64_t role_id, LogType type) {
    // TODO: 从数据库统计玩家日志数量
    return 0;
}

bool LogServer::ClearOldLogs(int32_t days) {
    // TODO: 清理旧日志
    LOG_INFO("Clearing old logs: days=%d", days);
    return true;
}

bool LogServer::ArchiveLogs(int32_t days) {
    // TODO: 归档旧日志
    LOG_INFO("Archiving logs: days=%d", days);
    return true;
}

bool LogServer::LogLogin(uint64_t role_id, const std::string& role_name, int32_t server_id, const std::string& ip, bool is_login) {
    LogRecord record;
    record.log_id = GenerateLogId();
    record.type = is_login ? LogType::LOGIN : LogType::LOGOUT;
    record.level = LogLevel::INFO;
    record.server_id = server_id;
    record.role_id = role_id;
    record.role_name = role_name;
    record.content = is_login ? "Player login" : "Player logout";
    record.log_time = time(nullptr);
    record.ip = ip;
    
    return WriteLogAsync(record);
}

bool LogServer::LogRecharge(uint64_t role_id, const std::string& role_name, int32_t server_id, int32_t amount, int32_t product_id) {
    LogRecord record;
    record.log_id = GenerateLogId();
    record.type = LogType::RECHARGE;
    record.level = LogLevel::INFO;
    record.server_id = server_id;
    record.role_id = role_id;
    record.role_name = role_name;
    record.content = "Player recharge";
    record.extra_data = "amount=" + std::to_string(amount) + ",product_id=" + std::to_string(product_id);
    record.log_time = time(nullptr);
    
    return WriteLogAsync(record);
}

bool LogServer::LogConsume(uint64_t role_id, const std::string& role_name, int32_t server_id, int32_t type, int32_t amount, const std::string& reason) {
    LogRecord record;
    record.log_id = GenerateLogId();
    record.type = LogType::CONSUME;
    record.level = LogLevel::INFO;
    record.server_id = server_id;
    record.role_id = role_id;
    record.role_name = role_name;
    record.content = "Player consume";
    record.extra_data = "type=" + std::to_string(type) + ",amount=" + std::to_string(amount) + ",reason=" + reason;
    record.log_time = time(nullptr);
    
    return WriteLogAsync(record);
}

bool LogServer::LogItem(uint64_t role_id, const std::string& role_name, int32_t server_id, int32_t item_id, int32_t count, int32_t type, const std::string& reason) {
    LogRecord record;
    record.log_id = GenerateLogId();
    record.type = LogType::ITEM;
    record.level = LogLevel::INFO;
    record.server_id = server_id;
    record.role_id = role_id;
    record.role_name = role_name;
    record.content = "Item change";
    record.extra_data = "item_id=" + std::to_string(item_id) + ",count=" + std::to_string(count) + ",type=" + std::to_string(type) + ",reason=" + reason;
    record.log_time = time(nullptr);
    
    return WriteLogAsync(record);
}

bool LogServer::LogTrade(uint64_t role_id, const std::string& role_name, int32_t server_id, uint64_t target_id, const std::string& target_name, const std::string& items) {
    LogRecord record;
    record.log_id = GenerateLogId();
    record.type = LogType::TRADE;
    record.level = LogLevel::INFO;
    record.server_id = server_id;
    record.role_id = role_id;
    record.role_name = role_name;
    record.content = "Player trade";
    record.extra_data = "target_id=" + std::to_string(target_id) + ",target_name=" + target_name + ",items=" + items;
    record.log_time = time(nullptr);
    
    return WriteLogAsync(record);
}

bool LogServer::LogBattle(uint64_t role_id, const std::string& role_name, int32_t server_id, int32_t battle_type, int32_t result, int32_t duration) {
    LogRecord record;
    record.log_id = GenerateLogId();
    record.type = LogType::BATTLE;
    record.level = LogLevel::INFO;
    record.server_id = server_id;
    record.role_id = role_id;
    record.role_name = role_name;
    record.content = "Battle result";
    record.extra_data = "battle_type=" + std::to_string(battle_type) + ",result=" + std::to_string(result) + ",duration=" + std::to_string(duration);
    record.log_time = time(nullptr);
    
    return WriteLogAsync(record);
}

void LogServer::OnTimer() {
    // 定期清理旧日志
    static time_t last_clear_time = 0;
    time_t now = time(nullptr);
    
    // 每天清理一次
    if (now - last_clear_time > 86400) {
        ClearOldLogs(log_retention_days_);
        last_clear_time = now;
    }
}

void LogServer::ProcessLogQueue() {
    while (!stop_flag_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        queue_cv_.wait(lock, [this] { return !log_queue_.empty() || stop_flag_; });
        
        if (stop_flag_ && log_queue_.empty()) {
            break;
        }
        
        // 批量处理日志
        std::vector<LogRecord> records;
        while (!log_queue_.empty() && records.size() < 100) {
            records.push_back(log_queue_.front());
            log_queue_.pop();
        }
        
        lock.unlock();
        
        // 写入日志
        for (const auto& record : records) {
            WriteLog(record);
        }
    }
}

bool LogServer::WriteToFile(const LogRecord& record) {
    std::string filename = GetLogFileName(record.type, record.log_time);
    std::string filepath = log_dir_ + "/" + filename;
    
    std::lock_guard<std::mutex> lock(file_mutex_);
    
    // 获取或创建文件句柄
    auto it = file_handles_.find(filepath);
    if (it == file_handles_.end()) {
        file_handles_[filepath].open(filepath, std::ios::app);
        it = file_handles_.find(filepath);
    }
    
    if (!it->second.is_open()) {
        LOG_ERROR("Failed to open log file: %s", filepath.c_str());
        return false;
    }
    
    // 格式化日志
    std::tm* tm = std::localtime(&record.log_time);
    char time_buf[64];
    std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm);
    
    it->second << "[" << time_buf << "] "
               << "[" << static_cast<int32_t>(record.type) << "] "
               << "[" << static_cast<int32_t>(record.level) << "] "
               << "[server:" << record.server_id << "] "
               << "[role:" << record.role_id << "] "
               << record.content;
    
    if (!record.extra_data.empty()) {
        it->second << " " << record.extra_data;
    }
    
    if (!record.ip.empty()) {
        it->second << " ip=" << record.ip;
    }
    
    it->second << std::endl;
    it->second.flush();
    
    return true;
}

bool LogServer::WriteToDatabase(const LogRecord& record) {
    // TODO: 写入数据库
    return true;
}

int64_t LogServer::GenerateLogId() {
    static int64_t next_id = time(nullptr) * 1000000;
    return next_id++;
}

std::string LogServer::GetLogFileName(LogType type, time_t log_time) {
    std::tm* tm = std::localtime(&log_time);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y%m%d", tm);
    
    std::string type_name;
    switch (type) {
        case LogType::LOGIN: type_name = "login"; break;
        case LogType::LOGOUT: type_name = "logout"; break;
        case LogType::CREATE_ROLE: type_name = "create_role"; break;
        case LogType::DELETE_ROLE: type_name = "delete_role"; break;
        case LogType::RECHARGE: type_name = "recharge"; break;
        case LogType::CONSUME: type_name = "consume"; break;
        case LogType::ITEM: type_name = "item"; break;
        case LogType::TRADE: type_name = "trade"; break;
        case LogType::BATTLE: type_name = "battle"; break;
        case LogType::TASK: type_name = "task"; break;
        case LogType::GUILD: type_name = "guild"; break;
        case LogType::CHAT: type_name = "chat"; break;
        case LogType::ERROR: type_name = "error"; break;
        case LogType::SYSTEM: type_name = "system"; break;
        default: type_name = "other"; break;
    }
    
    return type_name + "_" + buf + ".log";
}

} // namespace game_server
