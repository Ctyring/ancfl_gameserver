#ifndef __ACTIVITY_MODULE_H__
#define __ACTIVITY_MODULE_H__

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <mutex>

namespace msg_activity {
    class ActivityInfo;
    class PlayerActivityData;
}

namespace game_server {

class LogicService;

struct ActivityInfo {
    int32_t activity_id;
    std::string activity_name;
    int32_t type;
    int32_t status;
    int64_t start_time;
    int64_t end_time;
    int64_t reward_end_time;
    std::string description;
    bool is_active;
};

struct ActivityTaskInfo {
    int32_t task_id;
    int32_t task_type;
    int32_t target_id;
    int32_t target_count;
    int32_t current_count;
    bool is_completed;
    bool is_rewarded;
};

struct ActivityRewardInfo {
    int32_t reward_id;
    int32_t reward_type;
    int32_t reward_value;
    int32_t reward_count;
};

struct PlayerActivityData {
    int32_t activity_id;
    int32_t score;
    int32_t rank;
    int64_t join_time;
    bool is_joined;
};

struct ActivityRankData {
    uint64_t role_id;
    std::string role_name;
    int32_t score;
    int32_t rank;
};

class ActivityModule {
public:
    explicit ActivityModule(LogicService* service);
    ~ActivityModule();

    bool JoinActivity(uint64_t role_id, int32_t activity_id);
    bool GetActivityInfo(int32_t activity_id, ActivityInfo& info);
    bool GetActivityList(std::vector<ActivityInfo>& activities);
    bool GetPlayerActivityData(uint64_t role_id, std::vector<PlayerActivityData>& data_list);
    bool GetActivityTaskProgress(uint64_t role_id, int32_t activity_id, std::vector<ActivityTaskInfo>& tasks);
    bool UpdateTaskProgress(uint64_t role_id, int32_t activity_id, int32_t task_id, int32_t progress);
    bool GetTaskReward(uint64_t role_id, int32_t activity_id, int32_t task_id, std::vector<ActivityRewardInfo>& rewards);
    bool GetActivityRanking(int32_t activity_id, int32_t count, std::vector<ActivityRankData>& ranking);
    bool UpdateActivityScore(uint64_t role_id, int32_t activity_id, int32_t score);

    bool SaveActivityData(uint64_t role_id = 0);
    bool LoadActivityData(uint64_t role_id = 0);

private:
    LogicService* service_;

    std::mutex cache_mutex_;

    std::map<int32_t, msg_activity::ActivityInfo> activity_cache_;
    std::map<uint64_t, std::vector<msg_activity::PlayerActivityData>> player_activity_cache_;
};

} // namespace game_server

#endif // __ACTIVITY_MODULE_H__
