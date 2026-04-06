#ifndef __LOG_CLIENT_H__
#define __LOG_CLIENT_H__

#include "log_server/log_server.h"
#include "common/tcp_client.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace game_server {

class LogClient {
public:
    using ptr = std::shared_ptr<LogClient>;
    using Callback = std::function<void(int result)>;
    
    LogClient();
    ~LogClient();
    
    bool Connect(const std::string& ip, uint16_t port);
    void Disconnect();
    bool IsConnected();
    
    bool SendLog(const LogRecord& record, Callback callback = nullptr);
    bool SendLogBatch(const std::vector<LogRecord>& records, Callback callback = nullptr);
    
    bool LogLogin(uint64_t role_id, const std::string& role_name, 
                  int32_t server_id, const std::string& ip, bool is_login,
                  Callback callback = nullptr);
    bool LogRecharge(uint64_t role_id, const std::string& role_name, 
                     int32_t server_id, int32_t amount, int32_t product_id,
                     Callback callback = nullptr);
    bool LogConsume(uint64_t role_id, const std::string& role_name, 
                    int32_t server_id, int32_t type, int32_t amount, 
                    const std::string& reason, Callback callback = nullptr);
    bool LogItem(uint64_t role_id, const std::string& role_name, 
                 int32_t server_id, int32_t item_id, int32_t count, 
                 int32_t type, const std::string& reason, 
                 Callback callback = nullptr);
    bool LogTrade(uint64_t role_id, const std::string& role_name, 
                  int32_t server_id, uint64_t target_id, 
                  const std::string& target_name, const std::string& items,
                  Callback callback = nullptr);
    bool LogBattle(uint64_t role_id, const std::string& role_name, 
                   int32_t server_id, int32_t battle_type, 
                   int32_t result, int32_t duration,
                   Callback callback = nullptr);
    
    void SetServerId(int32_t server_id);
    int32_t GetServerId() const;
    
private:
    bool SendMessage(uint32_t msg_id, const std::string& data, Callback callback);
    int64_t GenerateLogId();
    
    TcpClient::ptr tcp_client_;
    int32_t server_id_;
    bool connected_;
    
    std::unordered_map<int64_t, Callback> pending_callbacks_;
    std::mutex callback_mutex_;
};

extern LogClient* g_log_client;

#define LOG_CLIENT_LOGIN(role_id, role_name, ip, is_login) \
    if (g_log_client) { \
        g_log_client->LogLogin(role_id, role_name, \
            g_log_client->GetServerId(), ip, is_login); \
    }

#define LOG_CLIENT_RECHARGE(role_id, role_name, amount, product_id) \
    if (g_log_client) { \
        g_log_client->LogRecharge(role_id, role_name, \
            g_log_client->GetServerId(), amount, product_id); \
    }

#define LOG_CLIENT_CONSUME(role_id, role_name, type, amount, reason) \
    if (g_log_client) { \
        g_log_client->LogConsume(role_id, role_name, \
            g_log_client->GetServerId(), type, amount, reason); \
    }

#define LOG_CLIENT_ITEM(role_id, role_name, item_id, count, type, reason) \
    if (g_log_client) { \
        g_log_client->LogItem(role_id, role_name, \
            g_log_client->GetServerId(), item_id, count, type, reason); \
    }

#define LOG_CLIENT_BATTLE(role_id, role_name, battle_type, result, duration) \
    if (g_log_client) { \
        g_log_client->LogBattle(role_id, role_name, \
            g_log_client->GetServerId(), battle_type, result, duration); \
    }

} // namespace game_server

#endif // __LOG_CLIENT_H__
