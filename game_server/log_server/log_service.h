#ifndef __LOG_SERVICE_H__
#define __LOG_SERVICE_H__

#include "../common/game_service_base.h"
#include "../common/message_dispatcher.h"

namespace game_server {

// 日志服务器服务类
class LogService : public GameServiceBase {
public:
    LogService();
    ~LogService();

    // 初始化服务
    virtual bool InitService() override;

    // 反初始化服务
    virtual void UninitService() override;

    // 注册所有消息处理器
    virtual void RegisterAllHandlers() override;

    // 每秒定时器
    virtual void OnTimer() override;
    
    // 5秒定时器
    virtual void OnTimer5s() override;
    
    // 连接中心服务器
    bool ConnectToCenterServer();
    
    // 注册到中心服务器
    bool RegisterToCenterServer();
    
    // 发送心跳到中心服务器
    void SendHeartbeatToCenterServer();

private:
    // 中心服务器配置
    std::string center_server_ip_;
    int32_t center_server_port_;
    
    // 中心服务器连接
    ancfl::Socket::ptr center_server_conn_;
};

} // namespace game_server

#endif // __LOG_SERVICE_H__
