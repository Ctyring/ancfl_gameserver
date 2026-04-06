#ifndef __LOGIC_SERVICE_H__
#define __LOGIC_SERVICE_H__

#include "../common/game_service_base.h"
#include "../common/message_dispatcher.h"

namespace game_server {

// 逻辑服务器服务类
class LogicService : public GameServiceBase {
public:
    LogicService(int32_t server_id);
    ~LogicService();

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

    // 获取服务器ID
    int32_t GetServerId() const { return server_id_; }

private:
    // 服务器ID
    int32_t server_id_;
    
    // 中心服务器配置
    std::string center_server_ip_;
    int32_t center_server_port_;
    
    // 中心服务器连接
    ancfl::Socket::ptr center_server_conn_;
    
    // 在线人数
    int32_t online_count_;
    
    // 最大在线人数
    int32_t max_online_;
};

} // namespace game_server

#endif // __LOGIC_SERVICE_H__
