#include "login_service.h"
#include "ancfl/ancfl.h"

using namespace game_server;

int main(int argc, char** argv) {
    // 设置时区
    setenv("TZ", ":/etc/localtime", 1);
    tzset();
    srand(time(0));

    // 初始化ancfl
    ancfl::IOManager iom(1);
    
    // 创建工作线程池
    ancfl::IOManager::ptr worker(new ancfl::IOManager(4, false, "worker"));

    // 获取登录服务实例
    LoginService* service = LoginService::GetInstance();

    // 初始化服务
    if (!service->InitService()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to initialize LoginServer";
        return -1;
    }

    // 启动服务
    service->Run();

    // 启动主循环
    iom.schedule([service]() {
        service->MainLoop();
    });

    // 运行IO管理器
    iom.start();

    return 0;
}
