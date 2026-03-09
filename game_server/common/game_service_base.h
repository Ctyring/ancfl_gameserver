#ifndef __GAME_SERVICE_BASE_H__
#define __GAME_SERVICE_BASE_H__

#include "tcp_service.h"

namespace game_server {

// 游戏服务基类
class GameServiceBase : public TcpService {
public:
    using ptr = std::shared_ptr<GameServiceBase>;

    GameServiceBase(const std::string& service_name);
    virtual ~GameServiceBase();

    // 获取服务名称
    const std::string& GetServiceName() const { return service_name_; }

    // 获取服务ID
    uint32_t GetServiceId() const { return service_id_; }

    // 设置服务ID
    void SetServiceId(uint32_t id) { service_id_ = id; }

    // 初始化服务
    virtual bool InitService() = 0;

    // 反初始化服务
    virtual void UninitService() = 0;

    // 主循环
    virtual void MainLoop();

    // 停止服务
    virtual void StopService();

    // 注册所有消息处理器
    virtual void RegisterAllHandlers() = 0;

    // 连接其他服务器
    virtual bool ConnectToOtherServers() { return true; }

    // 注册到服务器
    virtual bool RegisterToServers() { return true; }

    // 每帧更新
    virtual void OnUpdate() {}

    // 每秒定时器
    virtual void OnTimer() {}

    // 每5秒定时器
    virtual void OnTimer5s() {}

    // 每30秒定时器
    virtual void OnTimer30s() {}

protected:
    std::string service_name_;
    uint32_t service_id_;
    bool stop_flag_;
    
    // 定时器计数
    int32_t timer_counter_;
    int32_t timer5s_counter_;
    int32_t timer30s_counter_;
};

// 服务单例宏
#define SERVICE_SINGLETON(ClassName) \
public: \
    static ClassName* GetInstance() { \
        static ClassName instance; \
        return &instance; \
    } \
private: \
    ClassName(); \
    ~ClassName(); \
    ClassName(const ClassName&) = delete; \
    ClassName& operator=(const ClassName&) = delete

} // namespace game_server

#endif // __GAME_SERVICE_BASE_H__
