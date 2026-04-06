#include "tcp_client.h"
#include <fcntl.h>
#include "proto/msg_id.pb.h"
#include "proto/msg_account.pb.h"

namespace game_server {

TcpClient::TcpClient() : socket_(nullptr), connected_(false) {
}

TcpClient::~TcpClient() {
    Disconnect();
}

bool TcpClient::Connect(const std::string& addr) {
    try {
        size_t pos = addr.find(":");
        if (pos == std::string::npos) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Invalid address format: " << addr;
            std::cerr << "[Client] Invalid address format: " << addr << std::endl;
            return false;
        }

        std::string host = addr.substr(0, pos);
        std::string port_str = addr.substr(pos + 1);
        std::string host_with_port = host + ":" + port_str;

        socket_ = ancfl::Socket::CreateTCPSocket();
        if (!socket_) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to create socket";
            std::cerr << "[Client] Failed to create socket" << std::endl;
            return false;
        }

        auto address = ancfl::Address::LookupAnyIPAddress(host_with_port);
        if (!address) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to create address";
            std::cerr << "[Client] Failed to create address" << std::endl;
            return false;
        }

        if (!socket_->connect(address)) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to connect to " << addr;
            std::cerr << "[Client] Failed to connect to " << addr << std::endl;
            return false;
        }

        connected_ = true;
        ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Connected to " << addr;
        std::cerr << "[Client] Connected to " << addr << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Connect exception: " << e.what();
        std::cerr << "[Client] Connect exception: " << e.what() << std::endl;
        return false;
    }
}

void TcpClient::Disconnect() {
    if (socket_) {
        socket_->close();
        socket_ = nullptr;
    }
    connected_ = false;
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Disconnected";
}

bool TcpClient::SendMessage(uint32_t msg_id, uint64_t target_id, uint32_t user_data, const std::string& data) {
    if (!connected_ || !socket_) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Not connected";
        return false;
    }

    try {
        MessageHeader header;
        header.msg_id = ancfl::byteswapOnLittleEndian(msg_id);
        uint32_t total_len = data.size() + sizeof(header);
        header.msg_len = ancfl::byteswapOnLittleEndian(total_len);
        header.target_id = ancfl::byteswapOnLittleEndian(target_id);
        header.user_data = ancfl::byteswapOnLittleEndian(user_data);

        const char* header_ptr = reinterpret_cast<const char*>(&header);
        size_t header_sent = 0;
        while (header_sent < sizeof(header)) {
            int ret = socket_->send(header_ptr + header_sent, sizeof(header) - header_sent, 0);
            if (ret < 0) {
                if (errno == EINPROGRESS || errno == EAGAIN || errno == EWOULDBLOCK) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                } else {
                    ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to send header, ret: " << ret;
                    return false;
                }
            } else if (ret == 0) {
                ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Connection closed by server";
                return false;
            }
            header_sent += ret;
        }

        if (data.size() > 0) {
            size_t data_sent = 0;
            while (data_sent < data.size()) {
                int ret = socket_->send(data.data() + data_sent, data.size() - data_sent, 0);
                if (ret < 0) {
                    if (errno == EINPROGRESS || errno == EAGAIN || errno == EWOULDBLOCK) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        continue;
                    } else {
                        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to send data, ret: " << ret;
                        return false;
                    }
                } else if (ret == 0) {
                    ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Connection closed by server";
                    return false;
                }
                data_sent += ret;
            }
        }

        ANCFL_LOG_DEBUG(ANCFL_LOG_ROOT()) << "Sent message: msg_id=" << msg_id << ", len=" << data.size();
        return true;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "SendMessage exception: " << e.what();
        return false;
    }
}

bool TcpClient::SendMessage(uint32_t msg_id, uint64_t target_id, uint32_t user_data, const google::protobuf::Message& msg) {
    std::string data;
    if (!SerializeMessage(msg, data)) {
        return false;
    }
    return SendMessage(msg_id, target_id, user_data, data);
}

bool TcpClient::RecvMessage(NetPacket& packet) {
    if (!connected_ || !socket_) {
        return false;
    }

    try {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(socket_->getSocket(), &read_fds);
        
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        
        int ret = select(socket_->getSocket() + 1, &read_fds, NULL, NULL, &timeout);
        
        if (ret <= 0) {
            return false;
        }
        
        MessageHeader header;
        size_t header_received = 0;
        int retry_count = 0;
        const int max_retries = 20;
        
        while (header_received < sizeof(header)) {
            int ret = socket_->recv(reinterpret_cast<char*>(&header) + header_received, sizeof(header) - header_received, 0);
            if (ret < 0) {
                if (errno == EINPROGRESS || errno == EAGAIN || errno == EWOULDBLOCK) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    retry_count++;
                    if (retry_count >= max_retries) {
                        return false;
                    }
                    continue;
                } else {
                    ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to receive header";
                    return false;
                }
            } else if (ret == 0) {
                ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Connection closed by server";
                return false;
            }
            header_received += ret;
            retry_count = 0;
        }

        header.msg_id = ancfl::byteswapOnLittleEndian(header.msg_id);
        header.msg_len = ancfl::byteswapOnLittleEndian(header.msg_len);
        header.target_id = ancfl::byteswapOnLittleEndian(header.target_id);
        header.user_data = ancfl::byteswapOnLittleEndian(header.user_data);

        std::string data;
        uint32_t body_len = header.msg_len - sizeof(MessageHeader);
        
        if (body_len > 0 && body_len < 1024 * 1024) {
            data.resize(body_len);
            size_t data_received = 0;
            retry_count = 0;
            
            while (data_received < body_len) {
                size_t chunk_size = std::min<size_t>(body_len - data_received, 1024);
                int ret = socket_->recv(&data[0] + data_received, chunk_size, 0);
                if (ret < 0) {
                    if (errno == EINPROGRESS || errno == EAGAIN || errno == EWOULDBLOCK) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        retry_count++;
                        if (retry_count >= max_retries) {
                            return false;
                        }
                        continue;
                    } else {
                        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to receive data";
                        return false;
                    }
                } else if (ret == 0) {
                    ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Connection closed by server";
                    return false;
                }
                data_received += ret;
                retry_count = 0;
                
                if (data_received >= body_len) {
                    break;
                }
            }
        } else if (body_len > 0) {
            return false;
        }

        packet.conn_id = 0;
        packet.msg_id = header.msg_id;
        packet.target_id = header.target_id;
        packet.user_data = header.user_data;
        
        auto message = CreateMessage(header.msg_id);
        if (message && message->ParseFromString(data)) {
            packet.msg = message;
        }

        ANCFL_LOG_DEBUG(ANCFL_LOG_ROOT()) << "Received message: msg_id=" << header.msg_id << ", len=" << header.msg_len;
        return true;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "RecvMessage exception: " << e.what();
        return false;
    }
}

bool TcpClient::IsConnected() const {
    return connected_;
}

void TcpClient::SetMessageDispatcher(MessageDispatcher::ptr dispatcher) {
    dispatcher_ = dispatcher;
}

void TcpClient::HandleRecv() {
    while (connected_) {
        NetPacket packet;
        if (RecvMessage(packet)) {
            if (dispatcher_) {
                dispatcher_->Dispatch(packet);
            }
        } else {
            Disconnect();
            break;
        }
    }
}

bool TcpClient::ParseMessage(const char* data, uint32_t len, NetPacket& packet) {
    return true;
}

bool TcpClient::SerializeMessage(const google::protobuf::Message& msg, std::string& out_data) {
    return msg.SerializeToString(&out_data);
}

std::shared_ptr<google::protobuf::Message> TcpClient::CreateMessage(uint32_t msg_id) {
    switch (static_cast<MessageID>(msg_id)) {
        case MessageID::MSG_ACCOUNT_LOGIN_ACK:
            return std::make_shared<AccountLoginAck>();
        default:
            return nullptr;
    }
}

}  // namespace game_server
