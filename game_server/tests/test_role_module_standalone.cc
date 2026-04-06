#include "logic_server/role_module.h"
#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <memory>

namespace game_server {

// 测试断言宏
#define TEST_BEGIN(name) \
    do { \
        std::cout << std::endl; \
        std::cout << "========================================" << std::endl; \
        std::cout << "[开始] " << name << std::endl; \
    } while(0)

#define TEST_END(name, passed) \
    do { \
        if (passed) { \
            std::cout << "[通过] " << name << std::endl; \
        } else { \
            std::cout << "[失败] " << name << std::endl; \
            test_failed++; \
        } \
        test_total++; \
    } while(0)

#define ASSERT_TRUE(condition, msg) \
    do { \
        if (!(condition)) { \
            std::cout << "[断言失败] " << msg << std::endl; \
            return false; \
        } \
    } while(0)

#define ASSERT_FALSE(condition, msg) \
    do { \
        if (condition) { \
            std::cout << "[断言失败] " << msg << std::endl; \
            return false; \
        } \
    } while(0)

#define ASSERT_EQ(expected, actual, msg) \
    do { \
        if ((expected) != (actual)) { \
            std::cout << "[断言失败] " << msg << " - 期望: " << (expected) << ", 实际: " << (actual) << std::endl; \
            return false; \
        } \
    } while(0)

#define ASSERT_FLOAT_EQ(expected, actual, msg) \
    do { \
        float diff = (expected) - (actual); \
        if (diff < 0) diff = -diff; \
        if (diff > 0.001f) { \
            std::cout << "[断言失败] " << msg << " - 期望: " << (expected) << ", 实际: " << (actual) << std::endl; \
            return false; \
        } \
    } while(0)

// 全局测试统计
int test_total = 0;
int test_failed = 0;

// 测试角色创建
bool TestCreateRole() {
    TEST_BEGIN("创建角色");
    
    RoleModule module(nullptr);
    
    uint64_t role_id = 0;
    bool result = module.CreateRole(
        1001, 
        "测试角色", 
        1, 
        1, 
        role_id
    );
    
    ASSERT_TRUE(result, "角色创建应该成功");
    ASSERT_TRUE(role_id > 0, "角色ID应该大于0");
    
    std::cout << "[信息] 创建的角色ID: " << role_id << std::endl;
    
    TEST_END("创建角色", true);
    return true;
}

// 测试获取角色信息
bool TestGetRoleInfo() {
    TEST_BEGIN("获取角色信息");
    
    RoleModule module(nullptr);
    
    uint64_t role_id = 0;
    module.CreateRole(1001, "测试角色", 1, 1, role_id);
    
    RoleData info;
    bool result = module.GetRoleInfo(role_id, info);
    
    ASSERT_TRUE(result, "获取角色信息应该成功");
    ASSERT_EQ(role_id, info.role_id, "角色ID应该匹配");
    ASSERT_EQ(std::string("测试角色"), info.role_name, "角色名称应该匹配");
    ASSERT_EQ(1, info.level, "角色初始等级应该为1");
    ASSERT_EQ(0, info.exp, "角色初始经验应该为0");
    ASSERT_EQ(10000, info.gold, "初始金币应该为10000");
    ASSERT_EQ(1000, info.diamond, "初始钻石应该为1000");
    
    TEST_END("获取角色信息", true);
    return true;
}

// 测试获取不存在的角色
bool TestGetNonExistentRole() {
    TEST_BEGIN("获取不存在的角色");
    
    RoleModule module(nullptr);
    
    RoleData info;
    bool result = module.GetRoleInfo(99999, info);
    
    ASSERT_FALSE(result, "获取不存在的角色应该失败");
    
    TEST_END("获取不存在的角色", true);
    return true;
}

// 测试设置角色等级
bool TestSetRoleLevel() {
    TEST_BEGIN("设置角色等级");
    
    RoleModule module(nullptr);
    
    uint64_t role_id = 0;
    module.CreateRole(1001, "测试角色", 1, 1, role_id);
    
    bool result = module.AddRoleLevel(role_id, 10);
    
    ASSERT_TRUE(result, "设置角色等级应该成功");
    
    RoleData info;
    module.GetRoleInfo(role_id, info);
    
    ASSERT_EQ(11, info.level, "角色等级应该从1增加到11");
    
    RoleProperty property;
    module.GetRoleProperty(role_id, property);
    
    ASSERT_EQ(1000 + 11 * 100, property.hp, "HP应该随等级增长");
    ASSERT_EQ(100 + 11 * 10, property.attack, "攻击应该随等级增长");
    
    std::cout << "[信息] 角色等级: " << info.level << std::endl;
    std::cout << "[信息] 角色HP: " << property.hp << std::endl;
    std::cout << "[信息] 角色攻击: " << property.attack << std::endl;
    
    TEST_END("设置角色等级", true);
    return true;
}

// 测试增加经验值
bool TestAddExp() {
    TEST_BEGIN("增加经验值");
    
    RoleModule module(nullptr);
    
    uint64_t role_id = 0;
    module.CreateRole(1001, "测试角色", 1, 1, role_id);
    
    // 添加经验但不升级
    bool result = module.AddRoleExp(role_id, 500);
    
    ASSERT_TRUE(result, "添加经验应该成功");
    
    RoleData info;
    module.GetRoleInfo(role_id, info);
    
    ASSERT_EQ(1, info.level, "等级应该保持为1");
    ASSERT_EQ(500, info.exp, "经验应该为500");
    
    std::cout << "[信息] 经验: " << info.exp << ", 等级: " << info.level << std::endl;
    
    TEST_END("增加经验值", true);
    return true;
}

// 测试升级
bool TestLevelUp() {
    TEST_BEGIN("升级测试");
    
    RoleModule module(nullptr);
    
    uint64_t role_id = 0;
    module.CreateRole(1001, "测试角色", 1, 1, role_id);
    
    // 1级升2级需要1000经验
    bool result = module.AddRoleExp(role_id, 1500);
    
    ASSERT_TRUE(result, "添加经验应该成功");
    
    RoleData info;
    module.GetRoleInfo(role_id, info);
    
    ASSERT_EQ(2, info.level, "等级应该升到2级");
    ASSERT_EQ(500, info.exp, "剩余经验应该为500 (1500-1000)");
    
    std::cout << "[信息] 经验: " << info.exp << ", 等级: " << info.level << std::endl;
    
    TEST_END("升级测试", true);
    return true;
}

// 测试连续升级
bool TestMultiLevelUp() {
    TEST_BEGIN("连续升级测试");
    
    RoleModule module(nullptr);
    
    uint64_t role_id = 0;
    module.CreateRole(1001, "测试角色", 1, 1, role_id);
    
    // 添加足够的经验连续升级
    // 1级: 0-1000, 2级: 0-2000, 3级: 0-3000
    // 总计: 1000 + 2000 + 1500 = 4500
    bool result = module.AddRoleExp(role_id, 4500);
    
    ASSERT_TRUE(result, "添加经验应该成功");
    
    RoleData info;
    module.GetRoleInfo(role_id, info);
    
    ASSERT_EQ(3, info.level, "等级应该升到3级");
    ASSERT_EQ(1500, info.exp, "剩余经验应该为1500");
    
    std::cout << "[信息] 经验: " << info.exp << ", 等级: " << info.level << std::endl;
    
    RoleProperty property;
    module.GetRoleProperty(role_id, property);
    
    ASSERT_EQ(1000 + 3 * 100, property.hp, "HP应该是1300");
    ASSERT_EQ(100 + 3 * 10, property.attack, "攻击应该是130");
    
    TEST_END("连续升级测试", true);
    return true;
}

// 测试删除角色
bool TestDeleteRole() {
    TEST_BEGIN("删除角色");
    
    RoleModule module(nullptr);
    
    uint64_t role_id = 0;
    module.CreateRole(1001, "测试角色", 1, 1, role_id);
    
    // 先验证角色存在
    RoleData info;
    bool exists = module.GetRoleInfo(role_id, info);
    ASSERT_TRUE(exists, "角色应该存在");
    
    // 删除角色
    bool result = module.DeleteRole(role_id);
    ASSERT_TRUE(result, "删除角色应该成功");
    
    // 验证角色不存在
    exists = module.GetRoleInfo(role_id, info);
    ASSERT_FALSE(exists, "角色应该已被删除");
    
    TEST_END("删除角色", true);
    return true;
}

// 测试角色在线状态
bool TestRoleOnlineStatus() {
    TEST_BEGIN("角色在线状态");
    
    RoleModule module(nullptr);
    
    uint64_t role_id = 0;
    module.CreateRole(1001, "测试角色", 1, 1, role_id);
    
    // 初始应该是离线
    bool is_online = module.IsRoleOnline(role_id);
    ASSERT_FALSE(is_online, "初始状态应该是离线");
    
    // 设置为在线
    bool result = module.SetRoleOnline(role_id, true);
    ASSERT_TRUE(result, "设置在线应该成功");
    
    is_online = module.IsRoleOnline(role_id);
    ASSERT_TRUE(is_online, "状态应该是在线");
    
    // 设置为离线
    result = module.SetRoleOnline(role_id, false);
    ASSERT_TRUE(result, "设置离线应该成功");
    
    is_online = module.IsRoleOnline(role_id);
    ASSERT_FALSE(is_online, "状态应该是离线");
    
    TEST_END("角色在线状态", true);
    return true;
}

// 测试角色属性
bool TestRoleProperty() {
    TEST_BEGIN("角色属性");
    
    RoleModule module(nullptr);
    
    uint64_t role_id = 0;
    module.CreateRole(1001, "测试角色", 1, 1, role_id);
    
    RoleProperty property;
    bool result = module.GetRoleProperty(role_id, property);
    
    ASSERT_TRUE(result, "获取角色属性应该成功");
    ASSERT_EQ(1100, property.hp, "HP应该是1100 (1000 + 1*100)");
    ASSERT_EQ(1100, property.max_hp, "最大HP应该等于HP");
    ASSERT_EQ(550, property.mp, "MP应该是550 (500 + 1*50)");
    ASSERT_EQ(110, property.attack, "攻击应该是110 (100 + 1*10)");
    ASSERT_EQ(55, property.defense, "防御应该是55 (50 + 1*5)");
    ASSERT_EQ(22, property.armor, "护甲应该是22 (20 + 1*2)");
    ASSERT_EQ(22, property.magic_resist, "魔抗应该是22 (20 + 1*2)");
    
    std::cout << "[信息] HP: " << property.hp << ", 攻击: " << property.attack << ", 防御: " << property.defense << std::endl;
    
    TEST_END("角色属性", true);
    return true;
}

// 测试角色位置
bool TestRolePosition() {
    TEST_BEGIN("角色位置");
    
    RoleModule module(nullptr);
    
    uint64_t role_id = 0;
    module.CreateRole(1001, "测试角色", 1, 1, role_id);
    
    // 设置位置
    RolePosition new_pos;
    new_pos.scene_id = 2001;
    new_pos.x = 100.5f;
    new_pos.y = 200.5f;
    new_pos.z = 300.5f;
    new_pos.rotation_y = 90.0f;
    
    bool result = module.UpdateRolePosition(role_id, new_pos);
    ASSERT_TRUE(result, "更新位置应该成功");
    
    // 获取位置
    RolePosition pos;
    result = module.GetRolePosition(role_id, pos);
    ASSERT_TRUE(result, "获取位置应该成功");
    
    ASSERT_EQ(2001, pos.scene_id, "场景ID应该匹配");
    ASSERT_FLOAT_EQ(100.5f, pos.x, "X坐标应该匹配");
    ASSERT_FLOAT_EQ(200.5f, pos.y, "Y坐标应该匹配");
    ASSERT_FLOAT_EQ(300.5f, pos.z, "Z坐标应该匹配");
    ASSERT_FLOAT_EQ(90.0f, pos.rotation_y, "旋转角度应该匹配");
    
    std::cout << "[信息] 场景: " << pos.scene_id << ", 位置: (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
    
    TEST_END("角色位置", true);
    return true;
}

// 运行所有测试
void RunAllTests() {
    std::cout << "========================================" << std::endl;
    std::cout << "      角色模块独立测试" << std::endl;
    std::cout << "========================================" << std::endl;
    
    test_total = 0;
    test_failed = 0;
    
    TestCreateRole();
    TestGetRoleInfo();
    TestGetNonExistentRole();
    TestDeleteRole();
    TestSetRoleLevel();
    TestAddExp();
    TestLevelUp();
    TestMultiLevelUp();
    TestRoleOnlineStatus();
    TestRoleProperty();
    TestRolePosition();
    
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "测试结果统计：" << std::endl;
    std::cout << "  总测试数: " << test_total << std::endl;
    std::cout << "  通过数: " << (test_total - test_failed) << std::endl;
    std::cout << "  失败数: " << test_failed << std::endl;
    std::cout << "  通过率: " << (test_total > 0 ? (test_total - test_failed) * 100 / test_total : 0) << "%" << std::endl;
    std::cout << "========================================" << std::endl;
}

} // namespace game_server

int main() {
    game_server::RunAllTests();
    return 0;
}
