#ifndef __CONFIG_MANAGER_H__
#define __CONFIG_MANAGER_H__

#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <memory>
#include <sqlite3.h>

namespace game_server {

// 配置数据类型
enum class ConfigDataType {
    INT = 0,
    FLOAT = 1,
    STRING = 2,
    BOOL = 3
};

// 配置数据值
struct ConfigValue {
    ConfigDataType type;
    int32_t int_val;
    float float_val;
    std::string str_val;
    bool bool_val;
};

// 配置行数据
using ConfigRow = std::unordered_map<std::string, ConfigValue>;

// 配置表数据
struct ConfigTable {
    std::string table_name;
    std::vector<std::string> columns;
    std::vector<ConfigRow> rows;
    std::unordered_map<int32_t, ConfigRow*> id_index;
};

// 配置管理器类
class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();
    
    // 初始化
    bool Init(const std::string& db_path);
    
    // 加载配置
    bool LoadConfig(const std::string& table_name);
    bool LoadAllConfigs();
    bool ReloadConfig(const std::string& table_name);
    bool ReloadAllConfigs();
    
    // 查询配置
    bool GetConfigRow(const std::string& table_name, int32_t id, ConfigRow& row);
    bool GetConfigRows(const std::string& table_name, std::vector<ConfigRow>& rows);
    bool GetConfigValue(const std::string& table_name, int32_t id, const std::string& column, ConfigValue& value);
    
    // 模板方法
    template<typename T>
    T GetConfigInt(const std::string& table_name, int32_t id, const std::string& column, T default_val = 0) {
        ConfigValue value;
        if (GetConfigValue(table_name, id, column, value)) {
            return static_cast<T>(value.int_val);
        }
        return default_val;
    }
    
    float GetConfigFloat(const std::string& table_name, int32_t id, const std::string& column, float default_val = 0.0f);
    std::string GetConfigString(const std::string& table_name, int32_t id, const std::string& column, const std::string& default_val = "");
    bool GetConfigBool(const std::string& table_name, int32_t id, const std::string& column, bool default_val = false);
    
    // 获取配置表
    ConfigTable* GetConfigTable(const std::string& table_name);
    
    // 检查配置是否存在
    bool HasConfig(const std::string& table_name);
    bool HasConfigRow(const std::string& table_name, int32_t id);
    
    // 获取所有表名
    bool GetTableNames(std::vector<std::string>& names);
    
    // 关闭数据库
    void Close();
    
private:
    // 执行SQL查询
    bool ExecuteQuery(const std::string& sql, std::vector<ConfigRow>& results);
    
    // 解析配置值
    ConfigValue ParseValue(const std::string& value, const std::string& type);
    
    // 数据库连接
    sqlite3* db_;
    
    // 配置缓存
    std::unordered_map<std::string, ConfigTable> config_cache_;
    
    // 数据库路径
    std::string db_path_;
    
    // 互斥锁
    std::mutex config_mutex_;
    
    // 是否已初始化
    bool initialized_;
};

// 全局配置管理器实例
extern ConfigManager* g_config_manager;

// 配置访问宏
#define CONFIG_GET_INT(table, id, column, default_val) \
    (g_config_manager ? g_config_manager->GetConfigInt<int32_t>(table, id, column, default_val) : default_val)

#define CONFIG_GET_FLOAT(table, id, column, default_val) \
    (g_config_manager ? g_config_manager->GetConfigFloat(table, id, column, default_val) : default_val)

#define CONFIG_GET_STRING(table, id, column, default_val) \
    (g_config_manager ? g_config_manager->GetConfigString(table, id, column, default_val) : default_val)

#define CONFIG_GET_BOOL(table, id, column, default_val) \
    (g_config_manager ? g_config_manager->GetConfigBool(table, id, column, default_val) : default_val)

} // namespace game_server

#endif // __CONFIG_MANAGER_H__
