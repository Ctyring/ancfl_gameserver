#include "proxy_service.h"
#include "ancfl/log.h"
#include "ancfl/config.h"

using namespace game_server;

int main(int argc, char* argv[]) {
    // 初始化日志
    ancfl::Logger::Instance().Init("proxy_server");
    
    // 加载配置
    auto config = ancfl::Config::Instance().Load("conf/proxy_server.yml");
    if (!config) {
        LOG_ERROR("Failed to load config");
        return -1;
    }
    
    // 创建网关服务
    auto proxy_service = ProxyService::Instance();
    
    // 初始化服务
    if (!proxy_service->InitService()) {
        LOG_ERROR("Failed to init ProxyService");
        return -1;
    }
    
    // 启动服务
    proxy_service->Start();
    
    // 主循环
    proxy_service->MainLoop();
    
    // 停止服务
    proxy_service->StopService();
    
    // 反初始化服务
    proxy_service->UninitService();
    
    return 0;
}
