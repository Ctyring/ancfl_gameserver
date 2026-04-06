#ifndef __LOG_SERVER_H__
#define __LOG_SERVER_H__

#include "common/game_service_base.h"
#include "common/message_dispatcher.h"
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

/**
 * @brief 日志类型枚举
 * 定义了系统支持的所有日志类型
 */
enum class LogType {
    LOGIN = 0,           // 玩家登录
    LOGOUT = 1,          // 玩家登出
    CREATE_ROLE = 2,     // 创建角色
    DELETE_ROLE = 3,     // 删除角色
    RECHARGE = 4,        // 充值
    CONSUME = 5,         // 消费
    ITEM = 6,            // 物品变更
    TRADE = 7,           // 交易
    BATTLE = 8,          // 战斗
    TASK = 9,            // 任务
    GUILD = 10,          // 公会
    CHAT = 11,           // 聊天
    ERROR = 12,          // 错误日志
    SYSTEM = 13          // 系统日志
};

/**
 * @brief 日志级别枚举
 * 定义了日志的严重程度级别
 */
enum class LogLevel {
    DEBUG = 0,   // 调试信息
    INFO = 1,    // 普通信息
    WARN = 2,    // 警告信息
    ERROR = 3,   // 错误信息
    FATAL = 4    // 致命错误
};

/**
 * @brief 日志记录结构体
 * 存储单条日志的完整信息
 */
struct LogRecord {
    int64_t log_id;               // 日志唯一ID
    LogType type;                 // 日志类型
    LogLevel level;               // 日志级别
    int32_t server_id;            // 服务器ID
    uint64_t role_id;             // 角色ID
    std::string role_name;        // 角色名称
    std::string content;          // 日志内容
    std::string extra_data;       // 扩展数据（JSON或自定义格式）
    time_t log_time;              // 日志时间戳
    std::string ip;               // 客户端IP地址
};

/**
 * @brief 日志查询条件结构体
 * 用于指定日志查询的过滤条件
 */
struct LogQueryCondition {
    LogType type;                 // 日志类型过滤
    int32_t server_id;            // 服务器ID过滤
    uint64_t role_id;             // 角色ID过滤
    time_t start_time;            // 开始时间
    time_t end_time;              // 结束时间
    int32_t limit;                // 返回记录数量限制
    int32_t offset;               // 查询偏移量（用于分页）
};

/**
 * @brief 日志服务器类
 * 继承自TcpService，提供网络通信和日志管理功能
 * 负责接收、存储、查询和归档游戏日志
 */
class LogServer : public GameServiceBase {
public:
    using ptr = std::shared_ptr<LogServer>;
    
    /**
     * @brief 构造函数
     * @param worker 工作线程IO管理器
     * @param accept_worker 接受连接的IO管理器
     */
    LogServer(ancfl::IOManager* worker = nullptr, 
              ancfl::IOManager* accept_worker = nullptr);
    
    /**
     * @brief 析构函数
     */
    virtual ~LogServer();
    
    /**
     * @brief 初始化日志服务器
     * @param ip 监听IP地址
     * @param port 监听端口
     * @return 初始化是否成功
     */
    bool Init(const std::string& ip, uint16_t port) override;
    
    /**
     * @brief 反初始化日志服务器
     */
    void Uninit() override;
    
    /**
     * @brief 停止日志服务器
     */
    void Stop() override;
    
    /**
     * @brief 同步写入单条日志
     * @param record 日志记录
     * @return 写入是否成功
     */
    bool WriteLog(const LogRecord& record);
    
    /**
     * @brief 异步写入单条日志（放入队列）
     * @param record 日志记录
     * @return 入队是否成功
     */
    bool WriteLogAsync(const LogRecord& record);
    
    /**
     * @brief 批量异步写入日志
     * @param records 日志记录列表
     * @return 入队是否成功
     */
    bool WriteLogBatch(const std::vector<LogRecord>& records);
    
    /**
     * @brief 根据条件查询日志
     * @param condition 查询条件
     * @param logs 输出的日志列表
     * @return 查询是否成功
     */
    bool QueryLogs(const LogQueryCondition& condition, std::vector<LogRecord>& logs);
    
    /**
     * @brief 根据日志ID查询单条日志
     * @param log_id 日志ID
     * @param record 输出的日志记录
     * @return 查询是否成功
     */
    bool QueryLogById(int64_t log_id, LogRecord& record);
    
    /**
     * @brief 获取指定时间范围内的日志数量
     * @param type 日志类型
     * @param start_time 开始时间
     * @param end_time 结束时间
     * @return 日志数量
     */
    int64_t GetLogCount(LogType type, time_t start_time, time_t end_time);
    
    /**
     * @brief 获取指定角色的日志数量
     * @param role_id 角色ID
     * @param type 日志类型
     * @return 日志数量
     */
    int64_t GetRoleLogCount(uint64_t role_id, LogType type);
    
    /**
     * @brief 清理指定天数之前的旧日志
     * @param days 保留天数
     * @return 清理是否成功
     */
    bool ClearOldLogs(int32_t days);
    
    /**
     * @brief 归档指定天数之前的日志
     * @param days 归档天数
     * @return 归档是否成功
     */
    bool ArchiveLogs(int32_t days);
    
    /**
     * @brief 记录登录/登出日志
     * @param role_id 角色ID
     * @param role_name 角色名称
     * @param server_id 服务器ID
     * @param ip 客户端IP
     * @param is_login true为登录，false为登出
     * @return 记录是否成功
     */
    bool LogLogin(uint64_t role_id, const std::string& role_name, 
                  int32_t server_id, const std::string& ip, bool is_login);
    
    /**
     * @brief 记录充值日志
     * @param role_id 角色ID
     * @param role_name 角色名称
     * @param server_id 服务器ID
     * @param amount 充值金额
     * @param product_id 产品ID
     * @return 记录是否成功
     */
    bool LogRecharge(uint64_t role_id, const std::string& role_name, 
                     int32_t server_id, int32_t amount, int32_t product_id);
    
    /**
     * @brief 记录消费日志
     * @param role_id 角色ID
     * @param role_name 角色名称
     * @param server_id 服务器ID
     * @param type 消费类型
     * @param amount 消费金额
     * @param reason 消费原因
     * @return 记录是否成功
     */
    bool LogConsume(uint64_t role_id, const std::string& role_name, 
                    int32_t server_id, int32_t type, int32_t amount, 
                    const std::string& reason);
    
    /**
     * @brief 记录物品变更日志
     * @param role_id 角色ID
     * @param role_name 角色名称
     * @param server_id 服务器ID
     * @param item_id 物品ID
     * @param count 物品数量
     * @param type 变更类型（增加/减少）
     * @param reason 变更原因
     * @return 记录是否成功
     */
    bool LogItem(uint64_t role_id, const std::string& role_name, 
                 int32_t server_id, int32_t item_id, int32_t count, 
                 int32_t type, const std::string& reason);
    
    /**
     * @brief 记录交易日志
     * @param role_id 角色ID
     * @param role_name 角色名称
     * @param server_id 服务器ID
     * @param target_id 交易对象ID
     * @param target_name 交易对象名称
     * @param items 交易物品信息
     * @return 记录是否成功
     */
    bool LogTrade(uint64_t role_id, const std::string& role_name, 
                  int32_t server_id, uint64_t target_id, 
                  const std::string& target_name, const std::string& items);
    
    /**
     * @brief 记录战斗日志
     * @param role_id 角色ID
     * @param role_name 角色名称
     * @param server_id 服务器ID
     * @param battle_type 战斗类型
     * @param result 战斗结果（胜利/失败）
     * @param duration 战斗时长（秒）
     * @return 记录是否成功
     */
    bool LogBattle(uint64_t role_id, const std::string& role_name, 
                   int32_t server_id, int32_t battle_type, 
                   int32_t result, int32_t duration);
    
    /**
     * @brief 定时器回调函数
     * 定期执行日志清理、归档等任务
     */
    void OnTimer();
    
protected:
    /**
     * @brief 注册网络消息处理器
     */
    void RegisterMessageHandlers();
    
    /**
     * @brief 处理单条日志写入请求
     * @param packet 网络数据包
     * @return 处理是否成功
     */
    bool HandleSendLogReq(const NetPacket& packet);
    
    /**
     * @brief 处理批量日志写入请求
     * @param packet 网络数据包
     * @return 处理是否成功
     */
    bool HandleSendLogBatchReq(const NetPacket& packet);
    
    /**
     * @brief 处理日志查询请求
     * @param packet 网络数据包
     * @return 处理是否成功
     */
    bool HandleQueryLogReq(const NetPacket& packet);
    
    /**
     * @brief 处理日志计数请求
     * @param packet 网络数据包
     * @return 处理是否成功
     */
    bool HandleCountLogReq(const NetPacket& packet);
    
    /**
     * @brief 处理清理旧日志请求
     * @param packet 网络数据包
     * @return 处理是否成功
     */
    bool HandleClearOldLogReq(const NetPacket& packet);
    
    /**
     * @brief 处理登录日志请求
     * @param packet 网络数据包
     * @return 处理是否成功
     */
    bool HandleLogLoginReq(const NetPacket& packet);
    
    /**
     * @brief 处理充值日志请求
     * @param packet 网络数据包
     * @return 处理是否成功
     */
    bool HandleLogRechargeReq(const NetPacket& packet);
    
    /**
     * @brief 处理消费日志请求
     * @param packet 网络数据包
     * @return 处理是否成功
     */
    bool HandleLogConsumeReq(const NetPacket& packet);
    
    /**
     * @brief 处理物品日志请求
     * @param packet 网络数据包
     * @return 处理是否成功
     */
    bool HandleLogItemReq(const NetPacket& packet);
    
    /**
     * @brief 处理战斗日志请求
     * @param packet 网络数据包
     * @return 处理是否成功
     */
    bool HandleLogBattleReq(const NetPacket& packet);
    
private:
    /**
     * @brief 工作线程函数，处理日志队列
     * 从队列中取出日志并批量写入文件/数据库
     */
    void ProcessLogQueue();
    
    /**
     * @brief 将单条日志写入文件
     * @param record 日志记录
     * @return 写入是否成功
     */
    bool WriteToFile(const LogRecord& record);
    
    /**
     * @brief 将单条日志写入数据库
     * @param record 日志记录
     * @return 写入是否成功
     */
    bool WriteToDatabase(const LogRecord& record);
    
    /**
     * @brief 生成唯一日志ID
     * 使用时间戳+计数器的方式生成
     * @return 日志ID
     */
    int64_t GenerateLogId();
    
    /**
     * @brief 根据日志类型和时间生成日志文件名
     * @param type 日志类型
     * @param log_time 日志时间
     * @return 日志文件名（例如：login_20260329.log）
     */
    std::string GetLogFileName(LogType type, time_t log_time);
    
    std::queue<LogRecord> log_queue_;        // 日志队列
    std::mutex queue_mutex_;                   // 队列互斥锁
    std::condition_variable queue_cv_;         // 队列条件变量
    
    std::thread worker_thread_;                // 日志处理工作线程
    bool stop_flag_;                            // 停止标志
    
    std::unordered_map<std::string, std::ofstream> file_handles_;  // 文件句柄缓存
    std::mutex file_mutex_;                     // 文件操作互斥锁
    
    bool is_running_;                           // 运行状态标志
    std::string log_dir_;                       // 日志存储目录
    int32_t log_retention_days_;                // 日志保留天数
    
    time_t last_clear_time_;                    // 上次清理时间
};

} // namespace game_server

#endif // __LOG_SERVER_H__
