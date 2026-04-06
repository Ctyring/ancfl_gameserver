#ifndef __PROXY_SERVICE_H__
#define __PROXY_SERVICE_H__

#include "ancfl/ancfl.h"
#include "common/game_service_base.h"
#include "common/message_dispatcher.h"

namespace game_server {

/**
 * @brief 代理服务器服务类
 * 
 * 主要功能：
 * 1. 接受客户端连接
 * 2. 处理客户端消息
 * 3. 转发消息到逻辑服务器（待实现）
 * 4. 管理客户端连接
 * 
 * 继承自GameServiceBase，重写了以下方法：
 * - handleClient: 处理客户端连接
 * - InitService: 初始化服务
 * - UninitService: 反初始化服务
 */
class ProxyService : public GameServiceBase {
public:
    ProxyService();
    ~ProxyService();

    /**
     * @brief 初始化代理服务器服务
     * @return true 初始化成功
     * @return false 初始化失败
     */
    virtual bool InitService() override;
    
    /**
     * @brief 反初始化代理服务器服务
     */
    virtual void UninitService() override;
    
    /**
     * @brief 注册所有消息处理器
     */
    virtual void RegisterAllHandlers() override;
    
    /**
     * @brief 处理客户端连接（重写父类方法）
     * @param client 客户端socket
     */
    virtual void handleClient(ancfl::Socket::ptr client) override;
    
    /**
     * @brief 处理接收客户端消息
     * @param client 客户端socket
     * @param conn_id 连接ID
     */
    void HandleRecv(ancfl::Socket::ptr client, int32_t conn_id);
    
    /**
     * @brief 检查测试消息是否已发送
     * @return true 测试消息已发送
     * @return false 测试消息未发送
     */
    bool IsTestMessageSent() const { return test_msg_sent_; }
    
    /**
     * @brief 获取当前连接数
     * @return 当前连接数
     */
    size_t GetConnectionCount() const;
    
    /**
     * @brief 根据连接ID获取客户端socket
     * @param conn_id 连接ID
     * @return 客户端socket，如果不存在返回nullptr
     */
    ancfl::Socket::ptr GetClientByConnId(uint32_t conn_id);
    
    /**
     * @brief 设置最大连接数
     * @param max_connections 最大连接数
     */
    void SetMaxConnections(size_t max_connections);
    
    /**
     * @brief 获取最大连接数
     * @return 最大连接数
     */
    size_t GetMaxConnections() const { return max_connections_; }

    /**
     * @brief 5秒定时器回调
     */
    virtual void OnTimer5s() override;

protected:
    std::unordered_map<uint32_t, ancfl::Socket::ptr> connections_;  ///< 连接映射表：conn_id -> socket
    std::unordered_map<uint32_t, time_t> last_heart_time_;          ///< 心跳时间映射表：conn_id -> 最后心跳时间
    ancfl::Mutex conn_mutex_;                                        ///< 连接映射表的互斥锁
    int32_t next_conn_id_;                                           ///< 下一个连接ID
    size_t max_connections_;                                         ///< 最大连接数

private:
    bool test_msg_sent_;
    
    // 中心服务器连接
    int32_t center_server_id_;
    std::string center_server_ip_;
    int32_t center_server_port_;
    ancfl::Socket::ptr center_server_conn_;
    
    // 连接中心服务器
    bool ConnectToCenterServer();
    // 向中心服务器注册
    bool RegisterToCenterServer();
    // 发送心跳到中心服务器
    void SendHeartbeatToCenterServer();  ///< 测试消息是否已发送标志
};

} // namespace game_server

#endif  // __PROXY_SERVICE_H__
