#include "db_service.h"
#include "ancfl/crypto.h"
#include "proto/msg_account.pb.h"
#include "proto/msg_base.pb.h"
#include "proto/msg_id.pb.h"
#include "proto/msg_role.pb.h"

namespace game_server {

DBService::DBService()
    : GameServiceBase("DBService"),
      db_port_(3306),
      db_pool_size_(5),
      sync_timer_(0) {}

DBService::~DBService() {}

bool DBService::InitService() {
    // 初始化服务
    if (!GameServiceBase::InitService()) {
        return false;
    }

    // 加载配置
    auto config = GetConfig();
    if (config) {
        db_host_ = config->GetString("database.host", "127.0.0.1");
        db_port_ = config->GetInt32("database.port", 3306);
        db_user_ = config->GetString("database.user", "root");
        db_password_ = config->GetString("database.password", "");
        db_name_ = config->GetString("database.name", "game_server");
        db_pool_size_ = config->GetInt32("database.pool_size", 5);
    }

    // 连接数据库
    if (!ConnectToDatabase()) {
        LOG_ERROR("Failed to connect to database");
        return false;
    }

    // 注册消息处理器
    RegisterAllHandlers();

    // 设置同步定时器
    sync_timer_ =
        GetTimerMgr()->AddTimer(60000, std::bind(&DBService::OnTimer, this));

    LOG_INFO("DBService initialized successfully");
    return true;
}

void DBService::UninitService() {
    // 清理数据库连接池
    db_pool_.clear();

    // 清理逻辑服务器连接
    logic_servers_.clear();

    // 清理定时器
    if (sync_timer_ > 0) {
        GetTimerMgr()->CancelTimer(sync_timer_);
        sync_timer_ = 0;
    }

    GameServiceBase::UninitService();
    LOG_INFO("DBService uninitialized");
}

void DBService::RegisterAllHandlers() {
    // 注册消息处理器
    RegisterHandler(
        static_cast<uint32_t>(MessageID::MSG_DB_DATA_SYNC_REQ),
        std::bind(&DBService::OnDBDataSyncReq, this, std::placeholders::_1));
    RegisterHandler(
        static_cast<uint32_t>(MessageID::MSG_ROLE_LIST_REQ),
        std::bind(&DBService::OnRoleListReq, this, std::placeholders::_1));
    RegisterHandler(
        static_cast<uint32_t>(MessageID::MSG_ROLE_DELETE_REQ),
        std::bind(&DBService::OnRoleDeleteReq, this, std::placeholders::_1));
    RegisterHandler(
        static_cast<uint32_t>(MessageID::MSG_ACCOUNT_CREATE_REQ),
        std::bind(&DBService::OnAccountCreateReq, this, std::placeholders::_1));
    RegisterHandler(
        static_cast<uint32_t>(MessageID::MSG_ACCOUNT_VERIFY_REQ),
        std::bind(&DBService::OnAccountVerifyReq, this, std::placeholders::_1));
    RegisterHandler(static_cast<uint32_t>(MessageID::MSG_ACCOUNT_GET_INFO_REQ),
                    std::bind(&DBService::OnAccountGetInfoReq, this,
                              std::placeholders::_1));
    RegisterHandler(
        static_cast<uint32_t>(MessageID::MSG_ACCOUNT_SEAL_REQ),
        std::bind(&DBService::OnAccountSealReq, this, std::placeholders::_1));
    RegisterHandler(
        static_cast<uint32_t>(MessageID::MSG_ACCOUNT_UNSEAL_REQ),
        std::bind(&DBService::OnAccountUnsealReq, this, std::placeholders::_1));
    RegisterHandler(static_cast<uint32_t>(MessageID::MSG_ACCOUNT_IS_SEALED_REQ),
                    std::bind(&DBService::OnAccountIsSealedReq, this,
                              std::placeholders::_1));
    RegisterHandler(
        static_cast<uint32_t>(MessageID::MSG_LOGIN_LOG_REQ),
        std::bind(&DBService::OnLoginLogReq, this, std::placeholders::_1));
    RegisterHandler(
        static_cast<uint32_t>(MessageID::MSG_HEART_BEAT_REQ),
        std::bind(&DBService::OnHeartBeatReq, this, std::placeholders::_1));
    RegisterHandler(
        static_cast<uint32_t>(MessageID::MSG_LOGIC_REG_TO_DB_REQ),
        std::bind(&DBService::OnLogicRegToDBReq, this, std::placeholders::_1));
}

void DBService::OnTimer() {
    // 检查数据库连接
    for (auto& conn : db_pool_) {
        if (!conn->Ping()) {
            LOG_WARN("Database connection lost, reconnecting...");
            conn->Connect();
        }
    }
}

bool DBService::ConnectToDatabase() {
    // 创建数据库连接池
    for (int i = 0; i < db_pool_size_; ++i) {
        auto conn = std::make_shared<ancfl::MySQL>();
        if (!conn->Connect(db_host_, db_user_, db_password_, db_name_,
                           db_port_)) {
            LOG_ERROR("Failed to connect to database: %s",
                      conn->GetError().c_str());
            return false;
        }
        db_pool_.push_back(conn);
    }

    LOG_INFO("Connected to database: %s:%d, pool_size=%d", db_host_.c_str(),
             db_port_, db_pool_size_);
    return true;
}

std::shared_ptr<ancfl::MySQL> DBService::GetDBConnection() {
    std::lock_guard<std::mutex> lock(db_pool_mutex_);
    if (db_pool_.empty()) {
        LOG_ERROR("No database connection available");
        return nullptr;
    }
    return db_pool_[rand() % db_pool_.size()];
}

void DBService::ReleaseDBConnection(std::shared_ptr<ancfl::MySQL> conn) {
    // 连接池管理，这里简单处理
}

bool DBService::CreateRole(const msg_role::RoleDataSyncReq& req) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    std::string sql =
        "INSERT INTO role_base (role_id, account_id, role_name, level, exp, "
        "gold, diamond, 职业, gender, create_time, last_login_time, "
        "last_logout_time, online_time, vip_level, vip_exp, 体力, 精力, 声望, "
        "荣誉, 战功, 成就, 战斗力, 当前场景, position_x, position_y, "
        "position_z, rotation_y) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
        "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    try {
        auto stmt = conn->Prepare(sql);
        if (!stmt) {
            LOG_ERROR("Failed to prepare statement: %s",
                      conn->GetError().c_str());
            return false;
        }

        stmt->SetUInt64(1, req.role_id());
        stmt->SetUInt64(2, req.account_id());
        stmt->SetString(3, req.role_name());
        stmt->SetInt32(4, req.level());
        stmt->SetInt32(5, req.exp());
        stmt->SetInt32(6, req.gold());
        stmt->SetInt32(7, req.diamond());
        stmt->SetInt32(8, req.职业());
        stmt->SetInt32(9, req.gender());
        stmt->SetInt32(10, req.create_time());
        stmt->SetInt32(11, req.last_login_time());
        stmt->SetInt32(12, req.last_logout_time());
        stmt->SetInt32(13, req.online_time());
        stmt->SetInt32(14, req.vip_level());
        stmt->SetInt32(15, req.vip_exp());
        stmt->SetInt32(16, req.体力());
        stmt->SetInt32(17, req.精力());
        stmt->SetInt32(18, req.声望());
        stmt->SetInt32(19, req.荣誉());
        stmt->SetInt32(20, req.战功());
        stmt->SetInt32(21, req.成就());
        stmt->SetInt32(22, req.战斗力());
        stmt->SetInt32(23, req.当前场景());
        stmt->SetFloat(24, req.position_x());
        stmt->SetFloat(25, req.position_y());
        stmt->SetFloat(26, req.position_z());
        stmt->SetFloat(27, req.rotation_y());

        if (!stmt->Execute()) {
            LOG_ERROR("Failed to execute statement: %s",
                      conn->GetError().c_str());
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in CreateRole: %s", e.what());
        return false;
    }
}

bool DBService::UpdateRole(const msg_role::RoleDataSyncReq& req) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    std::string sql =
        "UPDATE role_base SET account_id=?, role_name=?, level=?, exp=?, "
        "gold=?, diamond=?, 职业=?, gender=?, last_login_time=?, "
        "last_logout_time=?, online_time=?, vip_level=?, vip_exp=?, 体力=?, "
        "精力=?, 声望=?, 荣誉=?, 战功=?, 成就=?, 战斗力=?, 当前场景=?, "
        "position_x=?, position_y=?, position_z=?, rotation_y=? WHERE "
        "role_id=?";

    try {
        auto stmt = conn->Prepare(sql);
        if (!stmt) {
            LOG_ERROR("Failed to prepare statement: %s",
                      conn->GetError().c_str());
            return false;
        }

        stmt->SetUInt64(1, req.account_id());
        stmt->SetString(2, req.role_name());
        stmt->SetInt32(3, req.level());
        stmt->SetInt32(4, req.exp());
        stmt->SetInt32(5, req.gold());
        stmt->SetInt32(6, req.diamond());
        stmt->SetInt32(7, req.职业());
        stmt->SetInt32(8, req.gender());
        stmt->SetInt32(9, req.last_login_time());
        stmt->SetInt32(10, req.last_logout_time());
        stmt->SetInt32(11, req.online_time());
        stmt->SetInt32(12, req.vip_level());
        stmt->SetInt32(13, req.vip_exp());
        stmt->SetInt32(14, req.体力());
        stmt->SetInt32(15, req.精力());
        stmt->SetInt32(16, req.声望());
        stmt->SetInt32(17, req.荣誉());
        stmt->SetInt32(18, req.战功());
        stmt->SetInt32(19, req.成就());
        stmt->SetInt32(20, req.战斗力());
        stmt->SetInt32(21, req.当前场景());
        stmt->SetFloat(22, req.position_x());
        stmt->SetFloat(23, req.position_y());
        stmt->SetFloat(24, req.position_z());
        stmt->SetFloat(25, req.rotation_y());
        stmt->SetUInt64(26, req.role_id());

        if (!stmt->Execute()) {
            LOG_ERROR("Failed to execute statement: %s",
                      conn->GetError().c_str());
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in UpdateRole: %s", e.what());
        return false;
    }
}

bool DBService::DeleteRole(uint64_t role_id) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    std::string sql = "DELETE FROM role_base WHERE role_id=?";

    try {
        auto stmt = conn->Prepare(sql);
        if (!stmt) {
            LOG_ERROR("Failed to prepare statement: %s",
                      conn->GetError().c_str());
            return false;
        }

        stmt->SetUInt64(1, role_id);

        if (!stmt->Execute()) {
            LOG_ERROR("Failed to execute statement: %s",
                      conn->GetError().c_str());
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in DeleteRole: %s", e.what());
        return false;
    }
}

bool DBService::GetRoleList(uint64_t account_id,
                            std::vector<msg_role::RoleInfo>& roles) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    std::string sql =
        "SELECT role_id, role_name, level, 职业, gender, create_time FROM "
        "role_base WHERE account_id=?";

    try {
        auto stmt = conn->Prepare(sql);
        if (!stmt) {
            LOG_ERROR("Failed to prepare statement: %s",
                      conn->GetError().c_str());
            return false;
        }

        stmt->SetUInt64(1, account_id);

        auto result = stmt->Query();
        if (!result) {
            LOG_ERROR("Failed to query: %s", conn->GetError().c_str());
            return false;
        }

        while (result->Next()) {
            msg_role::RoleBaseInfo info;
            info.set_role_id(result->GetUInt64(0));
            info.set_role_name(result->GetString(1));
            info.set_level(result->GetInt32(2));
            info.set_career(result->GetInt32(3));
            info.set_gender(result->GetInt32(4));
            info.set_create_time(result->GetInt32(5));
            roles.push_back(info);
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in GetRoleList: %s", e.what());
        return false;
    }
}

bool DBService::GetRoleData(uint64_t role_id, msg_role::RoleDataSyncReq& data) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    std::string sql = "SELECT * FROM role_base WHERE role_id=?";

    try {
        auto stmt = conn->Prepare(sql);
        if (!stmt) {
            LOG_ERROR("Failed to prepare statement: %s",
                      conn->GetError().c_str());
            return false;
        }

        stmt->SetUInt64(1, role_id);

        auto result = stmt->Query();
        if (!result) {
            LOG_ERROR("Failed to query: %s", conn->GetError().c_str());
            return false;
        }

        if (result->Next()) {
            data.set_role_id(result->GetUInt64(0));
            data.set_account_id(result->GetUInt64(1));
            data.set_role_name(result->GetString(2));
            data.set_level(result->GetInt32(3));
            data.set_exp(result->GetInt32(4));
            data.set_gold(result->GetInt32(5));
            data.set_diamond(result->GetInt32(6));
            data.set_job(result->GetInt32(7));
            data.set_gender(result->GetInt32(8));
            data.set_create_time(result->GetInt32(9));
            data.set_last_login_time(result->GetInt32(10));
            data.set_last_logout_time(result->GetInt32(11));
            data.set_online_time(result->GetInt32(12));
            data.set_vip_level(result->GetInt32(13));
            data.set_vip_exp(result->GetInt32(14));
            data.set_stamina(result->GetInt32(15));
            data.set_energy(result->GetInt32(16));
            data.set_reputation(result->GetInt32(17));
            data.set_honor(result->GetInt32(18));
            data.set_war_credit(result->GetInt32(19));
            data.set_achievement(result->GetInt32(20));
            data.set_fight_power(result->GetInt32(21));
            data.set_current_scene(result->GetInt32(22));
            data.set_position_x(result->GetFloat(23));
            data.set_position_y(result->GetFloat(24));
            data.set_position_z(result->GetFloat(25));
            data.set_rotation_y(result->GetFloat(26));
            return true;
        }

        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in GetRoleData: %s", e.what());
        return false;
    }
}

bool DBService::CreateAccount(const std::string& account_name,
                              const std::string& password,
                              int32_t channel,
                              uint64_t& account_id) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    // 生成账号ID
    account_id = time(nullptr) * 10000 + rand() % 10000;

    // 加密密码
    std::string encrypted_password = ancfl::Crypto::MD5(password);

    std::string sql =
        "INSERT INTO account (account_id, account_name, password, channel, "
        "create_time, last_login_time, last_login_ip, is_sealed, "
        "seal_end_time, review) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    try {
        auto stmt = conn->Prepare(sql);
        if (!stmt) {
            LOG_ERROR("Failed to prepare statement: %s",
                      conn->GetError().c_str());
            return false;
        }

        int64_t now = time(nullptr);
        stmt->SetUInt64(1, account_id);
        stmt->SetString(2, account_name);
        stmt->SetString(3, encrypted_password);
        stmt->SetInt32(4, channel);
        stmt->SetInt64(5, now);
        stmt->SetInt64(6, now);
        stmt->SetInt32(7, 0);
        stmt->SetInt32(8, 0);
        stmt->SetInt64(9, 0);
        stmt->SetInt32(10, 0);

        if (!stmt->Execute()) {
            LOG_ERROR("Failed to execute statement: %s",
                      conn->GetError().c_str());
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in CreateAccount: %s", e.what());
        return false;
    }
}

bool DBService::VerifyAccount(const std::string& account_name,
                              const std::string& password,
                              msg_account::AccountInfo& info) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    // 加密密码
    std::string encrypted_password = ancfl::Crypto::MD5(password);

    std::string sql =
        "SELECT account_id, account_name, channel, create_time, "
        "last_login_time, last_login_ip, is_sealed, seal_end_time, review FROM "
        "account WHERE account_name=? AND password=?";

    try {
        auto stmt = conn->Prepare(sql);
        if (!stmt) {
            LOG_ERROR("Failed to prepare statement: %s",
                      conn->GetError().c_str());
            return false;
        }

        stmt->SetString(1, account_name);
        stmt->SetString(2, encrypted_password);

        auto result = stmt->Query();
        if (!result) {
            LOG_ERROR("Failed to query: %s", conn->GetError().c_str());
            return false;
        }

        if (result->Next()) {
            info.set_account_id(result->GetUInt64(0));
            info.set_account_name(result->GetString(1));
            info.set_channel(result->GetInt32(2));
            info.set_create_time(result->GetInt64(3));
            info.set_last_login_time(result->GetInt64(4));
            info.set_last_login_ip(result->GetInt32(5));
            info.set_is_sealed(result->GetInt32(6) != 0);
            info.set_seal_end_time(result->GetInt64(7));
            info.set_review(result->GetInt32(8) != 0);
            return true;
        }

        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in VerifyAccount: %s", e.what());
        return false;
    }
}

bool DBService::GetAccountInfo(uint64_t account_id,
                               msg_account::AccountInfo& info) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    std::string sql =
        "SELECT account_id, account_name, channel, create_time, "
        "last_login_time, last_login_ip, is_sealed, seal_end_time, review FROM "
        "account WHERE account_id=?";

    try {
        auto stmt = conn->Prepare(sql);
        if (!stmt) {
            LOG_ERROR("Failed to prepare statement: %s",
                      conn->GetError().c_str());
            return false;
        }

        stmt->SetUInt64(1, account_id);

        auto result = stmt->Query();
        if (!result) {
            LOG_ERROR("Failed to query: %s", conn->GetError().c_str());
            return false;
        }

        if (result->Next()) {
            info.set_account_id(result->GetUInt64(0));
            info.set_account_name(result->GetString(1));
            info.set_channel(result->GetInt32(2));
            info.set_create_time(result->GetInt64(3));
            info.set_last_login_time(result->GetInt64(4));
            info.set_last_login_ip(result->GetInt32(5));
            info.set_is_sealed(result->GetInt32(6) != 0);
            info.set_seal_end_time(result->GetInt64(7));
            info.set_review(result->GetInt32(8) != 0);
            return true;
        }

        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in GetAccountInfo: %s", e.what());
        return false;
    }
}

bool DBService::SealAccount(uint64_t account_id, int32_t seal_time) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    int64_t seal_end_time = time(nullptr) + seal_time * 3600;

    std::string sql =
        "UPDATE account SET is_sealed=1, seal_end_time=? WHERE account_id=?";

    try {
        auto stmt = conn->Prepare(sql);
        if (!stmt) {
            LOG_ERROR("Failed to prepare statement: %s",
                      conn->GetError().c_str());
            return false;
        }

        stmt->SetInt64(1, seal_end_time);
        stmt->SetUInt64(2, account_id);

        if (!stmt->Execute()) {
            LOG_ERROR("Failed to execute statement: %s",
                      conn->GetError().c_str());
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in SealAccount: %s", e.what());
        return false;
    }
}

bool DBService::UnsealAccount(uint64_t account_id) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    std::string sql =
        "UPDATE account SET is_sealed=0, seal_end_time=0 WHERE account_id=?";

    try {
        auto stmt = conn->Prepare(sql);
        if (!stmt) {
            LOG_ERROR("Failed to prepare statement: %s",
                      conn->GetError().c_str());
            return false;
        }

        stmt->SetUInt64(1, account_id);

        if (!stmt->Execute()) {
            LOG_ERROR("Failed to execute statement: %s",
                      conn->GetError().c_str());
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in UnsealAccount: %s", e.what());
        return false;
    }
}

bool DBService::IsAccountSealed(uint64_t account_id) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    std::string sql =
        "SELECT is_sealed, seal_end_time FROM account WHERE account_id=?";

    try {
        auto stmt = conn->Prepare(sql);
        if (!stmt) {
            LOG_ERROR("Failed to prepare statement: %s",
                      conn->GetError().c_str());
            return false;
        }

        stmt->SetUInt64(1, account_id);

        auto result = stmt->Query();
        if (!result) {
            LOG_ERROR("Failed to query: %s", conn->GetError().c_str());
            return false;
        }

        if (result->Next()) {
            int32_t is_sealed = result->GetInt32(0);
            int64_t seal_end_time = result->GetInt64(1);

            if (is_sealed && seal_end_time > time(nullptr)) {
                return true;
            }
        }

        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in IsAccountSealed: %s", e.what());
        return false;
    }
}

bool DBService::RecordLoginLog(uint64_t account_id,
                               int32_t channel,
                               const std::string& version,
                               const std::string& uuid,
                               const std::string& idfa,
                               const std::string& imodel,
                               const std::string& imei,
                               int32_t ip) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    std::string sql =
        "INSERT INTO account_login_log (account_id, channel, version, uuid, "
        "idfa, imodel, imei, ip, login_time) VALUES (?, ?, ?, ?, ?, ?, ?, ?, "
        "?)";

    try {
        auto stmt = conn->Prepare(sql);
        if (!stmt) {
            LOG_ERROR("Failed to prepare statement: %s",
                      conn->GetError().c_str());
            return false;
        }

        int64_t now = time(nullptr);
        stmt->SetUInt64(1, account_id);
        stmt->SetInt32(2, channel);
        stmt->SetString(3, version);
        stmt->SetString(4, uuid);
        stmt->SetString(5, idfa);
        stmt->SetString(6, imodel);
        stmt->SetString(7, imei);
        stmt->SetInt32(8, ip);
        stmt->SetInt64(9, now);

        if (!stmt->Execute()) {
            LOG_ERROR("Failed to execute statement: %s",
                      conn->GetError().c_str());
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in RecordLoginLog: %s", e.what());
        return false;
    }
}

bool DBService::OnDBDataSyncReq(const NetPacket& packet) {
    msg_role::RoleDataSyncReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }

    // 检查角色是否存在
    msg_role::RoleDataSyncReq existing_data;
    bool exists = GetRoleData(req.role_id(), existing_data);

    bool success = false;
    if (exists) {
        // 更新角色
        success = UpdateRole(req);
    } else {
        // 创建角色
        success = CreateRole(req);
    }

    // 发送响应
    msg_role::RoleDataSyncAck ack;
    ack.set_role_id(req.role_id());
    ack.set_result(success ? 0 : 1);
    SendMsgToServer(packet.conn_id,
                    static_cast<uint32_t>(MessageID::MSG_DB_DATA_SYNC_ACK),
                    ack);

    return true;
}

bool DBService::OnRoleListReq(const NetPacket& packet) {
    msg_role::RoleListReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }

    // 获取角色列表
    std::vector<msg_role::RoleBaseInfo> roles;
    bool success = GetRoleList(req.account_id(), roles);

    // 发送响应
    msg_role::RoleListAck ack;
    ack.set_result(success ? 0 : 1);
    for (auto& role : roles) {
        auto role_info = ack.add_roles();
        role_info->CopyFrom(role);
    }
    SendMsgToServer(packet.conn_id,
                    static_cast<uint32_t>(MessageID::MSG_ROLE_LIST_ACK), ack);

    return true;
}

bool DBService::OnRoleDeleteReq(const NetPacket& packet) {
    msg_role::RoleDeleteReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }

    // 删除角色
    bool success = DeleteRole(req.role_id());

    // 发送响应
    msg_role::RoleDeleteAck ack;
    ack.set_result(success ? 0 : 1);
    SendMsgToServer(packet.conn_id,
                    static_cast<uint32_t>(MessageID::MSG_ROLE_DELETE_ACK), ack);

    return true;
}

bool DBService::OnAccountCreateReq(const NetPacket& packet) {
    msg_account::AccountCreateReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }

    // 创建账号
    uint64_t account_id = 0;
    bool success = CreateAccount(req.account_name(), req.password(),
                                 req.channel(), account_id);

    // 发送响应
    msg_account::AccountCreateAck ack;
    ack.set_result(success ? 0 : 1);
    if (success) {
        ack.set_account_id(account_id);
    }
    SendMsgToServer(packet.conn_id,
                    static_cast<uint32_t>(MessageID::MSG_ACCOUNT_CREATE_ACK),
                    ack);

    return true;
}

bool DBService::OnAccountVerifyReq(const NetPacket& packet) {
    msg_account::AccountVerifyReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }

    // 验证账号
    msg_account::AccountInfo info;
    bool success = VerifyAccount(req.account_name(), req.password(), info);

    // 发送响应
    msg_account::AccountVerifyAck ack;
    ack.set_result(success ? 0 : 1);
    if (success) {
        ack.mutable_info()->CopyFrom(info);
    }
    SendMsgToServer(packet.conn_id,
                    static_cast<uint32_t>(MessageID::MSG_ACCOUNT_VERIFY_ACK),
                    ack);

    return true;
}

bool DBService::OnAccountGetInfoReq(const NetPacket& packet) {
    msg_account::AccountGetInfoReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }

    // 获取账号信息
    msg_account::AccountInfo info;
    bool success = GetAccountInfo(req.account_id(), info);

    // 发送响应
    msg_account::AccountGetInfoAck ack;
    ack.set_result(success ? 0 : 1);
    if (success) {
        ack.mutable_info()->CopyFrom(info);
    }
    SendMsgToServer(packet.conn_id,
                    static_cast<uint32_t>(MessageID::MSG_ACCOUNT_GET_INFO_ACK),
                    ack);

    return true;
}

bool DBService::OnAccountSealReq(const NetPacket& packet) {
    msg_account::AccountSealReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }

    // 封号
    bool success = SealAccount(req.account_id(), req.seal_time());

    // 发送响应
    msg_account::AccountSealAck ack;
    ack.set_result(success ? 0 : 1);
    SendMsgToServer(packet.conn_id,
                    static_cast<uint32_t>(MessageID::MSG_ACCOUNT_SEAL_ACK),
                    ack);

    return true;
}

bool DBService::OnAccountUnsealReq(const NetPacket& packet) {
    msg_account::AccountUnsealReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }

    // 解封
    bool success = UnsealAccount(req.account_id());

    // 发送响应
    msg_account::AccountUnsealAck ack;
    ack.set_result(success ? 0 : 1);
    SendMsgToServer(packet.conn_id,
                    static_cast<uint32_t>(MessageID::MSG_ACCOUNT_UNSEAL_ACK),
                    ack);

    return true;
}

bool DBService::OnAccountIsSealedReq(const NetPacket& packet) {
    msg_account::AccountIsSealedReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }

    // 检查是否被封
    bool is_sealed = IsAccountSealed(req.account_id());

    // 发送响应
    msg_account::AccountIsSealedAck ack;
    ack.set_result(0);
    ack.set_is_sealed(is_sealed);
    SendMsgToServer(packet.conn_id,
                    static_cast<uint32_t>(MessageID::MSG_ACCOUNT_IS_SEALED_ACK),
                    ack);

    return true;
}

bool DBService::OnLoginLogReq(const NetPacket& packet) {
    msg_account::LoginLogReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }

    // 记录登录日志
    bool success = RecordLoginLog(req.account_id(), req.channel(),
                                  req.version(), req.uuid(), req.idfa(),
                                  req.imodel(), req.imei(), req.ip());

    // 发送响应
    msg_account::LoginLogAck ack;
    ack.set_result(success ? 0 : 1);
    SendMsgToServer(packet.conn_id,
                    static_cast<uint32_t>(MessageID::MSG_LOGIN_LOG_ACK), ack);

    return true;
}

bool DBService::OnHeartBeatReq(const NetPacket& packet) {
    msg_base::HeartBeatAck ack;
    ack.set_timestamp(time(nullptr));
    SendMsgToServer(packet.conn_id,
                    static_cast<uint32_t>(MessageID::MSG_HEART_BEAT_ACK), ack);
    return true;
}

bool DBService::OnLogicRegToDBReq(const NetPacket& packet) {
    // 处理逻辑服务器注册
    msg_base::ServerRegReq req;
    if (!DecodePacket(packet, req)) {
        return false;
    }

    logic_servers_[packet.conn_id] = req.server_name();
    LOG_INFO("Logic server registered: conn_id=%u, name=%s", packet.conn_id,
             req.server_name().c_str());

    // 发送注册响应
    msg_base::ServerRegAck ack;
    ack.set_result(0);
    SendMsgToServer(packet.conn_id,
                    static_cast<uint32_t>(MessageID::MSG_SERVER_REG_ACK), ack);

    return true;
}

}  // namespace game_server
