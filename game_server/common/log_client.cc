#include "log_client.h"
#include "proto/msg_id.pb.h"
#include "proto/msg_log.pb.h"
#include "ancfl/log.h"
#include <ctime>

namespace game_server {

LogClient* g_log_client = nullptr;

LogClient::LogClient()
    : server_id_(0)
    , connected_(false) {
}

LogClient::~LogClient() {
    Disconnect();
}

bool LogClient::Connect(const std::string& ip, uint16_t port) {
    if (connected_) {
        return true;
    }
    
    tcp_client_ = std::make_shared<TcpClient>();
    if (!tcp_client_->Connect(ip, port)) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "LogClient connect failed: " << ip << ":" << port;
        return false;
    }
    
    connected_ = true;
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "LogClient connected to log server: " << ip << ":" << port;
    return true;
}

void LogClient::Disconnect() {
    if (!connected_) {
        return;
    }
    
    if (tcp_client_) {
        tcp_client_->Close();
        tcp_client_.reset();
    }
    
    connected_ = false;
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "LogClient disconnected";
}

bool LogClient::IsConnected() {
    return connected_ && tcp_client_ && tcp_client_->IsConnected();
}

bool LogClient::SendLog(const LogRecord& record, Callback callback) {
    msg_log::SendLogReq req;
    auto* msg_record = req.mutable_record();
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
    
    std::string data;
    req.SerializeToString(&data);
    
    return SendMessage(MSG_WRITE_LOG_REQ, data, callback);
}

bool LogClient::SendLogBatch(const std::vector<LogRecord>& records, Callback callback) {
    msg_log::SendLogBatchReq req;
    
    for (const auto& record : records) {
        auto* msg_record = req.add_records();
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
    
    std::string data;
    req.SerializeToString(&data);
    
    return SendMessage(MSG_WRITE_LOG_BATCH_REQ, data, callback);
}

bool LogClient::LogLogin(uint64_t role_id, const std::string& role_name, 
                         int32_t server_id, const std::string& ip, bool is_login,
                         Callback callback) {
    msg_log::LogRecord req;
    req.set_log_id(GenerateLogId());
    req.set_type(is_login ? msg_log::LogType::LOG_TYPE_LOGIN : msg_log::LogType::LOG_TYPE_LOGOUT);
    req.set_level(msg_log::LogLevel::LOG_LEVEL_INFO);
    req.set_server_id(server_id);
    req.set_role_id(role_id);
    req.set_role_name(role_name);
    req.set_content(is_login ? "Player login" : "Player logout");
    req.set_extra_data("ip=" + ip);
    req.set_log_time(time(nullptr));
    req.set_ip(ip);
    
    std::string data;
    req.SerializeToString(&data);
    
    return SendMessage(MSG_LOG_LOGIN_REQ, data, callback);
}

bool LogClient::LogRecharge(uint64_t role_id, const std::string& role_name, 
                            int32_t server_id, int32_t amount, int32_t product_id,
                            Callback callback) {
    msg_log::LogRecord req;
    req.set_log_id(GenerateLogId());
    req.set_type(msg_log::LogType::LOG_TYPE_RECHARGE);
    req.set_level(msg_log::LogLevel::LOG_LEVEL_INFO);
    req.set_server_id(server_id);
    req.set_role_id(role_id);
    req.set_role_name(role_name);
    req.set_content("Player recharge");
    
    char extra[256];
    snprintf(extra, sizeof(extra), "amount=%d,product_id=%d", amount, product_id);
    req.set_extra_data(extra);
    req.set_log_time(time(nullptr));
    
    std::string data;
    req.SerializeToString(&data);
    
    return SendMessage(MSG_LOG_RECHARGE_REQ, data, callback);
}

bool LogClient::LogConsume(uint64_t role_id, const std::string& role_name, 
                           int32_t server_id, int32_t type, int32_t amount, 
                           const std::string& reason, Callback callback) {
    msg_log::LogRecord req;
    req.set_log_id(GenerateLogId());
    req.set_type(msg_log::LogType::LOG_TYPE_CONSUME);
    req.set_level(msg_log::LogLevel::LOG_LEVEL_INFO);
    req.set_server_id(server_id);
    req.set_role_id(role_id);
    req.set_role_name(role_name);
    req.set_content("Player consume");
    
    char extra[256];
    snprintf(extra, sizeof(extra), "type=%d,amount=%d,reason=%s", type, amount, reason.c_str());
    req.set_extra_data(extra);
    req.set_log_time(time(nullptr));
    
    std::string data;
    req.SerializeToString(&data);
    
    return SendMessage(MSG_LOG_CONSUME_REQ, data, callback);
}

bool LogClient::LogItem(uint64_t role_id, const std::string& role_name, 
                        int32_t server_id, int32_t item_id, int32_t count, 
                        int32_t type, const std::string& reason, 
                        Callback callback) {
    msg_log::LogRecord req;
    req.set_log_id(GenerateLogId());
    req.set_type(msg_log::LogType::LOG_TYPE_ITEM);
    req.set_level(msg_log::LogLevel::LOG_LEVEL_INFO);
    req.set_server_id(server_id);
    req.set_role_id(role_id);
    req.set_role_name(role_name);
    req.set_content("Item change");
    
    char extra[256];
    snprintf(extra, sizeof(extra), "item_id=%d,count=%d,type=%d,reason=%s", 
             item_id, count, type, reason.c_str());
    req.set_extra_data(extra);
    req.set_log_time(time(nullptr));
    
    std::string data;
    req.SerializeToString(&data);
    
    return SendMessage(MSG_LOG_ITEM_REQ, data, callback);
}

bool LogClient::LogTrade(uint64_t role_id, const std::string& role_name, 
                         int32_t server_id, uint64_t target_id, 
                         const std::string& target_name, const std::string& items,
                         Callback callback) {
    msg_log::LogRecord req;
    req.set_log_id(GenerateLogId());
    req.set_type(msg_log::LogType::LOG_TYPE_TRADE);
    req.set_level(msg_log::LogLevel::LOG_LEVEL_INFO);
    req.set_server_id(server_id);
    req.set_role_id(role_id);
    req.set_role_name(role_name);
    req.set_content("Player trade");
    
    char extra[512];
    snprintf(extra, sizeof(extra), "target_id=%lu,target_name=%s,items=%s", 
             target_id, target_name.c_str(), items.c_str());
    req.set_extra_data(extra);
    req.set_log_time(time(nullptr));
    
    std::string data;
    req.SerializeToString(&data);
    
    return SendMessage(MSG_LOG_ITEM_REQ, data, callback);
}

bool LogClient::LogBattle(uint64_t role_id, const std::string& role_name, 
                          int32_t server_id, int32_t battle_type, 
                          int32_t result, int32_t duration,
                          Callback callback) {
    msg_log::LogRecord req;
    req.set_log_id(GenerateLogId());
    req.set_type(msg_log::LogType::LOG_TYPE_BATTLE);
    req.set_level(msg_log::LogLevel::LOG_LEVEL_INFO);
    req.set_server_id(server_id);
    req.set_role_id(role_id);
    req.set_role_name(role_name);
    req.set_content("Battle result");
    
    char extra[256];
    snprintf(extra, sizeof(extra), "battle_type=%d,result=%d,duration=%d", 
             battle_type, result, duration);
    req.set_extra_data(extra);
    req.set_log_time(time(nullptr));
    
    std::string data;
    req.SerializeToString(&data);
    
    return SendMessage(MSG_LOG_BATTLE_REQ, data, callback);
}

void LogClient::SetServerId(int32_t server_id) {
    server_id_ = server_id;
}

int32_t LogClient::GetServerId() const {
    return server_id_;
}

bool LogClient::SendMessage(uint32_t msg_id, const std::string& data, Callback callback) {
    if (!IsConnected()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "LogClient not connected";
        return false;
    }
    
    if (callback) {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        pending_callbacks_[msg_id] = callback;
    }
    
    return tcp_client_->SendMessage(msg_id, data);
}

int64_t LogClient::GenerateLogId() {
    static int64_t base_time = 1700000000;
    static int64_t counter = 0;
    return (time(nullptr) - base_time) * 1000000 + (++counter);
}

} // namespace game_server
