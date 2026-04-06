#include "ancfl/ancfl.h"
#include "ancfl/config.h"
#include "ancfl/log.h"
#include "db_service.h"

using namespace game_server;

int main(int argc, char* argv[]) {
    // 设置时区
    setenv("TZ", ":/etc/localtime", 1);
    tzset();
    srand(time(0));

    // 初始化ancfl
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Starting database server...";
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Initializing IO manager...";
    auto iom = std::make_shared<ancfl::IOManager>(4, false, "main");
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "IO manager initialized";

    // 创建工作线程池
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Creating worker IO manager...";
    auto worker = std::make_shared<ancfl::IOManager>(4, false, "worker");
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Worker IO manager created";

    // 启动工作线程池
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Starting worker IO manager...";
    worker->start();
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Worker IO manager started";

    // 创建数据库服务
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Creating DBService...";
    auto db_service = std::make_shared<DBService>();
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "DBService created";

    // 设置IO管理器
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Setting IO manager for DBService...";
    db_service->SetIOManager(worker.get(), worker.get());
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "IO manager set";

    // 初始化服务
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Initializing DBService...";
    if (!db_service->InitService()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to init DBService";
        return -1;
    }
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "DBService initialized successfully";

    // 启动主循环
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Starting main loop...";
    std::atomic<bool> main_loop_exited(false);
    iom->schedule([db_service, iom, &main_loop_exited]() {
        ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Main loop coroutine started";
        db_service->MainLoop();
        ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Main loop exited";
        main_loop_exited = true;
        // 当 MainLoop 退出时，停止 IO 管理器
        ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Stopping IO manager...";
        iom->stop();
        ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "IO manager stopped";
    });
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Main loop coroutine scheduled";

    // 启动IO管理器
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Starting main IO manager...";
    iom->start();
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Main IO manager started";

    // 等待主循环退出
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Waiting for main loop to exit...";
    while (!main_loop_exited) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Main loop exited";

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Database server exited";
    return 0;
}
