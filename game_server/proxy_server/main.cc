#include "ancfl/ancfl.h"
#include "ancfl/config.h"
#include "ancfl/log.h"
#include "proxy_service.h"

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
    auto logger = ancfl::LoggerMgr::GetInstance()->getLogger("proxy_server");

    // 加载配置
    ancfl::Config::LoadFromConfDir("conf");

    // 创建网关服务
    auto proxy_service = std::make_shared<ProxyService>();

    // 设置主IOManager（用于网络IO）
    proxy_service->SetIOManager(&iom);

    // 设置工作线程池（用于后台任务）
    proxy_service->SetWorkerPool(worker.get());

    // 初始化服务
    if (!proxy_service->InitService()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Failed to init ProxyService";
        return -1;
    }

    // 启动服务
    proxy_service->Run();

    // 启动工作线程池
    worker->start();

    // 启动主循环
    iom.schedule([proxy_service]() mutable { proxy_service->MainLoop(); });

    // 运行IO管理器
    iom.start();

    // 停止工作线程池
    worker->stop();

    return 0;
}
