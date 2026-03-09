#include "center_server.h"
#include <iostream>
#include <csignal>

using namespace game_server;

CenterServer* g_center_server = nullptr;

void SignalHandler(int signal) {
    if (g_center_server) {
        std::cout << "Received signal " << signal << ", stopping server..." << std::endl;
        g_center_server->Stop();
    }
}

int main(int argc, char* argv[]) {
    std::string config_file = "conf/center_server.yaml";
    if (argc > 1) {
        config_file = argv[1];
    }
    
    CenterServer server;
    g_center_server = &server;
    
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
    
    if (!server.Init(config_file)) {
        std::cerr << "Failed to initialize center server" << std::endl;
        return 1;
    }
    
    if (!server.Start()) {
        std::cerr << "Failed to start center server" << std::endl;
        return 1;
    }
    
    std::cout << "Center server started, waiting for connections..." << std::endl;
    
    while (server.IsRunning()) {
        server.OnTimer();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    std::cout << "Center server stopped" << std::endl;
    return 0;
}
