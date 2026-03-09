cmake_minimum_required(VERSION 3.0)

project(game_server)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)
include(../cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)

# 编译选项
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O2 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

# 包含目录
include_directories(.)
include_directories(..)
include_directories(/yaml-cpp/include)
include_directories(/usr/local/include)
link_directories(/yaml-cpp/lib)

# 查找依赖包
find_package(Protobuf REQUIRED)
if(Protobuf_FOUND)
    include_directories(${Protobuf_INCLUDE_DIRS})
endif()

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

find_package(MySQL REQUIRED)
if(MYSQL_FOUND)
    include_directories(${MYSQL_INCLUDE_DIR})
endif()

find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIR})
endif()

# Protobuf生成
set(PROTO_FILES
    proto/msg_id.proto
    proto/msg_base.proto
    proto/msg_account.proto
    proto/msg_role.proto
    proto/msg_bag.proto
    proto/msg_equip.proto
    proto/msg_battle.proto
)

set(PROTO_SRCS)
foreach(proto ${PROTO_FILES})
    get_filename_component(proto_name ${proto} NAME_WE)
    list(APPEND PROTO_SRCS 
        ${CMAKE_CURRENT_SOURCE_DIR}/proto/${proto_name}.pb.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/proto/${proto_name}.pb.h
    )
endforeach()

# 自定义命令生成protobuf代码
add_custom_command(
    OUTPUT ${PROTO_SRCS}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE} --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/proto -I ${CMAKE_CURRENT_SOURCE_DIR}/proto ${PROTO_FILES}
    DEPENDS ${PROTO_FILES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# 公共库源文件
set(COMMON_SRCS
    common/message_dispatcher.cc
    common/tcp_service.cc
    common/game_service_base.cc
    common/shared_memory.cc
    common/config_manager.cc
    ${PROTO_SRCS}
)

# 创建公共库
add_library(game_common STATIC ${COMMON_SRCS})
force_redefine_file_macro_for_sources(game_common)

# 链接库
set(LIBS
    ancfl
    dl
    pthread
    yaml-cpp
    ${PROTOBUF_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    libjsoncpp_static.a
    ${ZLIB_LIBRARIES}
    sqlite3
    tinyxml2
    mysqlclient
    mysqlclient_r
    zookeeper_mt
    event
    hiredis_vip
    jemalloc
)

# 登录服务器
add_executable(login_server
    login_server/main.cc
    login_server/login_service.cc
)
force_redefine_file_macro_for_sources(login_server)
target_link_libraries(login_server game_common ${LIBS})

# 账号服务器
add_executable(account_server
    account_server/main.cc
    account_server/account_service.cc
)
force_redefine_file_macro_for_sources(account_server)
target_link_libraries(account_server game_common ${LIBS})

# 逻辑服务器
add_executable(logic_server
    logic_server/main.cc
    logic_server/logic_service.cc
    logic_server/role_module.cc
    logic_server/bag_module.cc
    logic_server/equip_module.cc
    logic_server/scene_module.cc
    logic_server/skill_module.cc
    logic_server/buff_module.cc
    logic_server/task_module.cc
    logic_server/mail_module.cc
    logic_server/friend_module.cc
    logic_server/shop_module.cc
    logic_server/guild_module.cc
    logic_server/activity_module.cc
)
force_redefine_file_macro_for_sources(logic_server)
target_link_libraries(logic_server game_common ${LIBS})

# 数据服务器
add_executable(db_server
    db_server/main.cc
    db_server/db_service.cc
)
force_redefine_file_macro_for_sources(db_server)
target_link_libraries(db_server game_common ${LIBS})

# 网关服务器
add_executable(proxy_server
    proxy_server/main.cc
    proxy_server/proxy_service.cc
)
force_redefine_file_macro_for_sources(proxy_server)
target_link_libraries(proxy_server game_common ${LIBS})

# 中心服务器
add_executable(center_server
    center_server/main.cc
    center_server/center_server.cc
)
force_redefine_file_macro_for_sources(center_server)
target_link_libraries(center_server game_common ${LIBS})

# 日志服务器
add_executable(log_server
    log_server/main.cc
    log_server/log_server.cc
)
force_redefine_file_macro_for_sources(log_server)
target_link_libraries(log_server game_common ${LIBS})

# 监控服务器
add_executable(monitor_server
    monitor_server/main.cc
    monitor_server/monitor_server.cc
)
force_redefine_file_macro_for_sources(monitor_server)
target_link_libraries(monitor_server game_common ${LIBS})

# 设置输出目录
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
