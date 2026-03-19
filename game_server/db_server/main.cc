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

    // 初始化日志
    ancfl::Logger::Instance().Init("db_server");

    // 加载配置
    auto config = ancfl::Config::Instance().Load("conf/db_server.yml");
    if (!config) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT())("Failed to load config");
        return -1;
    }

    // 创建数据库服务
    auto db_service = DBService::Instance();

    // 设置主IOManager（用于网络IO）
    db_service->SetIOManager(&iom);

    // 设置工作线程池（用于后台任务）
    db_service->SetWorkerPool(worker.get());

    // 初始化服务
    if (!db_service->InitService()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT())("Failed to init DBService");
        return -1;
    }

    // 启动服务
    db_service->Start();

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
