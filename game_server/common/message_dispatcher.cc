#include "message_dispatcher.h"

namespace game_server {

MessageDispatcher::MessageDispatcher() {
}

MessageDispatcher::~MessageDispatcher() {
}

void MessageDispatcher::RegisterHandler(uint32_t msg_id, MessageHandler handler) {
    ancfl::Mutex::Lock lock(mutex_);
    handlers_[msg_id] = handler;
}

bool MessageDispatcher::Dispatch(const NetPacket& packet) {
    MessageHandler handler;
    {
        ancfl::Mutex::Lock lock(mutex_);
        auto it = handlers_.find(packet.msg_id);
        if (it == handlers_.end()) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "No handler for message id: " << packet.msg_id;
            return false;
        }
        handler = it->second;
    }
    
    if (handler) {
        return handler(packet);
    }
    return false;
}

bool MessageDispatcher::HasHandler(uint32_t msg_id) const {
    ancfl::Mutex::Lock lock(mutex_);
    return handlers_.find(msg_id) != handlers_.end();
}

size_t MessageDispatcher::GetHandlerCount() const {
    ancfl::Mutex::Lock lock(mutex_);
    return handlers_.size();
}

MessageHandlerBase::MessageHandlerBase() {
}

MessageHandlerBase::~MessageHandlerBase() {
}

} // namespace game_server
