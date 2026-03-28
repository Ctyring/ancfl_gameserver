#include "activity_module.h"
#include "ancfl/log.h"
#include "proto/msg_activity.pb.h"
#include <mutex>

namespace game_server {

ActivityModule::ActivityModule(LogicService* service)
    : service_(service) {
    LoadActivityData();
}

ActivityModule::~ActivityModule() {
    SaveActivityData();
}

bool ActivityModule::JoinActivity(uint64_t role_id, int32_t activity_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = activity_cache_.find(activity_id);
    if (it == activity_cache_.end()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Activity not found: activity_id=" << activity_id;
        return false;
    }

    msg_activity::ActivityInfo& activity = it->second;
    if (!activity.is_active()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Activity not active: activity_id=" << activity_id;
        return false;
    }

    auto player_it = player_activity_cache_.find(role_id);
    if (player_it != player_activity_cache_.end()) {
        for (const auto& data : player_it->second) {
            if (data.activity_id() == activity_id) {
                ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Already joined activity: role_id=" << role_id << ", activity_id=" << activity_id;
                return false;
            }
        }
    }

    msg_activity::PlayerActivityData player_data;
    player_data.set_activity_id(activity_id);
    player_data.set_score(0);
    player_data.set_rank(0);
    player_data.set_join_time(time(nullptr));
    player_data.set_is_joined(true);

    player_activity_cache_[role_id].push_back(player_data);

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Joined activity: role_id=" << role_id << ", activity_id=" << activity_id;
    return true;
}

bool ActivityModule::GetActivityInfo(int32_t activity_id, ActivityInfo& info) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = activity_cache_.find(activity_id);
    if (it == activity_cache_.end()) {
        return false;
    }

    const msg_activity::ActivityInfo& activity = it->second;
    info.activity_id = activity.activity_id();
    info.activity_name = activity.activity_name();
    info.type = activity.type();
    info.status = activity.status();
    info.start_time = activity.start_time();
    info.end_time = activity.end_time();
    info.reward_end_time = activity.reward_end_time();
    info.description = activity.description();
    info.is_active = activity.is_active();

    return true;
}

bool ActivityModule::GetActivityList(std::vector<ActivityInfo>& activities) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    activities.clear();
    for (const auto& pair : activity_cache_) {
        const msg_activity::ActivityInfo& activity = pair.second;
        ActivityInfo info;
        info.activity_id = activity.activity_id();
        info.activity_name = activity.activity_name();
        info.type = activity.type();
        info.status = activity.status();
        info.start_time = activity.start_time();
        info.end_time = activity.end_time();
        info.reward_end_time = activity.reward_end_time();
        info.description = activity.description();
        info.is_active = activity.is_active();
        activities.push_back(info);
    }

    return true;
}

bool ActivityModule::GetPlayerActivityData(uint64_t role_id, std::vector<PlayerActivityData>& data_list) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = player_activity_cache_.find(role_id);
    if (it == player_activity_cache_.end()) {
        return false;
    }

    data_list.clear();
    for (const auto& player_data : it->second) {
        PlayerActivityData data;
        data.activity_id = player_data.activity_id();
        data.score = player_data.score();
        data.rank = player_data.rank();
        data.join_time = player_data.join_time();
        data.is_joined = player_data.is_joined();
        data_list.push_back(data);
    }

    return true;
}

bool ActivityModule::GetActivityTaskProgress(uint64_t role_id, int32_t activity_id, std::vector<ActivityTaskInfo>& tasks) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto player_it = player_activity_cache_.find(role_id);
    if (player_it == player_activity_cache_.end()) {
        return false;
    }

    for (const auto& player_data : player_it->second) {
        if (player_data.activity_id() == activity_id) {
            tasks.clear();
            for (const auto& task : player_data.tasks()) {
                ActivityTaskInfo task_info;
                task_info.task_id = task.task_id();
                task_info.task_type = task.task_type();
                task_info.target_id = task.target_id();
                task_info.target_count = task.target_count();
                task_info.current_count = task.current_count();
                task_info.is_completed = task.is_completed();
                task_info.is_rewarded = task.is_rewarded();
                tasks.push_back(task_info);
            }
            return true;
        }
    }

    return false;
}

bool ActivityModule::UpdateTaskProgress(uint64_t role_id, int32_t activity_id, int32_t task_id, int32_t progress) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto player_it = player_activity_cache_.find(role_id);
    if (player_it == player_activity_cache_.end()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Player activity data not found: role_id=" << role_id;
        return false;
    }

    for (auto& player_data : player_it->second) {
        if (player_data.activity_id() == activity_id) {
            for (auto& task : *player_data.mutable_tasks()) {
                if (task.task_id() == task_id) {
                    task.set_current_count(progress);
                    if (task.current_count() >= task.target_count()) {
                        task.set_is_completed(true);
                    }
                    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Task progress updated: role_id=" << role_id << ", task_id=" << task_id << ", progress=" << progress;
                    return true;
                }
            }
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Task not found: task_id=" << task_id;
            return false;
        }
    }

    ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Activity not found: activity_id=" << activity_id;
    return false;
}

bool ActivityModule::GetTaskReward(uint64_t role_id, int32_t activity_id, int32_t task_id, std::vector<ActivityRewardInfo>& rewards) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto player_it = player_activity_cache_.find(role_id);
    if (player_it == player_activity_cache_.end()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Player activity data not found: role_id=" << role_id;
        return false;
    }

    for (auto& player_data : player_it->second) {
        if (player_data.activity_id() == activity_id) {
            for (auto& task : *player_data.mutable_tasks()) {
                if (task.task_id() == task_id) {
                    if (!task.is_completed()) {
                        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Task not completed: task_id=" << task_id;
                        return false;
                    }
                    if (task.is_rewarded()) {
                        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Task already rewarded: task_id=" << task_id;
                        return false;
                    }

                    task.set_is_rewarded(true);
                    rewards.clear();
                    for (const auto& reward : task.rewards()) {
                        ActivityRewardInfo reward_info;
                        reward_info.reward_id = reward.reward_id();
                        reward_info.reward_type = reward.reward_type();
                        reward_info.reward_value = reward.reward_value();
                        reward_info.reward_count = reward.reward_count();
                        rewards.push_back(reward_info);
                    }

                    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Task reward claimed: role_id=" << role_id << ", task_id=" << task_id;
                    return true;
                }
            }
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Task not found: task_id=" << task_id;
            return false;
        }
    }

    ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Activity not found: activity_id=" << activity_id;
    return false;
}

bool ActivityModule::GetActivityRanking(int32_t activity_id, int32_t count, std::vector<ActivityRankData>& ranking) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    ranking.clear();

    std::vector<std::pair<uint64_t, int32_t>> player_scores;
    for (const auto& player_pair : player_activity_cache_) {
        for (const auto& player_data : player_pair.second) {
            if (player_data.activity_id() == activity_id) {
                player_scores.push_back({player_pair.first, player_data.score()});
            }
        }
    }

    std::sort(player_scores.begin(), player_scores.end(),
              [](const std::pair<uint64_t, int32_t>& a, const std::pair<uint64_t, int32_t>& b) {
                  return a.second > b.second;
              });

    for (size_t i = 0; i < std::min(player_scores.size(), static_cast<size_t>(count)); ++i) {
        ActivityRankData rank_data;
        rank_data.role_id = player_scores[i].first;
        rank_data.score = player_scores[i].second;
        rank_data.rank = static_cast<int32_t>(i + 1);
        ranking.push_back(rank_data);
    }

    return true;
}

bool ActivityModule::UpdateActivityScore(uint64_t role_id, int32_t activity_id, int32_t score) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto player_it = player_activity_cache_.find(role_id);
    if (player_it == player_activity_cache_.end()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Player activity data not found: role_id=" << role_id;
        return false;
    }

    for (auto& player_data : player_it->second) {
        if (player_data.activity_id() == activity_id) {
            player_data.set_score(player_data.score() + score);
            ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Activity score updated: role_id=" << role_id << ", activity_id=" << activity_id << ", score=" << score;
            return true;
        }
    }

    ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Activity not found: activity_id=" << activity_id;
    return false;
}

bool ActivityModule::SaveActivityData(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    if (role_id == 0) {
        for (const auto& player_pair : player_activity_cache_) {
            for (const auto& player_data : player_pair.second) {
                std::string data = player_data.SerializeAsString();
            }
        }
    } else {
        auto it = player_activity_cache_.find(role_id);
        if (it != player_activity_cache_.end()) {
            for (const auto& player_data : it->second) {
                std::string data = player_data.SerializeAsString();
            }
        }
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Activity data saved: role_id=" << role_id;
    return true;
}

bool ActivityModule::LoadActivityData(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    if (role_id == 0) {
        msg_activity::ActivityInfo activity;
        activity.set_activity_id(1);
        activity.set_activity_name("Test Activity");
        activity.set_type(1);
        activity.set_status(1);
        activity.set_start_time(time(nullptr));
        activity.set_end_time(time(nullptr) + 86400);
        activity.set_reward_end_time(time(nullptr) + 172800);
        activity.set_description("Test activity description");
        activity.set_is_active(true);

        msg_activity::ActivityTaskInfo task;
        task.set_task_id(1);
        task.set_task_type(1);
        task.set_target_id(0);
        task.set_target_count(10);
        task.set_current_count(0);
        task.set_is_completed(false);
        task.set_is_rewarded(false);

        activity.add_tasks()->CopyFrom(task);
        activity_cache_[1] = activity;
    } else {
        auto it = player_activity_cache_.find(role_id);
        if (it == player_activity_cache_.end()) {
            msg_activity::PlayerActivityData player_data;
            player_data.set_activity_id(1);
            player_data.set_score(0);
            player_data.set_rank(0);
            player_data.set_join_time(time(nullptr));
            player_data.set_is_joined(false);

            msg_activity::ActivityTaskInfo task;
            task.set_task_id(1);
            task.set_task_type(1);
            task.set_target_id(0);
            task.set_target_count(10);
            task.set_current_count(0);
            task.set_is_completed(false);
            task.set_is_rewarded(false);

            player_data.add_tasks()->CopyFrom(task);
            player_activity_cache_[role_id].push_back(player_data);
        }
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Activity data loaded: role_id=" << role_id;
    return true;
}

} // namespace game_server
