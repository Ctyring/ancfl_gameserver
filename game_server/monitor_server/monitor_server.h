#ifndef __MONITOR_SERVER_H__
#define __MONITOR_SERVER_H__

#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <memory>
#include <functional>

namespace game_server {

// 服务器状态
enum class MonitorServerStatus {
    UNKNOWN = 0,
    ONLINE = 1,
    OFFLINE = 2,
    BUSY = 3,
    ERROR = 4
};

// 服务器性能数据
struct ServerPerfData {
    int32_t server_id;
    std::string server_name;
    int32_t server_type;
    MonitorServerStatus status;
    int32_t online_count;
    int32_t max_online;
    double cpu_usage;
    double memory_usage;
    int32_t network_in;
    int32_t network_out;
    int32_t message_queue_size;
    int32_t db_query_count;
    int32_t db_query_time;
    time_t update_time;
};

// 告警类型
enum class AlertType {
    CPU_HIGH = 1,
    MEMORY_HIGH = 2,
    NETWORK_HIGH = 3,
    ONLINE_HIGH = 4,
    SERVER_DOWN = 5,
    DB_SLOW = 6
};

// 告警信息
struct AlertInfo {
    int32_t alert_id;
    AlertType type;
    int32_t server_id;
    std::string message;
    time_t alert_time;
    bool is_handled;
    std::string handler;
    time_t handle_time;
};

// 控制命令
enum class ControlCommand {
    START = 1,
    STOP = 2,
    RESTART = 3,
    RELOAD = 4,
    KICK_ALL = 5,
    BROADCAST = 6,
    MAINTENANCE = 7
};

// 控制命令结果
struct CommandResult {
    int32_t command_id;
    ControlCommand command;
    int32_t server_id;
    bool success;
    std::string message;
    time_t execute_time;
};

// 监控服务器类
class MonitorServer {
public:
    MonitorServer();
    ~MonitorServer();
    
    // 初始化和启动
    bool Init(const std::string& config_file);
    bool Start();
    void Stop();
    bool IsRunning();
    
    // 服务器状态管理
    bool UpdateServerStatus(const ServerPerfData& data);
    bool GetServerStatus(int32_t server_id, ServerPerfData& data);
    bool GetAllServerStatus(std::vector<ServerPerfData>& servers);
    
    // 告警管理
    bool AddAlert(const AlertInfo& alert);
    bool GetAlerts(std::vector<AlertInfo>& alerts, bool include_handled = false);
    bool HandleAlert(int32_t alert_id, const std::string& handler);
    bool ClearAlerts();
    
    // 控制命令
    bool SendCommand(int32_t server_id, ControlCommand command, const std::string& params, int32_t& command_id);
    bool GetCommandResult(int32_t command_id, CommandResult& result);
    
    // WEB接口
    bool HandleWebRequest(const std::string& path, const std::string& method, const std::string& body, std::string& response);
    
    // 性能统计
    bool GetOnlineTrend(int32_t server_id, std::vector<std::pair<time_t, int32_t>>& trend);
    bool GetPerformanceHistory(int32_t server_id, std::vector<ServerPerfData>& history);
    
    // 定时处理
    void OnTimer();
    
private:
    // 检查告警
    void CheckAlerts();
    
    // 检查服务器状态
    void CheckServerStatus();
    
    // 生成命令ID
    int32_t GenerateCommandId();
    
    // 生成告警ID
    int32_t GenerateAlertId();
    
    // 服务器状态缓存
    std::unordered_map<int32_t, ServerPerfData> server_status_;
    
    // 告警列表
    std::vector<AlertInfo> alerts_;
    
    // 命令结果缓存
    std::unordered_map<int32_t, CommandResult> command_results_;
    
    // 性能历史数据
    std::unordered_map<int32_t, std::vector<ServerPerfData>> perf_history_;
    
    // 互斥锁
    std::mutex status_mutex_;
    std::mutex alert_mutex_;
    std::mutex command_mutex_;
    
    // 运行状态
    bool is_running_;
    
    // WEB端口
    int32_t web_port_;
    
    // 告警阈值
    double cpu_threshold_;
    double memory_threshold_;
    int32_t online_threshold_;
    
    // 历史数据保留时间（秒）
    int32_t history_retention_;
};

} // namespace game_server

#endif // __MONITOR_SERVER_H__
