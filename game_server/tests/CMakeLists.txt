cmake_minimum_required(VERSION 3.10)
project(game_server_tests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找GTest
find_package(GTest REQUIRED)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/proto
    ${GTEST_INCLUDE_DIRS}
)

# 测试源文件
set(TEST_SRCS
    test_main.cc
    test_message_dispatcher.cc
    test_tcp_service.cc
    test_shared_memory.cc
    test_config_manager.cc
    test_role_module.cc
    test_bag_module.cc
    test_equip_module.cc
    test_scene_module.cc
    test_skill_module.cc
    test_buff_module.cc
    test_task_module.cc
    test_mail_module.cc
    test_friend_module.cc
    test_shop_module.cc
    test_guild_module.cc
    test_activity_module.cc
    test_center_server.cc
    test_log_server.cc
    test_monitor_server.cc
)

# 创建测试可执行文件
add_executable(game_server_tests ${TEST_SRCS})

# 链接库
target_link_libraries(game_server_tests
    game_common
    GTest::GTest
    GTest::Main
    pthread
    sqlite3
)

# 添加测试
add_test(NAME game_server_tests COMMAND game_server_tests)

# 设置输出目录
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
