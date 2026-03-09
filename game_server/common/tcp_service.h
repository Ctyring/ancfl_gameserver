#ifndef __TCP_SERVICE_H__
#define __TCP_SERVICE_H__

#include "ancfl/ancfl.h"
#include "message_dispatcher.h"
#include <google/protobuf/message.h>

namespace game_server {

// TCP服务基类
class TcpService : public ancfl::TcpServer {
public:
    using ptr = std::shared_ptr<TcpService>;

    TcpService(ancfl::IOManager* worker = nullptr, 
               ancfl::IOManager* accept_worker = nullptr);
    virtual ~TcpService();

    // 初始化服务
    virtual bool Init(const std::string& ip, uint16_t port);

    // 反初始化
    virtual void Uninit();

    // 运行服务
    virtual void Run();

    // 停止服务
    virtual void Stop();

    // 发送消息
    bool SendMessage(int32_t conn_id, uint32_t msg_id, uint64_t target_id, 
                     uint32_t user_data, const google::protobuf::Message& msg);

    // 发送原始数据
    bool SendRawData(int32_t conn_id, uint32_t msg_id, uint64_t target_id,
                     uint32_t user_data, const char* data, uint32_t len);

    // 关闭连接
    bool CloseConnection(int32_t conn_id);

    // 获取连接数量
    size_t GetConnectionCount() const;

    // 设置消息分发器
    void SetMessageDispatcher(MessageDispatcher::ptr dispatcher);

    // 每帧更新
    virtual void Update();

    // 每秒定时器
    virtual void OnSecondTimer();

protected:
    // 处理客户端连接
    virtual void handleClient(ancfl::Socket::ptr client) override;

    // 处理数据接收
    void HandleRecv(ancfl::Socket::ptr client);

    // 消息解析
    virtual bool ParseMessage(const char* data, uint32_t len, NetPacket& packet);

    // 序列化消息
    virtual bool SerializeMessage(const google::protobuf::Message& msg, 
                                   std::string& out_data);

    // 创建Protobuf消息
    virtual std::shared_ptr<google::protobuf::Message> CreateMessage(uint32_t msg_id);

protected:
    MessageDispatcher::ptr dispatcher_;
    std::unordered_map<int32_t, ancfl::Socket::ptr> connections_;
    ancfl::Mutex conn_mutex_;
    int32_t next_conn_id_;
    bool running_;
    std::string ip_;
    uint16_t port_;

    // 心跳相关
    int32_t heart_interval_;  // 心跳间隔(秒)
    std::unordered_map<int32_t, int64_t> last_heart_time_;
};

} // namespace game_server

#endif // __TCP_SERVICE_H__
