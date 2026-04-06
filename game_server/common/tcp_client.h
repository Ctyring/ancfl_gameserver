#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__

#include <google/protobuf/message.h>
#include "ancfl/ancfl.h"
#include "message_dispatcher.h"

namespace game_server {

// TCP客户端类
class TcpClient {
public:
    TcpClient();
    ~TcpClient();

    // 连接服务器
    bool Connect(const std::string& addr);

    // 断开连接
    void Disconnect();

    // 发送消息
    bool SendMessage(uint32_t msg_id, uint64_t target_id, uint32_t user_data, const std::string& data);

    // 发送Protobuf消息
    bool SendMessage(uint32_t msg_id, uint64_t target_id, uint32_t user_data, const google::protobuf::Message& msg);

    // 接收消息
    bool RecvMessage(NetPacket& packet);

    // 检查连接状态
    bool IsConnected() const;

    // 设置消息分发器
    void SetMessageDispatcher(MessageDispatcher::ptr dispatcher);

    // 处理接收到的数据
    void HandleRecv();

    // 获取底层 socket 对象
    const ancfl::Socket::ptr& GetSocket() const { return socket_; }

private:
    // 解析消息
    bool ParseMessage(const char* data, uint32_t len, NetPacket& packet);

    // 序列化消息
    bool SerializeMessage(const google::protobuf::Message& msg, std::string& out_data);

    // 创建Protobuf消息
    std::shared_ptr<google::protobuf::Message> CreateMessage(uint32_t msg_id);

private:
    ancfl::Socket::ptr socket_;
    MessageDispatcher::ptr dispatcher_;
    bool connected_;
    std::string recv_buffer_;
    std::mutex mutex_;
};

}  // namespace game_server

#endif  // __TCP_CLIENT_H__