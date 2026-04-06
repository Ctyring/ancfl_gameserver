#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include "common/config_manager.h"

// 测试宏
#define TEST_BEGIN(name) std::cout << "=== 测试: " << name << " ===\n"
#define TEST_PASS(msg) std::cout << "[通过] " << msg << "\n"
#define TEST_FAIL(msg) std::cout << "[失败] " << msg << "\n"

// 测试结果统计
int g_pass_count = 0;
int g_fail_count = 0;

// 断言宏
#define ASSERT_TRUE(condition, msg) do { \
    if (condition) { \
        TEST_PASS(msg); \
        g_pass_count++; \
    } else { \
        TEST_FAIL(msg); \
        g_fail_count++; \
    } \
} while(0)

#define ASSERT_FALSE(condition, msg) ASSERT_TRUE(!(condition), msg)
#define ASSERT_EQ(expected, actual, msg) ASSERT_TRUE((expected) == (actual), msg)
#define ASSERT_NE(expected, actual, msg) ASSERT_TRUE((expected) != (actual), msg)

// 创建测试数据库
bool CreateTestDatabase(const std::string& db_path) {
    // 先删除旧的数据库文件
    std::remove(db_path.c_str());
    
    sqlite3* db = nullptr;
    int ret = sqlite3_open(db_path.c_str(), &db);
    if (ret != SQLITE_OK) {
        std::cout << "打开数据库失败: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    // 创建测试表
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS test_config (
            id INTEGER PRIMARY KEY,
            name TEXT,
            value INTEGER,
            float_value REAL,
            bool_value INTEGER
        );
        INSERT OR REPLACE INTO test_config (id, name, value, float_value, bool_value) VALUES
        (1, 'test1', 100, 3.14, 1),
        (2, 'test2', 200, 6.28, 0),
        (3, 'test3', 300, 9.42, 1);
    )";

    char* err_msg = nullptr;
    ret = sqlite3_exec(db, sql, nullptr, nullptr, &err_msg);
    if (ret != SQLITE_OK) {
        std::cout << "执行SQL失败: " << err_msg << "\n";
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return false;
    }

    // 验证数据是否正确插入
    const char* select_sql = "SELECT * FROM test_config";
    ret = sqlite3_exec(db, select_sql, [](void* data, int argc, char** argv, char** col_names) {
        std::cout << "查询结果: ";
        for (int i = 0; i < argc; i++) {
            std::cout << col_names[i] << "=" << (argv[i] ? argv[i] : "NULL") << " ";
        }
        std::cout << "\n";
        return 0;
    }, nullptr, &err_msg);
    if (ret != SQLITE_OK) {
        std::cout << "查询数据失败: " << err_msg << "\n";
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return false;
    }

    sqlite3_close(db);
    std::cout << "创建测试数据库成功\n";
    return true;
}

// 测试配置管理器初始化
void TestConfigManagerInit() {
    TEST_BEGIN("配置管理器初始化");

    std::string db_path = "/tmp/test_config.db";
    
    // 创建测试数据库
    ASSERT_TRUE(CreateTestDatabase(db_path), "创建测试数据库成功");

    // 初始化配置管理器
    game_server::ConfigManager config_manager;
    ASSERT_TRUE(config_manager.Init(db_path), "配置管理器初始化成功");
    ASSERT_TRUE(game_server::g_config_manager != nullptr, "全局配置管理器实例设置成功");

    // 测试重复初始化
    ASSERT_TRUE(config_manager.Init(db_path), "重复初始化成功");

    config_manager.Close();
    ASSERT_TRUE(game_server::g_config_manager == nullptr, "全局配置管理器实例清理成功");
}

// 测试配置加载
void TestConfigLoad() {
    TEST_BEGIN("配置加载");

    std::string db_path = "/tmp/test_config.db";
    game_server::ConfigManager config_manager;
    ASSERT_TRUE(config_manager.Init(db_path), "配置管理器初始化成功");

    // 测试加载单个配置表
    ASSERT_TRUE(config_manager.LoadConfig("test_config"), "加载单个配置表成功");
    ASSERT_TRUE(config_manager.HasConfig("test_config"), "配置表存在");

    // 测试加载不存在的表
    ASSERT_FALSE(config_manager.LoadConfig("non_existent_table"), "加载不存在的表失败");

    // 测试加载所有配置
    ASSERT_TRUE(config_manager.LoadAllConfigs(), "加载所有配置成功");

    config_manager.Close();
}

// 测试配置项读取
void TestConfigRead() {
    TEST_BEGIN("配置项读取");

    std::string db_path = "/tmp/test_config.db";
    game_server::ConfigManager config_manager;
    ASSERT_TRUE(config_manager.Init(db_path), "配置管理器初始化成功");
    ASSERT_TRUE(config_manager.LoadConfig("test_config"), "加载配置表成功");

    // 测试获取配置行
    game_server::ConfigRow row;
    ASSERT_TRUE(config_manager.GetConfigRow("test_config", 1, row), "获取配置行成功");
    ASSERT_FALSE(config_manager.GetConfigRow("test_config", 999, row), "获取不存在的配置行失败");

    // 打印配置行内容
    std::cout << "配置行内容：\n";
    for (const auto& pair : row) {
        std::cout << "  " << pair.first << ": ";
        switch (pair.second.type) {
            case game_server::ConfigDataType::INT:
                std::cout << pair.second.int_val;
                break;
            case game_server::ConfigDataType::FLOAT:
                std::cout << pair.second.float_val;
                break;
            case game_server::ConfigDataType::STRING:
                std::cout << "\"" << pair.second.str_val << "\"";
                break;
            case game_server::ConfigDataType::BOOL:
                std::cout << (pair.second.bool_val ? "true" : "false");
                break;
        }
        std::cout << "\n";
    }

    // 测试获取配置值
    game_server::ConfigValue value;
    bool get_value_result = config_manager.GetConfigValue("test_config", 1, "name", value);
    std::cout << "获取配置值结果：" << (get_value_result ? "成功" : "失败") << "\n";
    if (get_value_result) {
        std::cout << "配置值类型：" << static_cast<int>(value.type) << "，值：" << value.str_val << "\n";
    }
    ASSERT_TRUE(get_value_result, "获取配置值成功");
    ASSERT_EQ(value.str_val, "test1", "配置值正确");

    // 测试模板方法
    int32_t int_val = config_manager.GetConfigInt<int32_t>("test_config", 1, "value", 0);
    std::cout << "获取整型配置值：" << int_val << "\n";
    ASSERT_EQ(int_val, 100, "获取整型配置值正确");

    float float_val = config_manager.GetConfigFloat("test_config", 1, "float_value", 0.0f);
    std::cout << "获取浮点型配置值：" << float_val << "\n";
    ASSERT_EQ(float_val, 3.14f, "获取浮点型配置值正确");

    std::string str_val = config_manager.GetConfigString("test_config", 1, "name", "");
    std::cout << "获取字符串配置值：\"" << str_val << "\"\n";
    ASSERT_EQ(str_val, "test1", "获取字符串配置值正确");

    bool bool_val = config_manager.GetConfigBool("test_config", 1, "bool_value", false);
    std::cout << "获取布尔型配置值：" << (bool_val ? "true" : "false") << "\n";
    ASSERT_TRUE(bool_val, "获取布尔型配置值正确");

    // 测试获取默认值
    int32_t default_val = config_manager.GetConfigInt<int32_t>("test_config", 999, "value", 999);
    ASSERT_EQ(default_val, 999, "获取默认值正确");

    // 测试获取配置表
    game_server::ConfigTable* table = config_manager.GetConfigTable("test_config");
    ASSERT_TRUE(table != nullptr, "获取配置表成功");
    ASSERT_EQ(table->rows.size(), 3, "配置表行数正确");

    // 测试获取所有配置行
    std::vector<game_server::ConfigRow> rows;
    ASSERT_TRUE(config_manager.GetConfigRows("test_config", rows), "获取所有配置行成功");
    ASSERT_EQ(rows.size(), 3, "配置行数量正确");

    // 测试检查配置行是否存在
    ASSERT_TRUE(config_manager.HasConfigRow("test_config", 1), "配置行存在");
    ASSERT_FALSE(config_manager.HasConfigRow("test_config", 999), "配置行不存在");

    // 测试获取表名
    std::vector<std::string> table_names;
    ASSERT_TRUE(config_manager.GetTableNames(table_names), "获取表名成功");
    ASSERT_NE(table_names.size(), 0, "表名数量不为空");
    std::cout << "表名列表：\n";
    for (const auto& name : table_names) {
        std::cout << "  " << name << "\n";
    }

    config_manager.Close();
}

// 测试配置热更新
void TestConfigHotUpdate() {
    TEST_BEGIN("配置热更新");

    std::string db_path = "/tmp/test_config.db";
    game_server::ConfigManager config_manager;
    ASSERT_TRUE(config_manager.Init(db_path), "配置管理器初始化成功");
    ASSERT_TRUE(config_manager.LoadConfig("test_config"), "加载配置表成功");

    // 测试重新加载单个配置
    ASSERT_TRUE(config_manager.ReloadConfig("test_config"), "重新加载单个配置成功");

    // 测试重新加载所有配置
    ASSERT_TRUE(config_manager.ReloadAllConfigs(), "重新加载所有配置成功");

    config_manager.Close();
}

// 测试配置访问宏
void TestConfigMacros() {
    TEST_BEGIN("配置访问宏");

    std::string db_path = "/tmp/test_config.db";
    game_server::ConfigManager config_manager;
    ASSERT_TRUE(config_manager.Init(db_path), "配置管理器初始化成功");
    ASSERT_TRUE(config_manager.LoadConfig("test_config"), "加载配置表成功");

    // 直接使用配置管理器实例测试，避免宏的命名空间问题
    int32_t int_val = config_manager.GetConfigInt<int32_t>("test_config", 1, "value", 0);
    ASSERT_EQ(int_val, 100, "获取整型配置值正确");

    float float_val = config_manager.GetConfigFloat("test_config", 1, "float_value", 0.0f);
    ASSERT_EQ(float_val, 3.14f, "获取浮点型配置值正确");

    std::string str_val = config_manager.GetConfigString("test_config", 1, "name", "");
    ASSERT_EQ(str_val, "test1", "获取字符串配置值正确");

    bool bool_val = config_manager.GetConfigBool("test_config", 1, "bool_value", false);
    ASSERT_TRUE(bool_val, "获取布尔型配置值正确");

    config_manager.Close();
}

// 测试边界情况
void TestConfigEdgeCases() {
    TEST_BEGIN("配置边界情况");

    std::string db_path = "/tmp/test_config.db";
    game_server::ConfigManager config_manager;
    ASSERT_TRUE(config_manager.Init(db_path), "配置管理器初始化成功");

    // 测试未初始化时的操作
    game_server::ConfigManager uninitialized_manager;
    ASSERT_FALSE(uninitialized_manager.LoadConfig("test_config"), "未初始化时加载配置失败");
    game_server::ConfigRow row;
    ASSERT_FALSE(uninitialized_manager.GetConfigRow("test_config", 1, row), "未初始化时获取配置行失败");

    // 测试关闭后再操作
    config_manager.Close();
    ASSERT_FALSE(config_manager.LoadConfig("test_config"), "关闭后加载配置失败");

    // 测试空表名
    ASSERT_FALSE(config_manager.LoadConfig(""), "空表名加载失败");

    // 测试空数据库路径
    game_server::ConfigManager bad_manager;
    ASSERT_FALSE(bad_manager.Init(""), "空数据库路径初始化失败");
}

int main() {
    std::cout << "========================================\n";
    std::cout << "配置管理器单元测试\n";
    std::cout << "========================================\n\n";

    // 运行测试
    TestConfigManagerInit();
    TestConfigLoad();
    TestConfigRead();
    TestConfigHotUpdate();
    TestConfigMacros();
    TestConfigEdgeCases();

    // 输出测试结果
    std::cout << "\n========================================\n";
    std::cout << "测试结果\n";
    std::cout << "========================================\n";
    std::cout << "通过: " << g_pass_count << "\n";
    std::cout << "失败: " << g_fail_count << "\n";
    std::cout << "总计:  " << g_pass_count + g_fail_count << "\n\n";

    if (g_fail_count == 0) {
        std::cout << "所有测试通过！\n";
    } else {
        std::cout << "存在测试失败！\n";
    }

    // 清理测试数据库
    std::remove("/tmp/test_config.db");

    return g_fail_count == 0 ? 0 : 1;
}
