#include "game_service_base.h"

namespace game_server {

GameServiceBase::GameServiceBase(const std::string& service_name)
    : TcpService()
    , service_name_(service_name)
    , service_id_(0)
    , stop_flag_(false)
    , timer_counter_(0)
    , timer5s_counter_(0)
    , timer30s_counter_(0) {
}

GameServiceBase::~GameServiceBase() {
}

void GameServiceBase::MainLoop() {
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << service_name_ << " started main loop";

    int64_t last_tick = ancfl::GetCurrentMs();
    const int64_t frame_interval = 50; // 20 FPS

    while (!stop_flag_) {
        int64_t now = ancfl::GetCurrentMs();
        int64_t elapsed = now - last_tick;

        if (elapsed >= frame_interval) {
            last_tick = now;

            // 每帧更新
            OnUpdate();
            Update();

            // 每秒定时器
            timer_counter_ += elapsed;
            if (timer_counter_ >= 1000) {
                timer_counter_ -= 1000;
                OnTimer();

                // 5秒定时器
                timer5s_counter_++;
                if (timer5s_counter_ >= 5) {
                    timer5s_counter_ = 0;
                    OnTimer5s();
                }

                // 30秒定时器
                timer30s_counter_++;
                if (timer30s_counter_ >= 30) {
                    timer30s_counter_ = 0;
                    OnTimer30s();
                }
            }
        }

        // 让出时间片
        ancfl::Fiber::YieldToHold();
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << service_name_ << " stopped main loop";
}

void GameServiceBase::StopService() {
    stop_flag_ = true;
    Stop();
}

} // namespace game_server
