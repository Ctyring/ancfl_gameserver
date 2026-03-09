#include "log_server.h"
#include <iostream>
#include <csignal>

using namespace game_server;

LogServer* g_log_server_instance = nullptr;

void SignalHandler(int signal) {
    if (g_log_server_instance) {
        std::cout << "Received signal " << signal << ", stopping server..." << std::endl;
        g_log_server_instance->Stop();
    }
}

int main(int argc, char* argv[]) {
    std::string config_file = "conf/log_server.yaml";
    if (argc > 1) {
        config_file = argv[1];
    }
    
    LogServer server;
    g_log_server_instance = &server;
    
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
    
    if (!server.Init(config_file)) {
        std::cerr << "Failed to initialize log server" << std::endl;
        return 1;
    }
    
    if (!server.Start()) {
        std::cerr << "Failed to start log server" << std::endl;
        return 1;
    }
    
    std::cout << "Log server started, waiting for log records..." << std::endl;
    
    while (server.IsRunning()) {
        server.OnTimer();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    std::cout << "Log server stopped" << std::endl;
    return 0;
}
