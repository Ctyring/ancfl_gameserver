#include "logic_service.h"
#include "proto/msg_role.pb.h"
#include "proto/msg_base.pb.h"
#include "proto/msg_id.pb.h"

namespace game_server {

LogicService::LogicService() : GameServiceBase("LogicService"),
    db_server_id_(0),
    db_server_port_(0),
    sync_timer_(0) {
}

LogicService::~LogicService() {
}

bool LogicService::InitService() {
    // 初始化服务
    if (!GameServiceBase::InitService()) {
        return false;
    }
    
    // 初始化共享内存
    if (!InitSharedMemory()) {
        LOG_ERROR("Failed to init shared memory");
        return false;
    }
    
    // 连接数据库服务器
    if (!ConnectToDBServer()) {
        LOG_ERROR("Failed to connect to DB server");
        return false;
    }
    
    // 注册消息处理器
    RegisterAllHandlers();
    
    // 设置同步定时器
    sync_timer_ = GetTimerMgr()->AddTimer(60000, std::bind(&LogicService::OnTimer, this));
    
    LOG_INFO("LogicService initialized successfully");
    return true;
}

void LogicService::UninitService() {
    // 清理共享内存
    role_memory_.reset();
    
    // 清理角色缓存
    role_cache_.clear();
    
    // 清理定时器
    if (sync_timer_ > 0) {
        GetTimerMgr()->CancelTimer(sync_timer_);
        sync_timer_ = 0;
    }
    
    GameServiceBase::UninitService();
    LOG_INFO("LogicService uninitialized");
}

void LogicService::RegisterAllHandlers() {
    // 注册消息处理器
    RegisterHandler(static_cast<uint32_t>(MessageID::MSG_ROLE_CREATE_REQ), std::bind(&LogicService::OnRoleCreateReq, this, std::placeholders::_1));
    RegisterHandler(static_cast<uint32_t>(MessageID::MSG_ROLE_LOGIN_REQ), std::bind(&LogicService::OnRoleLoginReq, this, std::placeholders::_1));
    RegisterHandler(static_cast<uint32_t>(MessageID::MSG_ROLE_LOGOUT_REQ), std::bind(&LogicService::OnRoleLogoutReq, this, std::placeholders::_1));
    RegisterHandler(static_cast<uint32_t>(MessageID::MSG_ROLE_LIST_REQ), std::bind(&LogicService::OnRoleListReq, this, std::placeholders::_1));
    RegisterHandler(static_cast<uint32_t>(MessageID::MSG_ROLE_DELETE_REQ), std::bind(&LogicService::OnRoleDeleteReq, this, std::placeholders::_1));
    RegisterHandler(static_cast<uint32_t>(MessageID::MSG_HEART_BEAT_REQ), std::bind(&LogicService::OnHeartBeatReq, this, std::placeholders::_1));
    RegisterHandler(static_cast<uint32_t>(MessageID::MSG_DB_REG_TO_LOGIC_REQ), std::bind(&LogicService::OnDBRegToLogicReq, this, std::placeholders::_1));
    RegisterHandler(static_cast<uint32_t>(MessageID::MSG_DB_DATA_SYNC_ACK), std::bind(&LogicService::OnDBDataSyncAck, this, std::placeholders::_1));
}

void LogicService::OnTimer() {
    // 同步数据到数据库
    for (auto& pair : role_cache_) {
        RoleData* data = pair.second;
        if (data->GetStatus() == SharedMemoryStatus::USE) {
            SaveRoleData(*data);
        }
    }
}

bool LogicService::ConnectToDBServer() {
    // 从配置文件读取数据库服务器信息
    auto config = GetConfig();
    if (config) {
        db_server_ip_ = config->GetString("db_server.ip", "127.0.0.1");
        db_server_port_ = config->GetInt32("db_server.port", 8003);
    }
    
    // 连接数据库服务器
    uint32_t conn_id = Connect(db_server_ip_, db_server_port_);
    if (conn_id == 0) {
        LOG_ERROR("Failed to connect to DB server: %s:%d", db_server_ip_.c_str(), db_server_port_);
        return false;
    }
    
    db_server_id_ = conn_id;
    LOG_INFO("Connected to DB server: %s:%d, conn_id: %u", db_server_ip_.c_str(), db_server_port_, conn_id);
    return true;
}

bool LogicService::CreateRole(uint64_t account_id, const std::string& role_name, int32_t职业, int32_t gender) {
    // 分配角色数据
    RoleData* data = AllocateRoleData();
    if (!data) {
        LOG_ERROR("Failed to allocate role data");
        return false;
    }
    
    // 初始化角色数据
    data->role_id = GetNextRoleId();
    data->account_id = account_id;
    data->role_name = role_name;
    data->level = 1;
    data->exp = 0;
    data->gold = 10000;
    data->diamond = 1000;
    data->职业 =职业;
    data->gender = gender;
    data->create_time = time(nullptr);
    data->last_login_time = data->create_time;
    data->last_logout_time = 0;
    data->online_time = 0;
    data->vip_level = 0;
    data->vip_exp = 0;
    data->体力 = 100;
    data->精力 = 100;
    data->声望 = 0;
    data->荣誉 = 0;
    data->战功 = 0;
    data->成就 = 0;
    data->战斗力 = 1000;
    data->当前场景 = 1001;
    data->position_x = 0.0f;
    data->position_y = 0.0f;
    data->position_z = 0.0f;
    data->rotation_y = 0.0f;
    
    // 保存到缓存
    role_cache_[data->role_id] = data;
    
    // 保存到数据库
    SaveRoleData(*data);
    
    LOG_INFO("Created role: id=%llu, name=%s, account_id=%llu", data->role_id, role_name.c_str(), account_id);
    return true;
}

bool LogicService::LoadRoleData(uint64_t role_id, RoleData& data) {
    // 从缓存中查找
    auto it = role_cache_.find(role_id);
    if (it != role_cache_.end()) {
        data = *it->second;
        return true;
    }
    
    // 从数据库加载
    // TODO: 实现从数据库加载角色数据
    
    return false;
}

bool LogicService::SaveRoleData(const RoleData& data) {
    // 发送数据到数据库服务器
    msg_role::RoleDataSyncReq req;
    req.set_role_id(data.role_id);
    req.set_account_id(data.account_id);
    req.set_role_name(data.role_name);
    req.set_level(data.level);
    req.set_exp(data.exp);
    req.set_gold(data.gold);
    req.set_diamond(data.diamond);
    req.set_职业(data.职业);
    req.set_gender(data.gender);
    req.set_create_time(data.create_time);
    req.set_last_login_time(data.last_login_time);
    req.set_last_logout_time(data.last_logout_time);
    req.set_online_time(data.online_time);
    req.set_vip_level(data.vip_level);
    req.set_vip_exp(data.vip_exp);
    req.set_体力(data.体力);
    req.set_精力(data.精力);
    req.set_声望(data.声望);
    req.set_荣誉(data.荣誉);
    req.set_战功(data.战功);
    req.set_成就(data.成就);
    req.set_战斗力(data.战斗力);
    req.set_当前场景(data.当前场景);
    req.set_position_x(data.position_x);
    req.set_position_y(data.position_y);
    req.set_position_z(data.position_z);
    req.set_rotation_y(data.rotation_y);
    
    SendMsgToServer(db_server_id_, static_cast<uint32_t>(MessageID::MSG_DB_DATA_SYNC_REQ), req);
    return true;
}

bool LogicService::DeleteRole(uint64_t role_id) {
    // 从缓存中删除
    auto it = role_cache_.find(role_id);
    if (it != role_cache_.end()) {
        FreeRoleData(it->second);
        role_cache_.erase(it);
    }
    
    // 发送删除请求到数据库服务器
    msg_role::RoleDeleteReq req;
    req.set_role_id(role_id);
    SendMsgToServer(db_server_id_, static_cast<uint32_t>(MessageID::MSG_ROLE_DELETE_REQ), req);
    
    return true;
}

bool LogicService::GetRoleList(uint64_t account_id, std::vector<RoleData>& roles) {
    // 从数据库获取角色列表
    msg_role::RoleListReq req;
    req.set_account_id(account_id);
    SendMsgToServer(db_server_id_, static_cast<uint32_t>(MessageID::MSG_ROLE_LIST_REQ), req);
    
    // TODO: 实现异步处理角色列表响应
    return true;
}

bool LogicService::InitSharedMemory() {
    // 初始化角色共享内存
    role_memory_ = std::make_unique<SharedMemory<RoleData>>(1, 1000);
    if (!role_memory_) {
        LOG_ERROR("Failed to create role shared memory");
        return false;
    }
    
    LOG_INFO("Shared memory initialized: block_count=%d", role_memory_->GetBlockCount());
    return true;
}

RoleData* LogicService::AllocateRoleData() {
    if (!role_memory_) {
        return nullptr;
    }
    return role_memory_->Allocate();
}

void LogicService::FreeRoleData(RoleData* data) {
    if (role_memory_ && data) {
        role_memory_->Free(data);
    }
}

bool LogicService::OnRoleCreateReq(const NetPacket& packet) {
    msg_role::RoleCreateReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }
    
    // 创建角色
    bool success = CreateRole(req.account_id(), req.role_name(), req.职业(), req.gender());
    
    // 发送响应
    msg_role::RoleCreateAck ack;
    ack.set_result(success ? 0 : 1);
    if (success) {
        // TODO: 设置角色ID
    }
    SendMsgToClient(packet.conn_id, static_cast<uint32_t>(MessageID::MSG_ROLE_CREATE_ACK), ack);
    
    return true;
}

bool LogicService::OnRoleLoginReq(const NetPacket& packet) {
    msg_role::RoleLoginReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }
    
    // 加载角色数据
    RoleData data;
    bool success = LoadRoleData(req.role_id(), data);
    
    // 发送响应
    msg_role::RoleLoginAck ack;
    ack.set_result(success ? 0 : 1);
    if (success) {
        // TODO: 设置角色数据
    }
    SendMsgToClient(packet.conn_id, static_cast<uint32_t>(MessageID::MSG_ROLE_LOGIN_ACK), ack);
    
    return true;
}

bool LogicService::OnRoleLogoutReq(const NetPacket& packet) {
    msg_role::RoleLogoutReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }
    
    // 处理角色登出
    // TODO: 实现登出逻辑
    
    // 发送响应
    msg_role::RoleLogoutAck ack;
    ack.set_result(0);
    SendMsgToClient(packet.conn_id, static_cast<uint32_t>(MessageID::MSG_ROLE_LOGOUT_ACK), ack);
    
    return true;
}

bool LogicService::OnRoleListReq(const NetPacket& packet) {
    msg_role::RoleListReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }
    
    // 获取角色列表
    std::vector<RoleData> roles;
    GetRoleList(req.account_id(), roles);
    
    // 发送响应
    msg_role::RoleListAck ack;
    ack.set_result(0);
    // TODO: 设置角色列表
    SendMsgToClient(packet.conn_id, static_cast<uint32_t>(MessageID::MSG_ROLE_LIST_ACK), ack);
    
    return true;
}

bool LogicService::OnRoleDeleteReq(const NetPacket& packet) {
    msg_role::RoleDeleteReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }
    
    // 删除角色
    bool success = DeleteRole(req.role_id());
    
    // 发送响应
    msg_role::RoleDeleteAck ack;
    ack.set_result(success ? 0 : 1);
    SendMsgToClient(packet.conn_id, static_cast<uint32_t>(MessageID::MSG_ROLE_DELETE_ACK), ack);
    
    return true;
}

bool LogicService::OnHeartBeatReq(const NetPacket& packet) {
    msg_base::HeartBeatAck ack;
    ack.set_timestamp(time(nullptr));
    SendMsgToClient(packet.conn_id, static_cast<uint32_t>(MessageID::MSG_HEART_BEAT_ACK), ack);
    return true;
}

bool LogicService::OnDBRegToLogicReq(const NetPacket& packet) {
    // 处理数据库服务器注册
    msg_base::ServerRegReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }
    
    db_server_id_ = packet.conn_id;
    LOG_INFO("DB server registered: conn_id=%u", db_server_id_);
    
    // 发送注册响应
    msg_base::ServerRegAck ack;
    ack.set_result(0);
    SendMsgToServer(db_server_id_, static_cast<uint32_t>(MessageID::MSG_SERVER_REG_ACK), ack);
    
    return true;
}

bool LogicService::OnDBDataSyncAck(const NetPacket& packet) {
    // 处理数据库同步响应
    msg_role::RoleDataSyncAck ack;
    if (!DecodePacket(packet, ack)) {
        return false;
    }
    
    LOG_INFO("DB data sync ack: role_id=%llu, result=%d", ack.role_id(), ack.result());
    
    return true;
}

uint64_t LogicService::GetNextRoleId() {
    // 生成角色ID
    static uint64_t next_id = 100000;
    return next_id++;
}

} // namespace game_server
