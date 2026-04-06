#include "ancfl/ancfl.h"
#include "ancfl/config.h"
#include "ancfl/log.h"
#include "logic_service.h"

using namespace game_server;

int main(int argc, char* argv[]) {
    // 设置时区
    setenv("TZ", ":/etc/localtime", 1);
    tzset();
    srand(time(0));

    // 解析命令行参数
    int32_t server_id = 1;
    if (argc > 1) {
        server_id = std::stoi(argv[1]);
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Starting logic server " << server_id << "...";

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
    auto logger = ancfl::LoggerMgr::GetInstance()->getLogger("logic_server");
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Logger initialized";

    // 加载配置
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Loading configuration...";
    ancfl::Config::LoadFromConfDir("conf");
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Configuration loaded";

    // 创建逻辑服务
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Creating LogicService for server " << server_id << "...";
    auto logic_service = std::make_shared<LogicService>(server_id);
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "LogicService created";

    // 设置主IOManager（用于网络IO）
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Setting IO manager for LogicService...";
    logic_service->SetIOManager(iom.get(), iom.get());
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "IO manager set";

    // 初始化服务
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Initializing LogicService...";
    if (!logic_service->InitService()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to init LogicService";
        return -1;
    }
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "LogicService initialized successfully";

    // 启动服务
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Starting LogicService...";
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "LogicService started";

    // 启动主循环
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Starting main loop...";
    std::atomic<bool> main_loop_exited(false);
    iom->schedule([logic_service, iom, &main_loop_exited]() {
        ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Main loop coroutine started";
        logic_service->MainLoop();
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
