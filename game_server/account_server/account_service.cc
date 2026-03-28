#include "account_service.h"
#include "ancfl/config.h"
#include "ancfl/util/hash_util.h"

namespace game_server {

AccountService::AccountService()
    : GameServiceBase("AccountServer"), db_port_(3306) {}

AccountService::~AccountService() {}

bool AccountService::InitService() {
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Initializing AccountServer...";

    // 硬编码配置
    std::string ip = "0.0.0.0";
    int port = 8100;
    service_id_ = 2;
    db_host_ = "127.0.0.1";
    db_port_ = 3306;
    db_user_ = "root";
    db_password_ = "12345678";
    db_name_ = "game_server";

    // 初始化数据库
    if (!InitDatabase()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to initialize database";
        return false;
    }

    // 初始化TCP服务
    if (!Init(ip, port)) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to init TCP service";
        return false;
    }

    // 创建消息分发器
    dispatcher_.reset(new MessageDispatcher());
    RegisterAllHandlers();
    SetMessageDispatcher(dispatcher_);

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT())
        << "AccountServer initialized on " << ip << ":" << port;
    return true;
}

void AccountService::UninitService() {
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Uninitializing AccountServer...";
    CloseDatabase();
    Uninit();
}

void AccountService::RegisterAllHandlers() {
    // 注册消息处理器
    dispatcher_->RegisterHandler(100003, std::bind(&AccountService::OnAccountRegReq, this, std::placeholders::_1));  // MSG_ACCOUNT_REG_REQ
    dispatcher_->RegisterHandler(100005, std::bind(&AccountService::OnAccountLoginReq, this, std::placeholders::_1));  // MSG_ACCOUNT_LOGIN_REQ
    dispatcher_->RegisterHandler(100039, std::bind(&AccountService::OnSealAccountReq, this, std::placeholders::_1));  // MSG_SEAL_ACCOUNT_REQ
    dispatcher_->RegisterHandler(100024, std::bind(&AccountService::OnHeartBeatReq, this, std::placeholders::_1));  // MSG_WATCH_HEART_BEAT_REQ

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT())
        << "Registered " << dispatcher_->GetHandlerCount()
        << " message handlers";
}

void AccountService::OnTimer() {
    // 调用基类定时器
    OnSecondTimer();
}

bool AccountService::InitDatabase() {
    // 构建 MySQL 连接参数
    std::map<std::string, std::string> args;
    args["host"] = db_host_;
    args["port"] = std::to_string(db_port_);
    args["user"] = db_user_;
    args["passwd"] = db_password_;
    args["dbname"] = db_name_;

    mysql_.reset(new ancfl::MySQL(args));

    if (!mysql_->connect()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT())
            << "Failed to connect to MySQL: " << db_host_ << ":" << db_port_;
        return false;
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Connected to MySQL database";
    return true;
}

void AccountService::CloseDatabase() {
    if (mysql_) {
        mysql_.reset();
    }
}

std::string AccountService::MD5Encrypt(const std::string& input) {
    return ancfl::md5(input);
}

bool AccountService::CreateAccount(const std::string& account_name,
                                   const std::string& password,
                                   int32_t channel,
                                   uint64_t& account_id) {
    // 检查账号名是否已存在
    AccountInfo info;
    if (GetAccountInfo(account_name, info)) {
        ANCFL_LOG_WARN(ANCFL_LOG_ROOT())
            << "Account already exists: " << account_name;
        return false;
    }

    // 加密密码
    std::string encrypted_password = MD5Encrypt(password);
    int64_t now = time(nullptr);

    // 插入数据库
    char sql[512];
    snprintf(sql, sizeof(sql),
             "INSERT INTO account (account_name, password, channel, "
             "create_time, last_login_time, is_sealed) "
             "VALUES ('%s', '%s', %d, %ld, %ld, 0)",
             account_name.c_str(), encrypted_password.c_str(), channel, now,
             now);

    if (!mysql_->execute(sql)) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT())
            << "Failed to create account: " << mysql_->getErrStr();
        return false;
    }

    // 获取插入的ID
    account_id = mysql_->getLastInsertId();

    // 更新缓存
    {
        ancfl::Mutex::Lock lock(cache_mutex_);
        AccountInfo info;
        info.account_id = account_id;
        info.account_name = account_name;
        info.password = encrypted_password;
        info.channel = channel;
        info.create_time = now;
        info.last_login_time = now;
        info.is_sealed = false;
        info.seal_end_time = 0;
        info.review = false;

        account_cache_[account_id] = info;
        name_to_id_[account_name] = account_id;
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT())
        << "Account created: " << account_name << " id=" << account_id;
    return true;
}

bool AccountService::VerifyAccount(const std::string& account_name,
                                   const std::string& password,
                                   AccountInfo& info) {
    // 先从缓存查找
    {
        ancfl::Mutex::Lock lock(cache_mutex_);
        auto it = name_to_id_.find(account_name);
        if (it != name_to_id_.end()) {
            auto acc_it = account_cache_.find(it->second);
            if (acc_it != account_cache_.end()) {
                std::string encrypted_password = MD5Encrypt(password);
                if (acc_it->second.password == encrypted_password) {
                    info = acc_it->second;
                    return true;
                }
            }
        }
    }

    // 从数据库查询
    char sql[256];
    snprintf(sql, sizeof(sql),
             "SELECT account_id, account_name, password, channel, create_time, "
             "last_login_time, last_login_ip, is_sealed, seal_end_time, review "
             "FROM account WHERE account_name = '%s'",
             account_name.c_str());

    auto result = mysql_->query(sql);
    if (!result || result->getDataCount() == 0) {
        return false;
    }

    result->next();
    info.account_id = result->getInt64(0);
    info.account_name = result->getString(1);
    info.password = result->getString(2);
    info.channel = result->getInt32(3);
    info.create_time = result->getInt64(4);
    info.last_login_time = result->getInt64(5);
    info.last_login_ip = result->getInt32(6);
    info.is_sealed = result->getInt32(7) != 0;
    info.seal_end_time = result->getInt64(8);
    info.review = result->getInt32(9) != 0;

    // 验证密码
    std::string encrypted_password = MD5Encrypt(password);
    if (info.password != encrypted_password) {
        return false;
    }

    // 更新缓存
    {
        ancfl::Mutex::Lock lock(cache_mutex_);
        account_cache_[info.account_id] = info;
        name_to_id_[account_name] = info.account_id;
    }

    return true;
}

bool AccountService::GetAccountInfo(uint64_t account_id, AccountInfo& info) {
    // 先从缓存查找
    {
        ancfl::Mutex::Lock lock(cache_mutex_);
        auto it = account_cache_.find(account_id);
        if (it != account_cache_.end()) {
            info = it->second;
            return true;
        }
    }

    // 从数据库查询
    char sql[256];
    snprintf(sql, sizeof(sql),
             "SELECT account_id, account_name, password, channel, create_time, "
             "last_login_time, last_login_ip, is_sealed, seal_end_time, review "
             "FROM account WHERE account_id = %lu",
             account_id);

    auto result = mysql_->query(sql);
    if (!result || result->getDataCount() == 0) {
        return false;
    }

    result->next();
    info.account_id = result->getInt64(0);
    info.account_name = result->getString(1);
    info.password = result->getString(2);
    info.channel = result->getInt32(3);
    info.create_time = result->getInt64(4);
    info.last_login_time = result->getInt64(5);
    info.last_login_ip = result->getInt32(6);
    info.is_sealed = result->getInt32(7) != 0;
    info.seal_end_time = result->getInt64(8);
    info.review = result->getInt32(9) != 0;

    // 更新缓存
    {
        ancfl::Mutex::Lock lock(cache_mutex_);
        account_cache_[info.account_id] = info;
        name_to_id_[info.account_name] = info.account_id;
    }

    return true;
}

bool AccountService::GetAccountInfo(const std::string& account_name,
                                    AccountInfo& info) {
    // 先从缓存查找
    {
        ancfl::Mutex::Lock lock(cache_mutex_);
        auto it = name_to_id_.find(account_name);
        if (it != name_to_id_.end()) {
            auto acc_it = account_cache_.find(it->second);
            if (acc_it != account_cache_.end()) {
                info = acc_it->second;
                return true;
            }
        }
    }

    // 从数据库查询
    char sql[256];
    snprintf(sql, sizeof(sql),
             "SELECT account_id, account_name, password, channel, create_time, "
             "last_login_time, last_login_ip, is_sealed, seal_end_time, review "
             "FROM account WHERE account_name = '%s'",
             account_name.c_str());

    auto result = mysql_->query(sql);
    if (!result || result->getDataCount() == 0) {
        return false;
    }

    result->next();
    info.account_id = result->getInt64(0);
    info.account_name = result->getString(1);
    info.password = result->getString(2);
    info.channel = result->getInt32(3);
    info.create_time = result->getInt64(4);
    info.last_login_time = result->getInt64(5);
    info.last_login_ip = result->getInt32(6);
    info.is_sealed = result->getInt32(7) != 0;
    info.seal_end_time = result->getInt64(8);
    info.review = result->getInt32(9) != 0;

    // 更新缓存
    {
        ancfl::Mutex::Lock lock(cache_mutex_);
        account_cache_[info.account_id] = info;
        name_to_id_[account_name] = info.account_id;
    }

    return true;
}

bool AccountService::SealAccount(uint64_t account_id, int32_t seal_time) {
    int64_t seal_end_time = time(nullptr) + seal_time;

    char sql[256];
    snprintf(sql, sizeof(sql),
             "UPDATE account SET is_sealed = 1, seal_end_time = %ld WHERE "
             "account_id = %lu",
             seal_end_time, account_id);

    if (!mysql_->execute(sql)) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT())
            << "Failed to seal account: " << mysql_->getErrStr();
        return false;
    }

    // 更新缓存
    {
        ancfl::Mutex::Lock lock(cache_mutex_);
        auto it = account_cache_.find(account_id);
        if (it != account_cache_.end()) {
            it->second.is_sealed = true;
            it->second.seal_end_time = seal_end_time;
        }
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Account sealed: " << account_id
                                     << " for " << seal_time << " seconds";
    return true;
}

bool AccountService::UnsealAccount(uint64_t account_id) {
    char sql[256];
    snprintf(sql, sizeof(sql),
             "UPDATE account SET is_sealed = 0, seal_end_time = 0 WHERE "
             "account_id = %lu",
             account_id);

    if (!mysql_->execute(sql)) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT())
            << "Failed to unseal account: " << mysql_->getErrStr();
        return false;
    }

    // 更新缓存
    {
        ancfl::Mutex::Lock lock(cache_mutex_);
        auto it = account_cache_.find(account_id);
        if (it != account_cache_.end()) {
            it->second.is_sealed = false;
            it->second.seal_end_time = 0;
        }
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Account unsealed: " << account_id;
    return true;
}

bool AccountService::IsAccountSealed(uint64_t account_id) {
    AccountInfo info;
    if (!GetAccountInfo(account_id, info)) {
        return false;
    }

    if (!info.is_sealed) {
        return false;
    }

    // 检查封号是否过期
    if (info.seal_end_time > 0 && time(nullptr) > info.seal_end_time) {
        UnsealAccount(account_id);
        return false;
    }

    return true;
}

bool AccountService::RecordLoginLog(uint64_t account_id,
                                    int32_t channel,
                                    const std::string& version,
                                    const std::string& uuid,
                                    const std::string& idfa,
                                    const std::string& imodel,
                                    const std::string& imei,
                                    int32_t ip) {
    char sql[512];
    snprintf(sql, sizeof(sql),
             "INSERT INTO account_login_log (account_id, login_time, login_ip, "
             "channel, version, uuid, idfa, imodel, imei) "
             "VALUES (%lu, %ld, %d, %d, '%s', '%s', '%s', '%s', '%s')",
             account_id, time(nullptr), ip, channel, version.c_str(),
             uuid.c_str(), idfa.c_str(), imodel.c_str(), imei.c_str());

    if (!mysql_->execute(sql)) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT())
            << "Failed to record login log: " << mysql_->getErrStr();
        return false;
    }

    return true;
}

// ==================== 消息处理器 ====================

bool AccountService::OnAccountRegReq(const NetPacket& packet) {
    // TODO: 实现账号注册请求处理
    return true;
}

bool AccountService::OnAccountLoginReq(const NetPacket& packet) {
    // TODO: 实现账号登录请求处理
    return true;
}

bool AccountService::OnSealAccountReq(const NetPacket& packet) {
    // TODO: 实现封号请求处理
    return true;
}

bool AccountService::OnHeartBeatReq(const NetPacket& packet) {
    // TODO: 实现心跳响应
    return true;
}

}  // namespace game_server
