#include "logic_service.h"
#include "ancfl/log.h"
#include "ancfl/config.h"

using namespace game_server;

int main(int argc, char* argv[]) {
    // 初始化日志
    ancfl::Logger::Instance().Init("logic_server");
    
    // 加载配置
    auto config = ancfl::Config::Instance().Load("conf/logic_server.yml");
    if (!config) {
        LOG_ERROR("Failed to load config");
        return -1;
    }
    
    // 创建逻辑服务
    auto logic_service = LogicService::Instance();
    
    // 初始化服务
    if (!logic_service->InitService()) {
        LOG_ERROR("Failed to init LogicService");
        return -1;
    }
    
    // 启动服务
    logic_service->Start();
    
    // 主循环
    logic_service->MainLoop();
    
    // 停止服务
    logic_service->StopService();
    
    // 反初始化服务
    logic_service->UninitService();
    
    return 0;
}
