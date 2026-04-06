#include "log_server.h"
#include "proto/msg_id.pb.h"
#include "proto/msg_log.pb.h"
#include "ancfl/log.h"
#include <sys/stat.h>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace game_server {

/**
 * @brief 构造函数
 * 初始化成员变量，设置默认参数
 * @param worker 工作线程IO管理器
 * @param accept_worker 接受连接的IO管理器
 */
LogServer::LogServer(ancfl::IOManager* worker, ancfl::IOManager* accept_worker)
    : GameServiceBase("log_server")
    , stop_flag_(false)
    , is_running_(false)
    , log_dir_("logs")
    , log_retention_days_(30)
    , last_clear_time_(0) {
}

/**
 * @brief 析构函数
 * 调用Stop()确保资源正确释放
 */
LogServer::~LogServer() {
    Stop();
}

/**
 * @brief 初始化日志服务器
 * 1. 调用父类Init初始化TCP服务
 * 2. 创建日志目录
 * 3. 注册消息处理器
 * @param ip 监听IP地址
 * @param port 监听端口
 * @return 初始化是否成功
 */
bool LogServer::Init(const std::string& ip, uint16_t port) {
    auto addr = ancfl::Address::LookupAnyIPAddress(ip + ":" + std::to_string(port));
    if (!bind(addr)) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Log server bind failed: " << ip << ":" << port;
        return false;
    }
    
    if (!start()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Log server start failed: " << ip << ":" << port;
        return false;
    }
    
    mkdir(log_dir_.c_str(), 0755);
    
    RegisterMessageHandlers();
    
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Log server initialized: " << ip << ":" << port;
    return true;
}

/**
 * @brief 反初始化日志服务器
 * 先停止服务，再调用父类Uninit
 */
void LogServer::Uninit() {
    Stop();
}

/**
 * @brief 停止日志服务器
 * 1. 设置停止标志
 * 2. 唤醒工作线程
 * 3. 等待工作线程结束
 * 4. 关闭所有文件句柄
 * 5. 调用父类Stop
 */
void LogServer::Stop() {
    if (!is_running_) {
        return;
    }
    
    stop_flag_ = true;
    queue_cv_.notify_all();
    
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    
    {
        std::lock_guard<std::mutex> lock(file_mutex_);
        for (auto& pair : file_handles_) {
            if (pair.second.is_open()) {
                pair.second.close();
            }
        }
        file_handles_.clear();
    }
    
    is_running_ = false;
    ancfl::TcpServer::Stop();
    
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Log server stopped";
}

/**
 * @brief 注册网络消息处理器
 * 将各种日志相关的消息ID绑定到对应的处理函数
 */
void LogServer::RegisterMessageHandlers() {
    dispatcher_->RegisterHandler(MSG_WRITE_LOG_REQ, 
        std::bind(&LogServer::HandleSendLogReq, this, std::placeholders::_1));
    
    dispatcher_->RegisterHandler(MSG_WRITE_LOG_BATCH_REQ, 
        std::bind(&LogServer::HandleSendLogBatchReq, this, std::placeholders::_1));
    
    dispatcher_->RegisterHandler(MSG_QUERY_LOG_REQ, 
        std::bind(&LogServer::HandleQueryLogReq, this, std::placeholders::_1));
    
    dispatcher_->RegisterHandler(MSG_COUNT_LOG_REQ, 
        std::bind(&LogServer::HandleCountLogReq, this, std::placeholders::_1));
    
    dispatcher_->RegisterHandler(MSG_CLEAR_OLD_LOG_REQ, 
        std::bind(&LogServer::HandleClearOldLogReq, this, std::placeholders::_1));
    
    dispatcher_->RegisterHandler(MSG_LOG_LOGIN_REQ, 
        std::bind(&LogServer::HandleLogLoginReq, this, std::placeholders::_1));
    
    dispatcher_->RegisterHandler(MSG_LOG_RECHARGE_REQ, 
        std::bind(&LogServer::HandleLogRechargeReq, this, std::placeholders::_1));
    
    dispatcher_->RegisterHandler(MSG_LOG_CONSUME_REQ, 
        std::bind(&LogServer::HandleLogConsumeReq, this, std::placeholders::_1));
    
    dispatcher_->RegisterHandler(MSG_LOG_ITEM_REQ, 
        std::bind(&LogServer::HandleLogItemReq, this, std::placeholders::_1));
    
    dispatcher_->RegisterHandler(MSG_LOG_BATTLE_REQ, 
        std::bind(&LogServer::HandleLogBattleReq, this, std::placeholders::_1));
}

/**
 * @brief 处理单条日志写入请求
 * 1. 解析Proto消息
 * 2. 转换为LogRecord结构
 * 3. 异步写入日志队列
 * 4. 发送响应
 * @param packet 网络数据包
 * @return 处理是否成功
 */
bool LogServer::HandleSendLogReq(const NetPacket& packet) {
    auto msg = std::dynamic_pointer_cast<msg_log::SendLogReq>(packet.msg);
    if (!msg) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Parse SendLogReq failed";
        return false;
    }
    
    const auto& req = *msg;
    
    LogRecord record;
    record.log_id = req.record().log_id();
    record.type = static_cast<LogType>(req.record().type());
    record.level = static_cast<LogLevel>(req.record().level());
    record.server_id = req.record().server_id();
    record.role_id = req.record().role_id();
    record.role_name = req.record().role_name();
    record.content = req.record().content();
    record.extra_data = req.record().extra_data();
    record.log_time = req.record().log_time();
    record.ip = req.record().ip();
    
    bool success = WriteLogAsync(record);
    
    msg_log::SendLogAck ack;
    ack.set_result(success ? 0 : 1);
    ack.set_log_id(record.log_id);
    
    std::string ack_data;
    ack.SerializeToString(&ack_data);
    SendMsgToServer(packet.conn_id, MSG_WRITE_LOG_ACK, ack_data);
    
    return true;
}

/**
 * @brief 处理批量日志写入请求
 * 1. 解析Proto消息
 * 2. 转换为LogRecord列表
 * 3. 批量异步写入日志队列
 * 4. 发送响应
 * @param packet 网络数据包
 * @return 处理是否成功
 */
bool LogServer::HandleSendLogBatchReq(const NetPacket& packet) {
    auto msg = std::dynamic_pointer_cast<msg_log::SendLogBatchReq>(packet.msg);
    if (!msg) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Parse SendLogBatchReq failed";
        return false;
    }
    
    const auto& req = *msg;
    
    std::vector<LogRecord> records;
    for (const auto& msg_record : req.records()) {
        LogRecord record;
        record.log_id = msg_record.log_id();
        record.type = static_cast<LogType>(msg_record.type());
        record.level = static_cast<LogLevel>(msg_record.level());
        record.server_id = msg_record.server_id();
        record.role_id = msg_record.role_id();
        record.role_name = msg_record.role_name();
        record.content = msg_record.content();
        record.extra_data = msg_record.extra_data();
        record.log_time = msg_record.log_time();
        record.ip = msg_record.ip();
        records.push_back(record);
    }
    
    bool success = WriteLogBatch(records);
    
    msg_log::SendLogBatchAck ack;
    ack.set_result(success ? 0 : 1);
    ack.set_success_count(success ? records.size() : 0);
    ack.set_failed_count(success ? 0 : records.size());
    
    std::string ack_data;
    ack.SerializeToString(&ack_data);
    SendMsgToServer(packet.conn_id, MSG_WRITE_LOG_BATCH_ACK, ack_data);
    
    return true;
}

/**
 * @brief 处理日志查询请求
 * 1. 解析Proto消息
 * 2. 转换为查询条件
 * 3. 执行查询
 * 4. 发送响应
 * @param packet 网络数据包
 * @return 处理是否成功
 */
bool LogServer::HandleQueryLogReq(const NetPacket& packet) {
    auto msg = std::dynamic_pointer_cast<msg_log::QueryLogReq>(packet.msg);
    if (!msg) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Parse QueryLogReq failed";
        return false;
    }
    
    const auto& req = *msg;
    
    LogQueryCondition condition;
    condition.type = static_cast<LogType>(req.type());
    condition.server_id = req.server_id();
    condition.role_id = req.role_id();
    condition.start_time = req.start_time();
    condition.end_time = req.end_time();
    condition.limit = req.limit();
    condition.offset = req.offset();
    
    std::vector<LogRecord> logs;
    bool success = QueryLogs(condition, logs);
    
    msg_log::QueryLogAck ack;
    ack.set_result(success ? 0 : 1);
    
    for (const auto& record : logs) {
        auto* msg_record = ack.add_records();
        msg_record->set_log_id(record.log_id);
        msg_record->set_type(static_cast<msg_log::LogType>(record.type));
        msg_record->set_level(static_cast<msg_log::LogLevel>(record.level));
        msg_record->set_server_id(record.server_id);
        msg_record->set_role_id(record.role_id);
        msg_record->set_role_name(record.role_name);
        msg_record->set_content(record.content);
        msg_record->set_extra_data(record.extra_data);
        msg_record->set_log_time(record.log_time);
        msg_record->set_ip(record.ip);
    }
    
    std::string ack_data;
    ack.SerializeToString(&ack_data);
    SendMsgToServer(packet.conn_id, MSG_QUERY_LOG_ACK, ack_data);
    
    return true;
}

/**
 * @brief 处理日志计数请求
 * 1. 解析Proto消息
 * 2. 调用计数函数
 * 3. 发送响应
 * @param packet 网络数据包
 * @return 处理是否成功
 */
bool LogServer::HandleCountLogReq(const NetPacket& packet) {
    auto msg = std::dynamic_pointer_cast<msg_log::CountLogReq>(packet.msg);
    if (!msg) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Parse CountLogReq failed";
        return false;
    }
    
    const auto& req = *msg;
    
    int64_t count = GetLogCount(
        static_cast<LogType>(req.type()),
        req.start_time(),
        req.end_time()
    );
    
    msg_log::CountLogAck ack;
    ack.set_result(0);
    ack.set_count(count);
    
    std::string ack_data;
    ack.SerializeToString(&ack_data);
    SendMsgToServer(packet.conn_id, MSG_COUNT_LOG_ACK, ack_data);
    
    return true;
}

/**
 * @brief 处理清理旧日志请求
 * 1. 解析Proto消息
 * 2. 调用清理函数
 * 3. 发送响应
 * @param packet 网络数据包
 * @return 处理是否成功
 */
bool LogServer::HandleClearOldLogReq(const NetPacket& packet) {
    auto msg = std::dynamic_pointer_cast<msg_log::CountLogReq>(packet.msg);
    if (!msg) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Parse ClearOldLogReq failed";
        return false;
    }
    
    const auto& req = *msg;
    
    int32_t days = static_cast<int32_t>(req.start_time());
    bool success = ClearOldLogs(days);
    
    msg_log::CountLogAck ack;
    ack.set_result(success ? 0 : 1);
    ack.set_count(0);
    
    std::string ack_data;
    ack.SerializeToString(&ack_data);
    SendMsgToServer(packet.conn_id, MSG_CLEAR_OLD_LOG_ACK, ack_data);
    
    return true;
}

/**
 * @brief 处理登录日志请求
 * 1. 解析Proto消息
 * 2. 调用LogLogin记录日志
 * 3. 发送响应
 * @param packet 网络数据包
 * @return 处理是否成功
 */
bool LogServer::HandleLogLoginReq(const NetPacket& packet) {
    auto msg = std::dynamic_pointer_cast<msg_log::LogRecord>(packet.msg);
    if (!msg) {
        return false;
    }
    
    const auto& req = *msg;
    
    bool is_login = (req.type() == msg_log::LogType::LOG_TYPE_LOGIN);
    bool success = LogLogin(
        req.role_id(),
        req.role_name(),
        req.server_id(),
        req.ip(),
        is_login
    );
    
    msg_log::SendLogAck ack;
    ack.set_result(success ? 0 : 1);
    ack.set_log_id(req.log_id());
    
    std::string ack_data;
    ack.SerializeToString(&ack_data);
    SendMsgToServer(packet.conn_id, MSG_LOG_LOGIN_ACK, ack_data);
    
    return true;
}

/**
 * @brief 处理充值日志请求
 * 1. 解析Proto消息
 * 2. 从extra_data中解析充值信息
 * 3. 调用LogRecharge记录日志
 * 4. 发送响应
 * @param packet 网络数据包
 * @return 处理是否成功
 */
bool LogServer::HandleLogRechargeReq(const NetPacket& packet) {
    auto msg = std::dynamic_pointer_cast<msg_log::LogRecord>(packet.msg);
    if (!msg) {
        return false;
    }
    
    const auto& req = *msg;
    
    int32_t amount = 0;
    int32_t product_id = 0;
    if (!req.extra_data().empty()) {
        sscanf(req.extra_data().c_str(), "amount=%d,product_id=%d", &amount, &product_id);
    }
    
    bool success = LogRecharge(
        req.role_id(),
        req.role_name(),
        req.server_id(),
        amount,
        product_id
    );
    
    msg_log::SendLogAck ack;
    ack.set_result(success ? 0 : 1);
    ack.set_log_id(req.log_id());
    
    std::string ack_data;
    ack.SerializeToString(&ack_data);
    SendMsgToServer(packet.conn_id, MSG_LOG_RECHARGE_ACK, ack_data);
    
    return true;
}

/**
 * @brief 处理消费日志请求
 * 1. 解析Proto消息
 * 2. 从extra_data中解析消费信息
 * 3. 调用LogConsume记录日志
 * 4. 发送响应
 * @param packet 网络数据包
 * @return 处理是否成功
 */
bool LogServer::HandleLogConsumeReq(const NetPacket& packet) {
    auto msg = std::dynamic_pointer_cast<msg_log::LogRecord>(packet.msg);
    if (!msg) {
        return false;
    }
    
    const auto& req = *msg;
    
    int32_t type = 0;
    int32_t amount = 0;
    char reason[256] = {0};
    if (!req.extra_data().empty()) {
        sscanf(req.extra_data().c_str(), "type=%d,amount=%d,reason=%s", &type, &amount, reason);
    }
    
    bool success = LogConsume(
        req.role_id(),
        req.role_name(),
        req.server_id(),
        type,
        amount,
        std::string(reason)
    );
    
    msg_log::SendLogAck ack;
    ack.set_result(success ? 0 : 1);
    ack.set_log_id(req.log_id());
    
    std::string ack_data;
    ack.SerializeToString(&ack_data);
    SendMsgToServer(packet.conn_id, MSG_LOG_CONSUME_ACK, ack_data);
    
    return true;
}

/**
 * @brief 处理物品日志请求
 * 1. 解析Proto消息
 * 2. 从extra_data中解析物品信息
 * 3. 调用LogItem记录日志
 * 4. 发送响应
 * @param packet 网络数据包
 * @return 处理是否成功
 */
bool LogServer::HandleLogItemReq(const NetPacket& packet) {
    auto msg = std::dynamic_pointer_cast<msg_log::LogRecord>(packet.msg);
    if (!msg) {
        return false;
    }
    
    const auto& req = *msg;
    
    int32_t item_id = 0;
    int32_t count = 0;
    int32_t type = 0;
    char reason[256] = {0};
    if (!req.extra_data().empty()) {
        sscanf(req.extra_data().c_str(), "item_id=%d,count=%d,type=%d,reason=%s", 
               &item_id, &count, &type, reason);
    }
    
    bool success = LogItem(
        req.role_id(),
        req.role_name(),
        req.server_id(),
        item_id,
        count,
        type,
        std::string(reason)
    );
    
    msg_log::SendLogAck ack;
    ack.set_result(success ? 0 : 1);
    ack.set_log_id(req.log_id());
    
    std::string ack_data;
    ack.SerializeToString(&ack_data);
    SendMsgToServer(packet.conn_id, MSG_LOG_ITEM_ACK, ack_data);
    
    return true;
}

/**
 * @brief 处理战斗日志请求
 * 1. 解析Proto消息
 * 2. 从extra_data中解析战斗信息
 * 3. 调用LogBattle记录日志
 * 4. 发送响应
 * @param packet 网络数据包
 * @return 处理是否成功
 */
bool LogServer::HandleLogBattleReq(const NetPacket& packet) {
    auto msg = std::dynamic_pointer_cast<msg_log::LogRecord>(packet.msg);
    if (!msg) {
        return false;
    }
    
    const auto& req = *msg;
    
    int32_t battle_type = 0;
    int32_t result = 0;
    int32_t duration = 0;
    if (!req.extra_data().empty()) {
        sscanf(req.extra_data().c_str(), "battle_type=%d,result=%d,duration=%d", 
               &battle_type, &result, &duration);
    }
    
    bool success = LogBattle(
        req.role_id(),
        req.role_name(),
        req.server_id(),
        battle_type,
        result,
        duration
    );
    
    msg_log::SendLogAck ack;
    ack.set_result(success ? 0 : 1);
    ack.set_log_id(req.log_id());
    
    std::string ack_data;
    ack.SerializeToString(&ack_data);
    SendMsgToServer(packet.conn_id, MSG_LOG_BATTLE_ACK, ack_data);
    
    return true;
}

/**
 * @brief 同步写入单条日志
 * 1. 写入文件
 * 2. 写入数据库（可选，失败不影响主流程）
 * @param record 日志记录
 * @return 写入是否成功
 */
bool LogServer::WriteLog(const LogRecord& record) {
    if (!WriteToFile(record)) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Write log to file failed";
        return false;
    }
    
    if (!WriteToDatabase(record)) {
        ANCFL_LOG_WARN(ANCFL_LOG_ROOT()) << "Write log to database failed";
    }
    
    return true;
}

/**
 * @brief 异步写入单条日志
 * 将日志放入队列，由工作线程处理
 * @param record 日志记录
 * @return 入队是否成功
 */
bool LogServer::WriteLogAsync(const LogRecord& record) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    log_queue_.push(record);
    queue_cv_.notify_one();
    return true;
}

/**
 * @brief 批量异步写入日志
 * 依次将多条日志放入队列
 * @param records 日志记录列表
 * @return 入队是否成功
 */
bool LogServer::WriteLogBatch(const std::vector<LogRecord>& records) {
    for (const auto& record : records) {
        WriteLogAsync(record);
    }
    return true;
}

/**
 * @brief 工作线程函数，处理日志队列
 * 1. 等待队列有数据或停止信号
 * 2. 批量取出日志（最多100条）
 * 3. 依次写入文件/数据库
 */
void LogServer::ProcessLogQueue() {
    while (!stop_flag_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        queue_cv_.wait(lock, [this] { 
            return !log_queue_.empty() || stop_flag_; 
        });

        if (stop_flag_ && log_queue_.empty()) {
            break;
        }

        std::vector<LogRecord> records;
        while (!log_queue_.empty() && records.size() < 100) {
            records.push_back(log_queue_.front());
            log_queue_.pop();
        }

        lock.unlock();

        for (const auto& record : records) {
            WriteLog(record);
        }
    }
}

/**
 * @brief 将单条日志写入文件
 * 1. 根据日志类型和时间生成文件名
 * 2. 从缓存中获取或打开文件句柄
 * 3. 格式化日志内容并写入
 * 4. 刷新缓冲区
 * @param record 日志记录
 * @return 写入是否成功
 */
bool LogServer::WriteToFile(const LogRecord& record) {
    std::string filename = GetLogFileName(record.type, record.log_time);
    std::string filepath = log_dir_ + "/" + filename;
    
    std::lock_guard<std::mutex> lock(file_mutex_);
    
    auto it = file_handles_.find(filepath);
    if (it == file_handles_.end()) {
        file_handles_[filepath].open(filepath, std::ios::app);
        it = file_handles_.find(filepath);
    }
    
    if (!it->second.is_open()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Open log file failed: " << filepath;
        return false;
    }
    
    char time_buf[64];
    struct tm* tm_info = localtime(&record.log_time);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
    
    it->second << "[" << time_buf << "] "
               << "[" << static_cast<int>(record.type) << "] "
               << "[" << static_cast<int>(record.level) << "] "
               << "[server:" << record.server_id << "] "
               << "[role:" << record.role_id << "] "
               << record.content;
    
    if (!record.extra_data.empty()) {
        it->second << " " << record.extra_data;
    }
    
    it->second << std::endl;
    it->second.flush();
    
    return true;
}

/**
 * @brief 将单条日志写入数据库
 * 预留接口，当前仅返回true
 * @param record 日志记录
 * @return 写入是否成功
 */
bool LogServer::WriteToDatabase(const LogRecord& record) {
    return true;
}

/**
 * @brief 生成唯一日志ID
 * 使用时间戳+计数器的方式：
 * - 基时间：1700000000（2023-11-14左右）
 * - 格式：(当前时间 - 基时间) * 1000000 + 计数器
 * @return 日志ID
 */
int64_t LogServer::GenerateLogId() {
    static int64_t base_time = 1700000000;
    static int64_t counter = 0;
    return (time(nullptr) - base_time) * 1000000 + (++counter);
}

/**
 * @brief 根据日志类型和时间生成日志文件名
 * 格式：{type_name}_{YYYYMMDD}.log
 * 例如：login_20260329.log
 * @param type 日志类型
 * @param log_time 日志时间
 * @return 日志文件名
 */
std::string LogServer::GetLogFileName(LogType type, time_t log_time) {
    const char* type_names[] = {
        "login", "logout", "create_role", "delete_role",
        "recharge", "consume", "item", "trade",
        "battle", "task", "guild", "chat",
        "error", "system"
    };
    
    char date_buf[16];
    struct tm* tm_info = localtime(&log_time);
    strftime(date_buf, sizeof(date_buf), "%Y%m%d", tm_info);
    
    int type_idx = static_cast<int>(type);
    if (type_idx < 0 || type_idx >= 14) {
        type_idx = 13;
    }
    
    return std::string(type_names[type_idx]) + "_" + date_buf + ".log";
}

/**
 * @brief 根据条件查询日志
 * 预留接口，当前仅返回true
 * @param condition 查询条件
 * @param logs 输出的日志列表
 * @return 查询是否成功
 */
bool LogServer::QueryLogs(const LogQueryCondition& condition, std::vector<LogRecord>& logs) {
    return true;
}

/**
 * @brief 根据日志ID查询单条日志
 * 预留接口，当前仅返回true
 * @param log_id 日志ID
 * @param record 输出的日志记录
 * @return 查询是否成功
 */
bool LogServer::QueryLogById(int64_t log_id, LogRecord& record) {
    return true;
}

/**
 * @brief 获取指定时间范围内的日志数量
 * 预留接口，当前返回0
 * @param type 日志类型
 * @param start_time 开始时间
 * @param end_time 结束时间
 * @return 日志数量
 */
int64_t LogServer::GetLogCount(LogType type, time_t start_time, time_t end_time) {
    return 0;
}

/**
 * @brief 获取指定角色的日志数量
 * 预留接口，当前返回0
 * @param role_id 角色ID
 * @param type 日志类型
 * @return 日志数量
 */
int64_t LogServer::GetRoleLogCount(uint64_t role_id, LogType type) {
    return 0;
}

/**
 * @brief 清理指定天数之前的旧日志
 * 预留接口，当前仅记录日志并返回true
 * @param days 保留天数
 * @return 清理是否成功
 */
bool LogServer::ClearOldLogs(int32_t days) {
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Clearing old logs: days=" << days;
    return true;
}

/**
 * @brief 归档指定天数之前的日志
 * 预留接口，当前仅记录日志并返回true
 * @param days 归档天数
 * @return 归档是否成功
 */
bool LogServer::ArchiveLogs(int32_t days) {
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Archiving logs: days=" << days;
    return true;
}

/**
 * @brief 记录登录/登出日志
 * 1. 生成日志ID
 * 2. 填充日志记录
 * 3. 异步写入队列
 * @param role_id 角色ID
 * @param role_name 角色名称
 * @param server_id 服务器ID
 * @param ip 客户端IP
 * @param is_login true为登录，false为登出
 * @return 记录是否成功
 */
bool LogServer::LogLogin(uint64_t role_id, const std::string& role_name, 
                         int32_t server_id, const std::string& ip, bool is_login) {
    LogRecord record;
    record.log_id = GenerateLogId();
    record.type = is_login ? LogType::LOGIN : LogType::LOGOUT;
    record.level = LogLevel::INFO;
    record.server_id = server_id;
    record.role_id = role_id;
    record.role_name = role_name;
    record.content = is_login ? "Player login" : "Player logout";
    record.extra_data = "ip=" + ip;
    record.log_time = time(nullptr);
    record.ip = ip;
    
    return WriteLogAsync(record);
}

/**
 * @brief 记录充值日志
 * 1. 生成日志ID
 * 2. 填充日志记录，格式化extra_data
 * 3. 异步写入队列
 * @param role_id 角色ID
 * @param role_name 角色名称
 * @param server_id 服务器ID
 * @param amount 充值金额
 * @param product_id 产品ID
 * @return 记录是否成功
 */
bool LogServer::LogRecharge(uint64_t role_id, const std::string& role_name, 
                            int32_t server_id, int32_t amount, int32_t product_id) {
    LogRecord record;
    record.log_id = GenerateLogId();
    record.type = LogType::RECHARGE;
    record.level = LogLevel::INFO;
    record.server_id = server_id;
    record.role_id = role_id;
    record.role_name = role_name;
    record.content = "Player recharge";
    char extra[256];
    snprintf(extra, sizeof(extra), "amount=%d,product_id=%d", amount, product_id);
    record.extra_data = extra;
    record.log_time = time(nullptr);
    
    return WriteLogAsync(record);
}

/**
 * @brief 记录消费日志
 * 1. 生成日志ID
 * 2. 填充日志记录，格式化extra_data
 * 3. 异步写入队列
 * @param role_id 角色ID
 * @param role_name 角色名称
 * @param server_id 服务器ID
 * @param type 消费类型
 * @param amount 消费金额
 * @param reason 消费原因
 * @return 记录是否成功
 */
bool LogServer::LogConsume(uint64_t role_id, const std::string& role_name, 
                           int32_t server_id, int32_t type, int32_t amount, 
                           const std::string& reason) {
    LogRecord record;
    record.log_id = GenerateLogId();
    record.type = LogType::CONSUME;
    record.level = LogLevel::INFO;
    record.server_id = server_id;
    record.role_id = role_id;
    record.role_name = role_name;
    record.content = "Player consume";
    char extra[256];
    snprintf(extra, sizeof(extra), "type=%d,amount=%d,reason=%s", type, amount, reason.c_str());
    record.extra_data = extra;
    record.log_time = time(nullptr);
    
    return WriteLogAsync(record);
}

/**
 * @brief 记录物品变更日志
 * 1. 生成日志ID
 * 2. 填充日志记录，格式化extra_data
 * 3. 异步写入队列
 * @param role_id 角色ID
 * @param role_name 角色名称
 * @param server_id 服务器ID
 * @param item_id 物品ID
 * @param count 物品数量
 * @param type 变更类型（增加/减少）
 * @param reason 变更原因
 * @return 记录是否成功
 */
bool LogServer::LogItem(uint64_t role_id, const std::string& role_name, 
                        int32_t server_id, int32_t item_id, int32_t count, 
                        int32_t type, const std::string& reason) {
    LogRecord record;
    record.log_id = GenerateLogId();
    record.type = LogType::ITEM;
    record.level = LogLevel::INFO;
    record.server_id = server_id;
    record.role_id = role_id;
    record.role_name = role_name;
    record.content = "Item change";
    char extra[256];
    snprintf(extra, sizeof(extra), "item_id=%d,count=%d,type=%d,reason=%s", 
             item_id, count, type, reason.c_str());
    record.extra_data = extra;
    record.log_time = time(nullptr);
    
    return WriteLogAsync(record);
}

/**
 * @brief 记录交易日志
 * 1. 生成日志ID
 * 2. 填充日志记录，格式化extra_data
 * 3. 异步写入队列
 * @param role_id 角色ID
 * @param role_name 角色名称
 * @param server_id 服务器ID
 * @param target_id 交易对象ID
 * @param target_name 交易对象名称
 * @param items 交易物品信息
 * @return 记录是否成功
 */
bool LogServer::LogTrade(uint64_t role_id, const std::string& role_name, 
                         int32_t server_id, uint64_t target_id, 
                         const std::string& target_name, const std::string& items) {
    LogRecord record;
    record.log_id = GenerateLogId();
    record.type = LogType::TRADE;
    record.level = LogLevel::INFO;
    record.server_id = server_id;
    record.role_id = role_id;
    record.role_name = role_name;
    record.content = "Player trade";
    char extra[512];
    snprintf(extra, sizeof(extra), "target_id=%lu,target_name=%s,items=%s", 
             target_id, target_name.c_str(), items.c_str());
    record.extra_data = extra;
    record.log_time = time(nullptr);
    
    return WriteLogAsync(record);
}

/**
 * @brief 记录战斗日志
 * 1. 生成日志ID
 * 2. 填充日志记录，格式化extra_data
 * 3. 异步写入队列
 * @param role_id 角色ID
 * @param role_name 角色名称
 * @param server_id 服务器ID
 * @param battle_type 战斗类型
 * @param result 战斗结果（胜利/失败）
 * @param duration 战斗时长（秒）
 * @return 记录是否成功
 */
bool LogServer::LogBattle(uint64_t role_id, const std::string& role_name, 
                          int32_t server_id, int32_t battle_type, 
                          int32_t result, int32_t duration) {
    LogRecord record;
    record.log_id = GenerateLogId();
    record.type = LogType::BATTLE;
    record.level = LogLevel::INFO;
    record.server_id = server_id;
    record.role_id = role_id;
    record.role_name = role_name;
    record.content = "Battle result";
    char extra[256];
    snprintf(extra, sizeof(extra), "battle_type=%d,result=%d,duration=%d", 
             battle_type, result, duration);
    record.extra_data = extra;
    record.log_time = time(nullptr);
    
    return WriteLogAsync(record);
}

/**
 * @brief 定时器回调函数
 * 定期执行日志清理任务（每天一次）
 */
void LogServer::OnTimer() {
    time_t now = time(nullptr);
    
    if (now - last_clear_time_ > 86400) {
        ClearOldLogs(log_retention_days_);
        last_clear_time_ = now;
    }
}

} // namespace game_server
