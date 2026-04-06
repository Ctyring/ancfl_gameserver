#include "monitor_server.h"
#include <sstream>
#include <algorithm>
#include <iostream>

namespace game_server {

MonitorServer::MonitorServer() 
    : is_running_(false)
    , web_port_(8080)
    , cpu_threshold_(80.0)
    , memory_threshold_(80.0)
    , online_threshold_(90)
    , history_retention_(3600) {
}

MonitorServer::~MonitorServer() {
    Stop();
}

bool MonitorServer::Init(const std::string& config_file) {
    std::cout << "[INFO] Monitor server initializing: config=" << config_file << std::endl;
    return true;
}

bool MonitorServer::Start() {
    if (is_running_) {
        return true;
    }
    
    is_running_ = true;
    
    std::cout << "[INFO] Monitor server started: web_port=" << web_port_ << std::endl;
    return true;
}

void MonitorServer::Stop() {
    is_running_ = false;
    std::cout << "[INFO] Monitor server stopped" << std::endl;
}

bool MonitorServer::IsRunning() {
    return is_running_;
}

bool MonitorServer::UpdateServerStatus(const ServerPerfData& data) {
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    ServerPerfData new_data = data;
    if (new_data.update_time == 0) {
        new_data.update_time = time(nullptr);
    }
    server_status_[new_data.server_id] = new_data;
    
    auto& history = perf_history_[new_data.server_id];
    history.push_back(new_data);
    
    if (history.size() > 1000) {
        history.erase(history.begin());
    }
    
    return true;
}

bool MonitorServer::GetServerStatus(int32_t server_id, ServerPerfData& data) {
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    auto it = server_status_.find(server_id);
    if (it == server_status_.end()) {
        return false;
    }
    
    data = it->second;
    return true;
}

bool MonitorServer::GetAllServerStatus(std::vector<ServerPerfData>& servers) {
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    servers.clear();
    for (const auto& pair : server_status_) {
        servers.push_back(pair.second);
    }
    
    return true;
}

bool MonitorServer::AddAlert(const AlertInfo& alert) {
    std::lock_guard<std::mutex> lock(alert_mutex_);
    
    AlertInfo new_alert = alert;
    if (new_alert.alert_id == 0) {
        new_alert.alert_id = GenerateAlertId();
    }
    
    alerts_.push_back(new_alert);
    
    std::cout << "[WARN] Alert added: alert_id=" << new_alert.alert_id
              << ", type=" << static_cast<int32_t>(new_alert.type) 
              << ", server_id=" << new_alert.server_id 
              << ", message=" << new_alert.message << std::endl;
    return true;
}

bool MonitorServer::GetAlerts(std::vector<AlertInfo>& alerts, bool include_handled) {
    std::lock_guard<std::mutex> lock(alert_mutex_);
    
    alerts.clear();
    for (const auto& alert : alerts_) {
        if (include_handled || !alert.is_handled) {
            alerts.push_back(alert);
        }
    }
    
    return true;
}

bool MonitorServer::HandleAlert(int32_t alert_id, const std::string& handler) {
    std::lock_guard<std::mutex> lock(alert_mutex_);
    
    for (auto& alert : alerts_) {
        if (alert.alert_id == alert_id) {
            alert.is_handled = true;
            alert.handler = handler;
            alert.handle_time = time(nullptr);
            std::cout << "[INFO] Alert handled: alert_id=" << alert_id 
                      << ", handler=" << handler << std::endl;
            return true;
        }
    }
    
    return false;
}

bool MonitorServer::ClearAlerts() {
    std::lock_guard<std::mutex> lock(alert_mutex_);
    alerts_.clear();
    return true;
}

bool MonitorServer::SendCommand(int32_t server_id, ControlCommand command, const std::string& params, int32_t& command_id) {
    std::lock_guard<std::mutex> lock(command_mutex_);
    
    command_id = GenerateCommandId();
    
    CommandResult result;
    result.command_id = command_id;
    result.command = command;
    result.server_id = server_id;
    result.success = true;
    result.message = "Command sent successfully";
    result.execute_time = time(nullptr);
    
    command_results_[command_id] = result;
    
    std::cout << "[INFO] Command sent: command_id=" << command_id 
              << ", server_id=" << server_id 
              << ", command=" << static_cast<int32_t>(command) << std::endl;
    return true;
}

bool MonitorServer::GetCommandResult(int32_t command_id, CommandResult& result) {
    std::lock_guard<std::mutex> lock(command_mutex_);
    
    auto it = command_results_.find(command_id);
    if (it == command_results_.end()) {
        return false;
    }
    
    result = it->second;
    return true;
}

bool MonitorServer::HandleWebRequest(const std::string& path, const std::string& method, const std::string& body, std::string& response) {
    // 简单的WEB接口处理
    std::ostringstream oss;
    
    if (path == "/api/servers") {
        // 获取所有服务器状态
        std::vector<ServerPerfData> servers;
        GetAllServerStatus(servers);
        
        oss << "{\"servers\":[";
        for (size_t i = 0; i < servers.size(); ++i) {
            const auto& s = servers[i];
            if (i > 0) oss << ",";
            oss << "{"
                << "\"server_id\":" << s.server_id << ","
                << "\"server_name\":\"" << s.server_name << "\","
                << "\"server_type\":" << s.server_type << ","
                << "\"status\":" << static_cast<int32_t>(s.status) << ","
                << "\"online_count\":" << s.online_count << ","
                << "\"cpu_usage\":" << s.cpu_usage << ","
                << "\"memory_usage\":" << s.memory_usage
                << "}";
        }
        oss << "]}";
    } else if (path == "/api/alerts") {
        // 获取告警列表
        std::vector<AlertInfo> alerts;
        GetAlerts(alerts, false);
        
        oss << "{\"alerts\":[";
        for (size_t i = 0; i < alerts.size(); ++i) {
            const auto& a = alerts[i];
            if (i > 0) oss << ",";
            oss << "{"
                << "\"alert_id\":" << a.alert_id << ","
                << "\"type\":" << static_cast<int32_t>(a.type) << ","
                << "\"server_id\":" << a.server_id << ","
                << "\"message\":\"" << a.message << "\","
                << "\"alert_time\":" << a.alert_time
                << "}";
        }
        oss << "]}";
    } else if (path == "/api/command") {
        // 发送控制命令
        // TODO: 解析body获取命令参数
        oss << "{\"result\":0,\"message\":\"Command sent\"}";
    } else {
        oss << "{\"error\":\"Unknown path\"}";
    }
    
    response = oss.str();
    return true;
}

bool MonitorServer::GetOnlineTrend(int32_t server_id, std::vector<std::pair<time_t, int32_t>>& trend) {
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    auto it = perf_history_.find(server_id);
    if (it == perf_history_.end()) {
        return false;
    }
    
    trend.clear();
    for (const auto& data : it->second) {
        trend.push_back(std::make_pair(data.update_time, data.online_count));
    }
    
    return true;
}

bool MonitorServer::GetPerformanceHistory(int32_t server_id, std::vector<ServerPerfData>& history) {
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    auto it = perf_history_.find(server_id);
    if (it == perf_history_.end()) {
        return false;
    }
    
    history = it->second;
    return true;
}

void MonitorServer::OnTimer() {
    CheckServerStatus();
    CheckAlerts();
}

void MonitorServer::CheckAlerts() {
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    time_t now = time(nullptr);
    
    for (const auto& pair : server_status_) {
        const auto& data = pair.second;
        
        // 检查CPU使用率
        if (data.cpu_usage > cpu_threshold_) {
            AlertInfo alert;
            alert.alert_id = GenerateAlertId();
            alert.type = AlertType::CPU_HIGH;
            alert.server_id = data.server_id;
            alert.message = "CPU usage too high: " + std::to_string(data.cpu_usage) + "%";
            alert.alert_time = now;
            alert.is_handled = false;
            AddAlert(alert);
        }
        
        // 检查内存使用率
        if (data.memory_usage > memory_threshold_) {
            AlertInfo alert;
            alert.alert_id = GenerateAlertId();
            alert.type = AlertType::MEMORY_HIGH;
            alert.server_id = data.server_id;
            alert.message = "Memory usage too high: " + std::to_string(data.memory_usage) + "%";
            alert.alert_time = now;
            alert.is_handled = false;
            AddAlert(alert);
        }
        
        // 检查在线人数
        if (data.max_online > 0 && data.online_count * 100 / data.max_online > online_threshold_) {
            AlertInfo alert;
            alert.alert_id = GenerateAlertId();
            alert.type = AlertType::ONLINE_HIGH;
            alert.server_id = data.server_id;
            alert.message = "Online count too high: " + std::to_string(data.online_count) + "/" + std::to_string(data.max_online);
            alert.alert_time = now;
            alert.is_handled = false;
            AddAlert(alert);
        }
    }
}

void MonitorServer::CheckServerStatus() {
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    time_t now = time(nullptr);
    
    for (auto& pair : server_status_) {
        // 超过30秒未更新视为离线
        if (now - pair.second.update_time > 30) {
            pair.second.status = MonitorServerStatus::OFFLINE;
            
            AlertInfo alert;
            alert.alert_id = GenerateAlertId();
            alert.type = AlertType::SERVER_DOWN;
            alert.server_id = pair.first;
            alert.message = "Server offline: " + pair.second.server_name;
            alert.alert_time = now;
            alert.is_handled = false;
            AddAlert(alert);
        }
    }
}

int32_t MonitorServer::GenerateCommandId() {
    static int32_t next_id = 1;
    return next_id++;
}

int32_t MonitorServer::GenerateAlertId() {
    static int32_t next_id = 1;
    return next_id++;
}

} // namespace game_server
