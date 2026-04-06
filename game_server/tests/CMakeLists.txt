cmake_minimum_required(VERSION 3.10)
project(game_server_tests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GAME_SERVER_ROOT ${CMAKE_SOURCE_DIR}/..)

find_package(GTest REQUIRED)

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

link_directories(
    ${GAME_SERVER_ROOT}/lib
    ${GAME_SERVER_ROOT}/../ancfl/lib
    /usr/local/lib
    /usr/lib64/mysql
)

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
    test_account_server.cc
    test_proxy_message_forwarding.cc
)

add_executable(game_server_tests ${TEST_SRCS})

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

add_test(NAME game_server_tests COMMAND game_server_tests)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../bin)

# 单独的代理服务器消息转发测试
add_executable(test_proxy_message_forwarding test_proxy_message_forwarding.cc)
target_link_libraries(test_proxy_message_forwarding
    game_common
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

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# TCP 服务基础测试
add_executable(test_tcp_service_basic test_tcp_service_basic.cc)
target_link_libraries(test_tcp_service_basic
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
    /root/ancfl/ancfl/lib/libancfl.so
)

# 原始 TCP 通信测试
add_executable(test_tcp_raw test_tcp_raw.cc)
target_link_libraries(test_tcp_raw
    GTest::GTest
    GTest::Main
    pthread
)

# Socket 测试
add_executable(test_tcp_socket test_tcp_socket.cc)
target_link_libraries(test_tcp_socket
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
    /root/ancfl/ancfl/lib/libancfl.so
)

# 自定义 Socket 测试
add_executable(test_tcp_custom_socket test_tcp_custom_socket.cc)
target_link_libraries(test_tcp_custom_socket
    game_common
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
    /root/ancfl/ancfl/lib/libancfl.so
)

# 原始 TCP 简单测试
add_executable(test_tcp_raw_simple test_tcp_raw_simple.cc)
target_link_libraries(test_tcp_raw_simple
    pthread
)

# 直接 Socket 测试
add_executable(test_socket_direct test_socket_direct.cc)
target_link_libraries(test_socket_direct
    game_common
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
    /root/ancfl/ancfl/lib/libancfl.so
)

# Socket 修复测试
add_executable(test_socket_fix test_socket_fix.cc)
target_link_libraries(test_socket_fix
    game_common
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
    /root/ancfl/ancfl/lib/libancfl.so
)

# TCP 服务器测试（独立可执行文件）
add_executable(test_tcp_server test_tcp_server.cc)
target_link_libraries(test_tcp_server
    game_common
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
    /root/ancfl/ancfl/lib/libancfl.so
)

# TCP 客户端测试（独立可执行文件）
add_executable(test_tcp_client test_tcp_client.cc)
target_link_libraries(test_tcp_client
    game_common
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
    /root/ancfl/ancfl/lib/libancfl.so
)

# 客户端登录测试（独立可执行文件）
add_executable(test_client_login test_client_login.cc)
target_link_libraries(test_client_login
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
    /root/ancfl/ancfl/lib/libancfl.so
)
