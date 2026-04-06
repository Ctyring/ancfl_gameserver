#include "ancfl/ancfl.h"
#include "ancfl/config.h"
#include "ancfl/log.h"
#include "monitor_service.h"

using namespace game_server;

int main(int argc, char* argv[]) {
    // 设置时区
    setenv("TZ", ":/etc/localtime", 1);
    tzset();
    srand(time(0));

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Starting monitor server...";

    // 初始化ancfl
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Initializing IO manager...";
    auto iom = std::make_shared<ancfl::IOManager>(2, false, "service");
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "IO manager initialized";
    
    // 启动IO管理器
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Starting IO manager...";
    iom->start();
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "IO manager started";

    // 初始化日志
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Initializing logger...";
    auto logger = ancfl::LoggerMgr::GetInstance()->getLogger("monitor_server");
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Logger initialized";

    // 加载配置
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Loading configuration...";
    ancfl::Config::LoadFromConfDir("conf");
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Configuration loaded";

    // 创建监控服务
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Creating MonitorService...";
    auto monitor_service = std::make_shared<MonitorService>();
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "MonitorService created";

    // 设置主IOManager（用于网络IO）
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Setting IO manager for MonitorService...";
    monitor_service->SetIOManager(iom.get(), iom.get());
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "IO manager set";

    // 初始化服务
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Initializing MonitorService...";
    if (!monitor_service->InitService()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to init MonitorService";
        return -1;
    }
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "MonitorService initialized successfully";

    // 启动服务
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Starting MonitorService...";
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "MonitorService started";

    // 启动主循环
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Starting main loop...";
    std::atomic<bool> main_loop_exited(false);
    iom->schedule([monitor_service, iom, &main_loop_exited]() {
        ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Main loop coroutine started";
        monitor_service->MainLoop();
        ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Main loop exited";
        main_loop_exited = true;
        // 当 MainLoop 退出时，停止 IO 管理器
        ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Stopping IO manager...";
        iom->stop();
        ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "IO manager stopped";
    });
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Main loop coroutine scheduled";

    // 等待主循环退出
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Waiting for main loop to exit...";
    while (!main_loop_exited) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Main loop exited";

    return 0;
}
