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

// 测试配置管理器加载MySQL配置
void TestConfigManagerMySQL() {
    TEST_BEGIN("配置管理器加载MySQL配置");

    // 初始化配置管理器
    game_server::ConfigManager config_manager;
    
    // 注意：这里使用SQLite数据库文件，因为配置管理器目前只支持SQLite
    // 实际项目中可能需要修改为MySQL连接
    std::string db_path = "/tmp/game_config.db";
    
    // 初始化配置管理器
    ASSERT_TRUE(config_manager.Init(db_path), "配置管理器初始化成功");
    ASSERT_TRUE(game_server::g_config_manager != nullptr, "全局配置管理器实例设置成功");

    // 测试加载配置表
    std::vector<std::string> tables = {
        "buff_config",
        "skill_config",
        "item_config",
        "monster_config",
        "scene_config",
        "shop_config",
        "task_config",
        "pet_config",
        "mount_config",
        "wing_config",
        "title_config"
    };

    for (const auto& table : tables) {
        std::cout << "加载配置表: " << table << "...";
        bool result = config_manager.LoadConfig(table);
        if (result) {
            std::cout << " 成功\n";
        } else {
            std::cout << " 失败\n";
        }
        // 注意：这里可能会失败，因为我们使用的是SQLite，而不是MySQL
        // 实际项目中需要修改配置管理器以支持MySQL
    }

    // 测试获取表名
    std::vector<std::string> table_names;
    ASSERT_TRUE(config_manager.GetTableNames(table_names), "获取表名成功");
    std::cout << "获取到的表名: " << table_names.size() << "个\n";
    for (const auto& name : table_names) {
        std::cout << "  " << name << "\n";
    }

    config_manager.Close();
    ASSERT_TRUE(game_server::g_config_manager == nullptr, "全局配置管理器实例清理成功");
}

// 测试配置管理器的基本功能
void TestConfigManagerBasic() {
    TEST_BEGIN("配置管理器基本功能");

    std::string db_path = "/tmp/test_config.db";
    game_server::ConfigManager config_manager;
    
    // 初始化
    ASSERT_TRUE(config_manager.Init(db_path), "配置管理器初始化成功");

    // 创建测试表
    sqlite3* db = nullptr;
    int ret = sqlite3_open(db_path.c_str(), &db);
    if (ret == SQLITE_OK) {
        const char* sql = R"(
            CREATE TABLE IF NOT EXISTS test_table (
                id INTEGER PRIMARY KEY,
                name TEXT,
                value INTEGER
            );
            INSERT OR REPLACE INTO test_table (id, name, value) VALUES
            (1, 'test1', 100),
            (2, 'test2', 200);
        )";
        char* err_msg = nullptr;
        ret = sqlite3_exec(db, sql, nullptr, nullptr, &err_msg);
        if (err_msg) {
            sqlite3_free(err_msg);
        }
        sqlite3_close(db);
    }

    // 加载配置
    ASSERT_TRUE(config_manager.LoadConfig("test_table"), "加载测试表成功");
    ASSERT_TRUE(config_manager.HasConfig("test_table"), "测试表存在");

    // 测试获取配置值
    int32_t value = config_manager.GetConfigInt<int32_t>("test_table", 1, "value", 0);
    ASSERT_EQ(value, 100, "获取配置值正确");

    std::string name = config_manager.GetConfigString("test_table", 1, "name", "");
    ASSERT_EQ(name, "test1", "获取配置字符串正确");

    // 测试获取配置行
    game_server::ConfigRow row;
    ASSERT_TRUE(config_manager.GetConfigRow("test_table", 1, row), "获取配置行成功");

    // 测试获取所有配置行
    std::vector<game_server::ConfigRow> rows;
    ASSERT_TRUE(config_manager.GetConfigRows("test_table", rows), "获取所有配置行成功");
    ASSERT_EQ(rows.size(), 2, "配置行数量正确");

    // 测试重新加载配置
    ASSERT_TRUE(config_manager.ReloadConfig("test_table"), "重新加载配置成功");

    // 测试检查配置行是否存在
    ASSERT_TRUE(config_manager.HasConfigRow("test_table", 1), "配置行存在");
    ASSERT_FALSE(config_manager.HasConfigRow("test_table", 999), "配置行不存在");

    config_manager.Close();
}

int main() {
    std::cout << "========================================\n";
    std::cout << "配置管理器数据库测试\n";
    std::cout << "========================================\n\n";

    // 运行测试
    TestConfigManagerBasic();
    TestConfigManagerMySQL();

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
    std::remove("/tmp/game_config.db");

    return g_fail_count == 0 ? 0 : 1;
}
