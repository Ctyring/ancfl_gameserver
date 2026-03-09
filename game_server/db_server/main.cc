#include "db_service.h"
#include "ancfl/log.h"
#include "ancfl/config.h"

using namespace game_server;

int main(int argc, char* argv[]) {
    // 初始化日志
    ancfl::Logger::Instance().Init("db_server");
    
    // 加载配置
    auto config = ancfl::Config::Instance().Load("conf/db_server.yml");
    if (!config) {
        LOG_ERROR("Failed to load config");
        return -1;
    }
    
    // 创建数据服务
    auto db_service = DBService::Instance();
    
    // 初始化服务
    if (!db_service->InitService()) {
        LOG_ERROR("Failed to init DBService");
        return -1;
    }
    
    // 启动服务
    db_service->Start();
    
    // 主循环
    db_service->MainLoop();
    
    // 停止服务
    db_service->StopService();
    
    // 反初始化服务
    db_service->UninitService();
    
    return 0;
}
