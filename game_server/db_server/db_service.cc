#include "db_service.h"
#include "ancfl/log.h"
#include <openssl/sha.h>
#include <iomanip>
#include <sstream>

namespace game_server {

// 简单的 SHA1 哈希函数
std::string Sha1Hash(const std::string& input) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), hash);
    
    std::stringstream ss;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

DBService::DBService()
    : GameServiceBase("DBService"),
      io_manager_(nullptr),
      worker_pool_(nullptr),
      db_port_(3306) {}

DBService::~DBService() {}

bool DBService::InitService() {
    db_host_ = "127.0.0.1";
    db_port_ = 3306;
    db_user_ = "root";
    db_password_ = "12345678";
    db_name_ = "game_server";

    if (!ConnectToDatabase()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to connect to database";
        return false;
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "DBService initialized successfully";
    return true;
}

void DBService::UninitService() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    db_connections_.clear();
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "DBService uninitialized";
}

void DBService::RegisterAllHandlers() {
    // TODO: 注册消息处理器
}

void DBService::OnTimer() {
    // TODO: 实现数据同步逻辑
}

bool DBService::ConnectToDatabase() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    
    for (int i = 0; i < 5; ++i) {
        std::map<std::string, std::string> params;
        params["host"] = db_host_;
        params["port"] = std::to_string(db_port_);
        params["user"] = db_user_;
        params["passwd"] = db_password_;
        params["dbname"] = db_name_;

        auto conn = std::make_shared<ancfl::MySQL>(params);
        if (!conn->connect()) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to connect to database";
            return false;
        }
        db_connections_.push_back(conn);
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Connected to database successfully";
    return true;
}

std::shared_ptr<ancfl::MySQL> DBService::GetDBConnection() {
    std::lock_guard<std::mutex> lock(db_mutex_);
    if (db_connections_.empty()) {
        return nullptr;
    }
    auto conn = db_connections_.back();
    db_connections_.pop_back();
    return conn;
}

void DBService::ReleaseDBConnection(std::shared_ptr<ancfl::MySQL> conn) {
    std::lock_guard<std::mutex> lock(db_mutex_);
    db_connections_.push_back(conn);
}

bool DBService::CreateRole(const RoleInfo& role_info) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    std::string sql =
        "INSERT INTO role_base (account_id, server_id, role_name, career, "
        "level, exp, head_id, portrait_frame, create_time, last_login_time, "
        "is_deleted, delete_time) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    try {
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            return false;
        }

        stmt->bindInt64(1, role_info.account_id);
        stmt->bindInt32(2, role_info.server_id);
        stmt->bindString(3, role_info.role_name);
        stmt->bindInt32(4, role_info.career);
        stmt->bindInt32(5, role_info.level);
        stmt->bindInt64(6, role_info.exp);
        stmt->bindInt32(7, role_info.head_id);
        stmt->bindInt32(8, role_info.portrait_frame);
        stmt->bindInt64(9, role_info.create_time);
        stmt->bindInt64(10, role_info.last_login_time);
        stmt->bindInt8(11, role_info.is_deleted);
        stmt->bindInt64(12, role_info.delete_time);

        if (!stmt->execute()) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to execute statement";
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in CreateRole: " << e.what();
        return false;
    }
}

bool DBService::UpdateRole(const RoleInfo& role_info) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    std::string sql =
        "UPDATE role_base SET account_id=?, server_id=?, role_name=?, "
        "career=?, level=?, exp=?, head_id=?, portrait_frame=?, "
        "last_login_time=?, is_deleted=?, delete_time=? "
        "WHERE role_id=?";

    try {
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            return false;
        }

        stmt->bindInt64(1, role_info.account_id);
        stmt->bindInt32(2, role_info.server_id);
        stmt->bindString(3, role_info.role_name);
        stmt->bindInt32(4, role_info.career);
        stmt->bindInt32(5, role_info.level);
        stmt->bindInt64(6, role_info.exp);
        stmt->bindInt32(7, role_info.head_id);
        stmt->bindInt32(8, role_info.portrait_frame);
        stmt->bindInt64(9, role_info.last_login_time);
        stmt->bindInt8(10, role_info.is_deleted);
        stmt->bindInt64(11, role_info.delete_time);
        stmt->bindInt64(12, role_info.role_id);

        if (!stmt->execute()) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to execute statement";
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in UpdateRole: " << e.what();
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
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            return false;
        }

        stmt->bindInt64(1, role_id);

        if (!stmt->execute()) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to execute statement";
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in DeleteRole: " << e.what();
        return false;
    }
}

bool DBService::GetRoleData(uint64_t role_id, RoleInfo& role_info) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    std::string sql =
        "SELECT role_id, account_id, server_id, role_name, career, "
        "level, exp, head_id, portrait_frame, create_time, last_login_time, "
        "is_deleted, delete_time FROM role_base WHERE role_id=?";

    try {
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            return false;
        }

        stmt->bindInt64(1, role_id);

        auto result = stmt->query();
        if (!result) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to query";
            return false;
        }

        if (result->next()) {
            role_info.role_id = result->getInt64(0);
            role_info.account_id = result->getInt64(1);
            role_info.server_id = result->getInt32(2);
            role_info.role_name = result->getString(3);
            role_info.career = result->getInt32(4);
            role_info.level = result->getInt32(5);
            role_info.exp = result->getInt64(6);
            role_info.head_id = result->getInt32(7);
            role_info.portrait_frame = result->getInt32(8);
            role_info.create_time = result->getInt64(9);
            role_info.last_login_time = result->getInt64(10);
            role_info.is_deleted = result->getInt8(11);
            role_info.delete_time = result->getInt64(12);
            return true;
        }

        return false;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in GetRoleData: " << e.what();
        return false;
    }
}

bool DBService::GetRoleList(uint64_t account_id, std::vector<RoleInfo>& roles) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    std::string sql =
        "SELECT role_id, account_id, server_id, role_name, career, "
        "level, exp, head_id, portrait_frame, create_time, last_login_time, "
        "is_deleted, delete_time FROM role_base WHERE account_id=?";

    try {
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            return false;
        }

        stmt->bindInt64(1, account_id);

        auto result = stmt->query();
        if (!result) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to query";
            return false;
        }

        roles.clear();
        while (result->next()) {
            RoleInfo role_info;
            role_info.role_id = result->getInt64(0);
            role_info.account_id = result->getInt64(1);
            role_info.server_id = result->getInt32(2);
            role_info.role_name = result->getString(3);
            role_info.career = result->getInt32(4);
            role_info.level = result->getInt32(5);
            role_info.exp = result->getInt64(6);
            role_info.head_id = result->getInt32(7);
            role_info.portrait_frame = result->getInt32(8);
            role_info.create_time = result->getInt64(9);
            role_info.last_login_time = result->getInt64(10);
            role_info.is_deleted = result->getInt8(11);
            role_info.delete_time = result->getInt64(12);
            roles.push_back(role_info);
        }

        return true;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in GetRoleList: " << e.what();
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

    std::string sql =
        "INSERT INTO account (account_name, password, channel, create_time, "
        "last_login_time, is_sealed, seal_end_time) VALUES (?, ?, ?, ?, ?, ?, ?)";

    try {
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            return false;
        }

        std::string encrypted_password = Sha1Hash(password);
        int64_t now = time(nullptr);

        stmt->bindString(1, account_name);
        stmt->bindString(2, encrypted_password);
        stmt->bindInt32(3, channel);
        stmt->bindInt64(4, now);
        stmt->bindInt64(5, now);
        stmt->bindInt32(6, 0);
        stmt->bindInt64(7, 0);

        if (!stmt->execute()) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to execute statement";
            return false;
        }

        account_id = conn->getLastInsertId();
        return true;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in CreateAccount: " << e.what();
        return false;
    }
}

bool DBService::VerifyAccount(const std::string& account_name,
                              const std::string& password,
                              uint64_t& account_id) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    std::string sql =
        "SELECT account_id, password FROM account WHERE account_name=?";

    try {
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            return false;
        }

        stmt->bindString(1, account_name);

        auto result = stmt->query();
        if (!result) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to query";
            return false;
        }

        if (result->next()) {
            account_id = result->getInt64(0);
            std::string stored_password = result->getString(1);
            std::string encrypted_password = Sha1Hash(password);
            return stored_password == encrypted_password;
        }

        return false;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in VerifyAccount: " << e.what();
        return false;
    }
}

bool DBService::GetAccountInfo(uint64_t account_id, AccountInfo& account_info) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    std::string sql =
        "SELECT account_id, account_name, channel, create_time, last_login_time, "
        "is_sealed, seal_end_time FROM account WHERE account_id=?";

    try {
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            return false;
        }

        stmt->bindInt64(1, account_id);

        auto result = stmt->query();
        if (!result) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to query";
            return false;
        }

        if (result->next()) {
            account_info.account_id = result->getInt64(0);
            account_info.account_name = result->getString(1);
            account_info.channel = result->getInt32(2);
            account_info.create_time = result->getInt64(3);
            account_info.last_login_time = result->getInt64(4);
            account_info.is_sealed = result->getInt32(5);
            account_info.seal_end_time = result->getInt64(6);
            return true;
        }

        return false;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in GetAccountInfo: " << e.what();
        return false;
    }
}

bool DBService::SealAccount(uint64_t account_id, int64_t seal_end_time) {
    auto conn = GetDBConnection();
    if (!conn) {
        return false;
    }

    std::string sql =
        "UPDATE account SET is_sealed=1, seal_end_time=? WHERE account_id=?";

    try {
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            return false;
        }

        stmt->bindInt64(1, seal_end_time);
        stmt->bindInt64(2, account_id);

        if (!stmt->execute()) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to execute statement";
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in SealAccount: " << e.what();
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
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            return false;
        }

        stmt->bindInt64(1, account_id);

        if (!stmt->execute()) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to execute statement";
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in UnsealAccount: " << e.what();
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
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            return false;
        }

        stmt->bindInt64(1, account_id);

        auto result = stmt->query();
        if (!result) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to query";
            return false;
        }

        if (result->next()) {
            int32_t is_sealed = result->getInt32(0);
            int64_t seal_end_time = result->getInt64(1);

            if (is_sealed && seal_end_time > time(nullptr)) {
                return true;
            }
        }

        return false;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in IsAccountSealed: " << e.what();
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
        "INSERT INTO account_login_log (account_id, login_time, login_ip, "
        "channel, version, uuid, idfa, imodel, imei) VALUES (?, ?, ?, ?, ?, "
        "?, ?, ?, ?)";

    try {
        auto stmt = conn->prepare(sql);
        if (!stmt) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to prepare statement";
            return false;
        }

        int64_t now = time(nullptr);
        stmt->bindInt64(1, account_id);
        stmt->bindInt64(2, now);
        stmt->bindInt32(3, ip);
        stmt->bindInt32(4, channel);
        stmt->bindString(5, version);
        stmt->bindString(6, uuid);
        stmt->bindString(7, idfa);
        stmt->bindString(8, imodel);
        stmt->bindString(9, imei);

        if (!stmt->execute()) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to execute statement";
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Exception in RecordLoginLog: " << e.what();
        return false;
    }
}

} // namespace game_server
