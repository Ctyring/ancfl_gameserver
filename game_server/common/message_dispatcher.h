#ifndef __MESSAGE_DISPATCHER_H__
#define __MESSAGE_DISPATCHER_H__

#include "ancfl/ancfl.h"
#include <google/protobuf/message.h>
#include <functional>
#include <unordered_map>

namespace game_server {

// 消息包头
struct MessageHeader {
    uint32_t msg_id;
    uint32_t msg_len;
    uint64_t target_id;
    uint32_t user_data;
};

// 网络包结构
struct NetPacket {
    int32_t conn_id;
    uint32_t msg_id;
    uint64_t target_id;
    uint32_t user_data;
    std::shared_ptr<google::protobuf::Message> msg;
};

// 消息处理器函数类型
using MessageHandler = std::function<bool(const NetPacket&)>;

// 消息分发器
class MessageDispatcher {
public:
    using ptr = std::shared_ptr<MessageDispatcher>;

    MessageDispatcher();
    ~MessageDispatcher();

    // 注册消息处理器
    void RegisterHandler(uint32_t msg_id, MessageHandler handler);

    // 分发消息
    bool Dispatch(const NetPacket& packet);

    // 检查是否有处理器
    bool HasHandler(uint32_t msg_id) const;

    // 获取处理器数量
    size_t GetHandlerCount() const;

private:
    std::unordered_map<uint32_t, MessageHandler> handlers_;
    ancfl::Mutex mutex_;
};

// 消息处理器基类
class MessageHandlerBase {
public:
    using ptr = std::shared_ptr<MessageHandlerBase>;

    MessageHandlerBase();
    virtual ~MessageHandlerBase();

    // 初始化
    virtual bool Init() = 0;

    // 反初始化
    virtual void Uninit() = 0;

    // 注册所有消息处理器
    virtual void RegisterHandlers(MessageDispatcher::ptr dispatcher) = 0;

    // 每帧更新
    virtual void Update() {}

    // 每秒定时器
    virtual void OnSecondTimer() {}
};

// 消息处理宏
#define REGISTER_MESSAGE(dispatcher, msg_id, func) \
    dispatcher->RegisterHandler(msg_id, std::bind(&func, this, std::placeholders::_1))

// 解析消息宏
#define PARSE_MESSAGE(packet, msg_type) \
    std::dynamic_pointer_cast<msg_type>(packet.msg)

} // namespace game_server

#endif // __MESSAGE_DISPATCHER_H__
