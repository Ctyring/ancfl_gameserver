#include "monitor_server.h"
#include <iostream>
#include <csignal>

using namespace game_server;

MonitorServer* g_monitor_server = nullptr;

void SignalHandler(int signal) {
    if (g_monitor_server) {
        std::cout << "Received signal " << signal << ", stopping server..." << std::endl;
        g_monitor_server->Stop();
    }
}

int main(int argc, char* argv[]) {
    std::string config_file = "conf/monitor_server.yaml";
    if (argc > 1) {
        config_file = argv[1];
    }
    
    MonitorServer server;
    g_monitor_server = &server;
    
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
    
    if (!server.Init(config_file)) {
        std::cerr << "Failed to initialize monitor server" << std::endl;
        return 1;
    }
    
    if (!server.Start()) {
        std::cerr << "Failed to start monitor server" << std::endl;
        return 1;
    }
    
    std::cout << "Monitor server started, web interface available..." << std::endl;
    
    while (server.IsRunning()) {
        server.OnTimer();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    std::cout << "Monitor server stopped" << std::endl;
    return 0;
}
