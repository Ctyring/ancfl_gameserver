#include "tcp_service.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>

namespace game_server {

TcpService::TcpService(ancfl::IOManager* worker, 
                       ancfl::IOManager* accept_worker)
    : ancfl::TcpServer(worker, accept_worker)
    , next_conn_id_(1)
    , running_(false)
    , port_(0)
    , heart_interval_(30) {
}

TcpService::~TcpService() {
    Stop();
}

bool TcpService::Init(const std::string& ip, uint16_t port) {
    ip_ = ip;
    port_ = port;
    
    auto addr = ancfl::Address::LookupAnyIPAddress(ip + ":" + std::to_string(port));
    if (!addr) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Lookup address failed: " << ip << ":" << port;
        return false;
    }

    if (!bind(addr)) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Bind failed: " << *addr;
        return false;
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "TCP Service initialized on " << *addr;
    return true;
}

void TcpService::Uninit() {
    Stop();
    connections_.clear();
    last_heart_time_.clear();
}

void TcpService::Run() {
    running_ = true;
    start();
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "TCP Service started";
}

void TcpService::Stop() {
    running_ = false;
    stop();
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "TCP Service stopped";
}

void TcpService::handleClient(ancfl::Socket::ptr client) {
    int32_t conn_id;
    {
        ancfl::Mutex::Lock lock(conn_mutex_);
        conn_id = next_conn_id_++;
        connections_[conn_id] = client;
        last_heart_time_[conn_id] = time(nullptr);
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "New connection, conn_id=" << conn_id 
                                     << " addr=" << *client->getRemoteAddress();

    // 启动接收协程
    ancfl::Fiber::ptr recv_fiber(new ancfl::Fiber(
        std::bind(&TcpService::HandleRecv, this, client)));
    recv_fiber->call();

    // 连接断开处理
    {
        ancfl::Mutex::Lock lock(conn_mutex_);
        connections_.erase(conn_id);
        last_heart_time_.erase(conn_id);
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Connection closed, conn_id=" << conn_id;
}

bool TcpService::SendMsgToServer(int32_t server_conn_id, uint32_t msg_id, const std::string& data) {
    return SendRawData(server_conn_id, msg_id, 0, 0, data.data(), data.size());
}

bool TcpService::SendMsgToClient(int32_t client_conn_id, uint32_t msg_id, const std::string& data) {
    return SendRawData(client_conn_id, msg_id, 0, 0, data.data(), data.size());
}

void TcpService::HandleRecv(ancfl::Socket::ptr client) {
    int32_t conn_id = -1;
    {
        ancfl::Mutex::Lock lock(conn_mutex_);
        for (auto& pair : connections_) {
            if (pair.second == client) {
                conn_id = pair.first;
                break;
            }
        }
    }

    if (conn_id < 0) {
        return;
    }

    std::vector<char> buffer(8192);
    while (running_) {
        // 接收包头
        MessageHeader header;
        int ret = client->recv(&header, sizeof(header));
        if (ret <= 0) {
            break;
        }

        // 大小端转换
        header.msg_id = ancfl::ByteSwapOnLittleEndian(header.msg_id);
        header.msg_len = ancfl::ByteSwapOnLittleEndian(header.msg_len);
        header.target_id = ancfl::ByteSwapOnLittleEndian(header.target_id);
        header.user_data = ancfl::ByteSwapOnLittleEndian(header.user_data);

        if (header.msg_len > 32768 || header.msg_len < sizeof(header)) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Invalid message length: " << header.msg_len;
            break;
        }

        // 接收消息体
        uint32_t body_len = header.msg_len - sizeof(header);
        if (body_len > 0) {
            if (buffer.size() < body_len) {
                buffer.resize(body_len);
            }
            
            ret = client->recv(buffer.data(), body_len);
            if (ret <= 0) {
                break;
            }
        }

        // 更新心跳时间
        {
            ancfl::Mutex::Lock lock(conn_mutex_);
            last_heart_time_[conn_id] = time(nullptr);
        }

        // 解析并分发消息
        NetPacket packet;
        packet.conn_id = conn_id;
        packet.msg_id = header.msg_id;
        packet.target_id = header.target_id;
        packet.user_data = header.user_data;

        if (ParseMessage(buffer.data(), body_len, packet)) {
            if (dispatcher_) {
                dispatcher_->Dispatch(packet);
            }
        }
    }
}

bool TcpService::ParseMessage(const char* data, uint32_t len, NetPacket& packet) {
    auto msg = CreateMessage(packet.msg_id);
    if (!msg) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Unknown message id: " << packet.msg_id;
        return false;
    }

    if (!msg->ParseFromArray(data, len)) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Parse message failed, id: " << packet.msg_id;
        return false;
    }

    packet.msg = msg;
    return true;
}

bool TcpService::SerializeMessage(const google::protobuf::Message& msg, 
                                   std::string& out_data) {
    return msg.SerializeToString(&out_data);
}

std::shared_ptr<google::protobuf::Message> TcpService::CreateMessage(uint32_t msg_id) {
    // 子类需要实现此方法，根据msg_id创建对应的Protobuf消息
    return nullptr;
}

bool TcpService::SendMessage(int32_t conn_id, uint32_t msg_id, uint64_t target_id,
                             uint32_t user_data, const google::protobuf::Message& msg) {
    std::string body_data;
    if (!SerializeMessage(msg, body_data)) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Serialize message failed";
        return false;
    }

    return SendRawData(conn_id, msg_id, target_id, user_data, 
                       body_data.data(), body_data.size());
}

bool TcpService::SendRawData(int32_t conn_id, uint32_t msg_id, uint64_t target_id,
                             uint32_t user_data, const char* data, uint32_t len) {
    ancfl::Socket::ptr client;
    {
        ancfl::Mutex::Lock lock(conn_mutex_);
        auto it = connections_.find(conn_id);
        if (it == connections_.end()) {
            return false;
        }
        client = it->second;
    }

    // 构造包头
    MessageHeader header;
    header.msg_id = ancfl::ByteSwapOnLittleEndian(msg_id);
    header.msg_len = ancfl::ByteSwapOnLittleEndian(len + sizeof(header));
    header.target_id = ancfl::ByteSwapOnLittleEndian(target_id);
    header.user_data = ancfl::ByteSwapOnLittleEndian(user_data);

    // 发送包头
    if (client->send(&header, sizeof(header)) <= 0) {
        return false;
    }

    // 发送消息体
    if (len > 0 && client->send(data, len) <= 0) {
        return false;
    }

    return true;
}

bool TcpService::CloseConnection(int32_t conn_id) {
    ancfl::Socket::ptr client;
    {
        ancfl::Mutex::Lock lock(conn_mutex_);
        auto it = connections_.find(conn_id);
        if (it == connections_.end()) {
            return false;
        }
        client = it->second;
        connections_.erase(it);
        last_heart_time_.erase(conn_id);
    }

    client->close();
    return true;
}

size_t TcpService::GetConnectionCount() const {
    ancfl::Mutex::Lock lock(conn_mutex_);
    return connections_.size();
}

void TcpService::SetMessageDispatcher(MessageDispatcher::ptr dispatcher) {
    dispatcher_ = dispatcher;
}

void TcpService::Update() {
    // 检查心跳超时
    int64_t now = time(nullptr);
    std::vector<int32_t> timeout_conns;
    
    {
        ancfl::Mutex::Lock lock(conn_mutex_);
        for (auto& pair : last_heart_time_) {
            if (now - pair.second > heart_interval_ * 2) {
                timeout_conns.push_back(pair.first);
            }
        }
    }

    for (auto conn_id : timeout_conns) {
        ANCFL_LOG_WARN(ANCFL_LOG_ROOT()) << "Connection heart timeout, conn_id=" << conn_id;
        CloseConnection(conn_id);
    }
}

void TcpService::OnSecondTimer() {
    Update();
}

} // namespace game_server
