#include "center_server.h"
#include "ancfl/ancfl.h"
#include <iostream>
#include <csignal>

using namespace game_server;

std::shared_ptr<CenterServer> g_center_server = nullptr;
std::shared_ptr<ancfl::IOManager> g_iom = nullptr;

void SignalHandler(int signal) {
    if (g_center_server) {
        std::cout << "Received signal " << signal << ", stopping server..." << std::endl;
        g_center_server->StopService();
    }
    if (g_iom) {
        g_iom->stop();
    }
}

int main(int argc, char* argv[]) {
    // 创建 IOManager，使用2个线程
    g_iom = std::make_shared<ancfl::IOManager>(2);
    
    // 使用 shared_ptr 管理 CenterServer 对象
    g_center_server = std::make_shared<CenterServer>();
    
    // 设置 IOManager
    g_center_server->SetIOManager(g_iom.get(), g_iom.get());
    
    // 注册信号处理
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
    
    // 初始化服务
    if (!g_center_server->InitService()) {
        std::cerr << "Failed to initialize center server" << std::endl;
        return 1;
    }
    
    std::cout << "Center server started, waiting for connections..." << std::endl;
    
    // 添加一个定时器，防止IOManager因为没有事件而退出
    g_iom->addTimer(1000, []() {
        // 空回调，只是为了保持IOManager运行
    }, true);
    
    // 启动 IOManager 事件循环
    g_iom->start();
    
    // 等待IOManager退出
    g_iom->stop();
    
    // 反初始化服务
    if (g_center_server) {
        g_center_server->UninitService();
    }
    
    std::cout << "Center server stopped" << std::endl;
    return 0;
}
