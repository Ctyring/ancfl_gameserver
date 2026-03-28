cmake_minimum_required(VERSION 3.10)
project(game_server_tests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置游戏服务器根目录
set(GAME_SERVER_ROOT ${CMAKE_SOURCE_DIR}/..)

# 查找GTest
find_package(GTest REQUIRED)

# 包含目录
include_directories(
    ${GAME_SERVER_ROOT}
    ${GAME_SERVER_ROOT}/common
    ${GAME_SERVER_ROOT}/proto
    ${GAME_SERVER_ROOT}/center_server
    ${GAME_SERVER_ROOT}/login_server
    ${GAME_SERVER_ROOT}/db_server
    ${GAME_SERVER_ROOT}/logic_server
    ${GAME_SERVER_ROOT}/proxy_server
    ${GAME_SERVER_ROOT}/account_server
    ${GAME_SERVER_ROOT}/monitor_server
    ${GAME_SERVER_ROOT}/log_server
    ${GAME_SERVER_ROOT}/..
    ${GTEST_INCLUDE_DIRS}
)

# 链接目录
link_directories(
    ${GAME_SERVER_ROOT}/lib
    ${GAME_SERVER_ROOT}/../ancfl/lib
    /usr/local/lib
    /usr/lib64/mysql
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
    mysqlclient
    protobuf
    ssl
    crypto
    jsoncpp
    yaml-cpp
    event
    hiredis_vip
    jemalloc
    dl
    z
    ancfl
)

# 添加测试
add_test(NAME game_server_tests COMMAND game_server_tests)

# 设置输出目录
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
