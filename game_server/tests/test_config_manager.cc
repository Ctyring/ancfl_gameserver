#include <gtest/gtest.h>
#include "config_manager.h"
#include <sqlite3.h>
#include <fstream>

using namespace game_server;

class ConfigManagerTest : public testing::Test {
protected:
    void SetUp() override {
        db_path_ = "test_config.db";
        CreateTestDatabase();
    }
    
    void TearDown() override {
        if (g_config_manager) {
            g_config_manager->Close();
            delete g_config_manager;
            g_config_manager = nullptr;
        }
        remove(db_path_.c_str());
    }
    
    void CreateTestDatabase() {
        sqlite3* db;
        sqlite3_open(db_path_.c_str(), &db);
        
        const char* sql = 
            "CREATE TABLE test_table ("
            "id INTEGER PRIMARY KEY,"
            "name TEXT,"
            "value INTEGER,"
            "rate REAL"
            ");"
            "INSERT INTO test_table VALUES (1, 'item1', 100, 1.5);"
            "INSERT INTO test_table VALUES (2, 'item2', 200, 2.5);"
            "INSERT INTO test_table VALUES (3, 'item3', 300, 3.5);";
        
        sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
        sqlite3_close(db);
    }
    
    std::string db_path_;
};

TEST_F(ConfigManagerTest, Init) {
    ConfigManager config;
    EXPECT_TRUE(config.Init(db_path_));
}

TEST_F(ConfigManagerTest, LoadConfig) {
    ConfigManager config;
    config.Init(db_path_);
    
    EXPECT_TRUE(config.LoadConfig("test_table"));
    EXPECT_TRUE(config.HasConfig("test_table"));
}

TEST_F(ConfigManagerTest, LoadNonExistingConfig) {
    ConfigManager config;
    config.Init(db_path_);
    
    EXPECT_FALSE(config.LoadConfig("non_existing_table"));
    EXPECT_FALSE(config.HasConfig("non_existing_table"));
}

TEST_F(ConfigManagerTest, GetConfigRow) {
    ConfigManager config;
    config.Init(db_path_);
    config.LoadConfig("test_table");
    
    ConfigRow row;
    EXPECT_TRUE(config.GetConfigRow("test_table", 1, row));
    
    EXPECT_EQ(row["id"].int_val, 1);
    EXPECT_EQ(row["name"].str_val, "item1");
    EXPECT_EQ(row["value"].int_val, 100);
    EXPECT_FLOAT_EQ(row["rate"].float_val, 1.5f);
}

TEST_F(ConfigManagerTest, GetNonExistingRow) {
    ConfigManager config;
    config.Init(db_path_);
    config.LoadConfig("test_table");
    
    ConfigRow row;
    EXPECT_FALSE(config.GetConfigRow("test_table", 999, row));
}

TEST_F(ConfigManagerTest, GetConfigValue) {
    ConfigManager config;
    config.Init(db_path_);
    config.LoadConfig("test_table");
    
    ConfigValue value;
    EXPECT_TRUE(config.GetConfigValue("test_table", 1, "name", value));
    EXPECT_EQ(value.str_val, "item1");
    
    EXPECT_TRUE(config.GetConfigValue("test_table", 1, "value", value));
    EXPECT_EQ(value.int_val, 100);
}

TEST_F(ConfigManagerTest, GetConfigInt) {
    ConfigManager config;
    config.Init(db_path_);
    config.LoadConfig("test_table");
    
    EXPECT_EQ(config.GetConfigInt<int32_t>("test_table", 1, "value", 0), 100);
    EXPECT_EQ(config.GetConfigInt<int32_t>("test_table", 999, "value", -1), -1);
}

TEST_F(ConfigManagerTest, GetConfigFloat) {
    ConfigManager config;
    config.Init(db_path_);
    config.LoadConfig("test_table");
    
    EXPECT_FLOAT_EQ(config.GetConfigFloat("test_table", 1, "rate", 0.0f), 1.5f);
    EXPECT_FLOAT_EQ(config.GetConfigFloat("test_table", 999, "rate", -1.0f), -1.0f);
}

TEST_F(ConfigManagerTest, GetConfigString) {
    ConfigManager config;
    config.Init(db_path_);
    config.LoadConfig("test_table");
    
    EXPECT_EQ(config.GetConfigString("test_table", 1, "name", ""), "item1");
    EXPECT_EQ(config.GetConfigString("test_table", 999, "name", "default"), "default");
}

TEST_F(ConfigManagerTest, GetConfigRows) {
    ConfigManager config;
    config.Init(db_path_);
    config.LoadConfig("test_table");
    
    std::vector<ConfigRow> rows;
    EXPECT_TRUE(config.GetConfigRows("test_table", rows));
    EXPECT_EQ(rows.size(), 3);
}

TEST_F(ConfigManagerTest, HasConfigRow) {
    ConfigManager config;
    config.Init(db_path_);
    config.LoadConfig("test_table");
    
    EXPECT_TRUE(config.HasConfigRow("test_table", 1));
    EXPECT_TRUE(config.HasConfigRow("test_table", 2));
    EXPECT_FALSE(config.HasConfigRow("test_table", 999));
}

TEST_F(ConfigManagerTest, GetTableNames) {
    ConfigManager config;
    config.Init(db_path_);
    
    std::vector<std::string> names;
    EXPECT_TRUE(config.GetTableNames(names));
    
    bool found = false;
    for (const auto& name : names) {
        if (name == "test_table") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ConfigManagerTest, ReloadConfig) {
    ConfigManager config;
    config.Init(db_path_);
    config.LoadConfig("test_table");
    
    EXPECT_TRUE(config.ReloadConfig("test_table"));
}

TEST_F(ConfigManagerTest, GlobalInstance) {
    ConfigManager config;
    config.Init(db_path_);
    config.LoadConfig("test_table");
    
    EXPECT_NE(g_config_manager, nullptr);
    EXPECT_EQ(CONFIG_GET_INT("test_table", 1, "value", 0), 100);
}
