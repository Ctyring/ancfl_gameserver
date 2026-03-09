#include "config_manager.h"
#include <iostream>
#include <sstream>

namespace game_server {

ConfigManager* g_config_manager = nullptr;

ConfigManager::ConfigManager() : db_(nullptr), initialized_(false) {
}

ConfigManager::~ConfigManager() {
    Close();
}

bool ConfigManager::Init(const std::string& db_path) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    if (initialized_) {
        return true;
    }
    
    db_path_ = db_path;
    
    // 打开SQLite数据库
    int ret = sqlite3_open(db_path.c_str(), &db_);
    if (ret != SQLITE_OK) {
        LOG_ERROR("Failed to open database: %s, error: %s", db_path.c_str(), sqlite3_errmsg(db_));
        return false;
    }
    
    initialized_ = true;
    g_config_manager = this;
    
    LOG_INFO("Config manager initialized: db=%s", db_path.c_str());
    return true;
}

bool ConfigManager::LoadConfig(const std::string& table_name) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    if (!initialized_) {
        return false;
    }
    
    // 查询表结构
    std::string sql = "PRAGMA table_info(" + table_name + ")";
    std::vector<ConfigRow> columns_info;
    if (!ExecuteQuery(sql, columns_info)) {
        LOG_ERROR("Failed to get table info: %s", table_name.c_str());
        return false;
    }
    
    if (columns_info.empty()) {
        LOG_ERROR("Table not found: %s", table_name.c_str());
        return false;
    }
    
    // 创建配置表
    ConfigTable table;
    table.table_name = table_name;
    
    // 获取列名
    for (const auto& col : columns_info) {
        auto it = col.find("name");
        if (it != col.end()) {
            table.columns.push_back(it->second.str_val);
        }
    }
    
    // 查询数据
    sql = "SELECT * FROM " + table_name;
    if (!ExecuteQuery(sql, table.rows)) {
        LOG_ERROR("Failed to load config: %s", table_name.c_str());
        return false;
    }
    
    // 建立ID索引
    for (auto& row : table.rows) {
        auto it = row.find("id");
        if (it != row.end()) {
            table.id_index[it->second.int_val] = &row;
        }
    }
    
    // 缓存配置表
    config_cache_[table_name] = table;
    
    LOG_INFO("Config loaded: table=%s, rows=%d", table_name.c_str(), table.rows.size());
    return true;
}

bool ConfigManager::LoadAllConfigs() {
    std::vector<std::string> table_names;
    if (!GetTableNames(table_names)) {
        return false;
    }
    
    for (const auto& name : table_names) {
        if (!LoadConfig(name)) {
            LOG_ERROR("Failed to load config: %s", name.c_str());
        }
    }
    
    return true;
}

bool ConfigManager::ReloadConfig(const std::string& table_name) {
    return LoadConfig(table_name);
}

bool ConfigManager::ReloadAllConfigs() {
    return LoadAllConfigs();
}

bool ConfigManager::GetConfigRow(const std::string& table_name, int32_t id, ConfigRow& row) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    auto table_it = config_cache_.find(table_name);
    if (table_it == config_cache_.end()) {
        return false;
    }
    
    auto row_it = table_it->second.id_index.find(id);
    if (row_it == table_it->second.id_index.end()) {
        return false;
    }
    
    row = *row_it->second;
    return true;
}

bool ConfigManager::GetConfigRows(const std::string& table_name, std::vector<ConfigRow>& rows) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    auto table_it = config_cache_.find(table_name);
    if (table_it == config_cache_.end()) {
        return false;
    }
    
    rows = table_it->second.rows;
    return true;
}

bool ConfigManager::GetConfigValue(const std::string& table_name, int32_t id, const std::string& column, ConfigValue& value) {
    ConfigRow row;
    if (!GetConfigRow(table_name, id, row)) {
        return false;
    }
    
    auto it = row.find(column);
    if (it == row.end()) {
        return false;
    }
    
    value = it->second;
    return true;
}

float ConfigManager::GetConfigFloat(const std::string& table_name, int32_t id, const std::string& column, float default_val) {
    ConfigValue value;
    if (GetConfigValue(table_name, id, column, value)) {
        return value.float_val;
    }
    return default_val;
}

std::string ConfigManager::GetConfigString(const std::string& table_name, int32_t id, const std::string& column, const std::string& default_val) {
    ConfigValue value;
    if (GetConfigValue(table_name, id, column, value)) {
        return value.str_val;
    }
    return default_val;
}

bool ConfigManager::GetConfigBool(const std::string& table_name, int32_t id, const std::string& column, bool default_val) {
    ConfigValue value;
    if (GetConfigValue(table_name, id, column, value)) {
        return value.bool_val;
    }
    return default_val;
}

ConfigTable* ConfigManager::GetConfigTable(const std::string& table_name) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    auto it = config_cache_.find(table_name);
    if (it == config_cache_.end()) {
        return nullptr;
    }
    
    return &it->second;
}

bool ConfigManager::HasConfig(const std::string& table_name) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return config_cache_.find(table_name) != config_cache_.end();
}

bool ConfigManager::HasConfigRow(const std::string& table_name, int32_t id) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    auto table_it = config_cache_.find(table_name);
    if (table_it == config_cache_.end()) {
        return false;
    }
    
    return table_it->second.id_index.find(id) != table_it->second.id_index.end();
}

bool ConfigManager::GetTableNames(std::vector<std::string>& names) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    if (!initialized_) {
        return false;
    }
    
    names.clear();
    
    std::string sql = "SELECT name FROM sqlite_master WHERE type='table'";
    std::vector<ConfigRow> results;
    if (!ExecuteQuery(sql, results)) {
        return false;
    }
    
    for (const auto& row : results) {
        auto it = row.find("name");
        if (it != row.end()) {
            names.push_back(it->second.str_val);
        }
    }
    
    return true;
}

void ConfigManager::Close() {
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
    
    config_cache_.clear();
    initialized_ = false;
    g_config_manager = nullptr;
}

bool ConfigManager::ExecuteQuery(const std::string& sql, std::vector<ConfigRow>& results) {
    results.clear();
    
    sqlite3_stmt* stmt = nullptr;
    int ret = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (ret != SQLITE_OK) {
        LOG_ERROR("Failed to prepare SQL: %s, error: %s", sql.c_str(), sqlite3_errmsg(db_));
        return false;
    }
    
    int column_count = sqlite3_column_count(stmt);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ConfigRow row;
        
        for (int i = 0; i < column_count; ++i) {
            const char* name = sqlite3_column_name(stmt, i);
            int type = sqlite3_column_type(stmt, i);
            
            ConfigValue value;
            value.type = ConfigDataType::STRING;
            
            switch (type) {
                case SQLITE_INTEGER:
                    value.type = ConfigDataType::INT;
                    value.int_val = sqlite3_column_int(stmt, i);
                    break;
                case SQLITE_FLOAT:
                    value.type = ConfigDataType::FLOAT;
                    value.float_val = static_cast<float>(sqlite3_column_double(stmt, i));
                    break;
                case SQLITE_TEXT:
                    value.type = ConfigDataType::STRING;
                    value.str_val = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                    break;
                case SQLITE_BLOB:
                    value.type = ConfigDataType::STRING;
                    // 处理BLOB
                    break;
                case SQLITE_NULL:
                    value.type = ConfigDataType::INT;
                    value.int_val = 0;
                    break;
            }
            
            row[name] = value;
        }
        
        results.push_back(row);
    }
    
    sqlite3_finalize(stmt);
    return true;
}

ConfigValue ConfigManager::ParseValue(const std::string& value, const std::string& type) {
    ConfigValue result;
    
    if (type == "int" || type == "integer") {
        result.type = ConfigDataType::INT;
        result.int_val = std::stoi(value);
    } else if (type == "float" || type == "real") {
        result.type = ConfigDataType::FLOAT;
        result.float_val = std::stof(value);
    } else if (type == "bool" || type == "boolean") {
        result.type = ConfigDataType::BOOL;
        result.bool_val = (value == "true" || value == "1");
    } else {
        result.type = ConfigDataType::STRING;
        result.str_val = value;
    }
    
    return result;
}

} // namespace game_server
