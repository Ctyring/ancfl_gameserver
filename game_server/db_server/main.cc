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
    ancfl::IOManager iom(1);

    // 创建工作线程池
    ancfl::IOManager::ptr worker(new ancfl::IOManager(4, false, "worker"));

    // 创建数据库服务
    auto db_service = std::make_shared<DBService>();

    // 初始化服务
    if (!db_service->InitService()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to init DBService";
        return -1;
    }

    // 启动服务
    db_service->Run();

    // 启动工作线程池
    worker->start();

    // 启动主循环
    iom.schedule([db_service]() { db_service->MainLoop(); });

    // 运行IO管理器
    iom.start();

    // 停止工作线程池
    worker->stop();

    return 0;
}
