#include "activity_module.h"
#include "proto/msg_activity.pb.h"

namespace game_server {

ActivityModule::ActivityModule(LogicService* service)
    : service_(service), timer_id_(0) {
    // 初始化活动配置
    InitActivities();
}

ActivityModule::~ActivityModule() {}

bool ActivityModule::InitActivities() {
    // TODO: 从配置文件加载活动配置
    // 这里简单示例

    std::lock_guard<std::mutex> lock(cache_mutex_);

    // 创建示例登录活动
    ActivityInfo login_activity;
    login_activity.activity_id = 1;
    login_activity.activity_name = "每日登录活动";
    login_activity.type = ActivityType::LOGIN;
    login_activity.status = ActivityStatus::IN_PROGRESS;
    login_activity.start_time = time(nullptr);
    login_activity.end_time = time(nullptr) + 7 * 86400;  // 7天
    login_activity.reward_end_time = login_activity.end_time + 86400;
    login_activity.description = "连续登录7天可获得丰厚奖励";
    login_activity.is_active = true;

    // 添加任务
    for (int32_t i = 1; i <= 7; ++i) {
        ActivityTask task;
        task.task_id = i;
        task.task_type = 1;  // 登录
        task.target_id = 0;
        task.target_count = i;
        task.current_count = 0;
        task.is_completed = false;
        task.is_rewarded = false;

        ActivityReward reward;
        reward.reward_id = i;
        reward.reward_type = 2;  // 物品
        reward.reward_id = 1000 + i;
        reward.reward_count = i * 10;
        task.rewards.push_back(reward);

        login_activity.tasks.push_back(task);
    }

    activity_configs_[login_activity.activity_id] = login_activity;

    LOG_INFO("Activities initialized: count=%d", activity_configs_.size());
    return true;
}

bool ActivityModule::GetActivityList(std::vector<ActivityInfo>& activities) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    activities.clear();
    for (const auto& pair : activity_configs_) {
        if (pair.second.is_active) {
            activities.push_back(pair.second);
        }
    }

    return true;
}

bool ActivityModule::GetActivityInfo(int32_t activity_id, ActivityInfo& info) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = activity_configs_.find(activity_id);
    if (it == activity_configs_.end()) {
        return false;
    }

    info = it->second;
    return true;
}

bool ActivityModule::IsActivityActive(int32_t activity_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = activity_configs_.find(activity_id);
    if (it == activity_configs_.end()) {
        return false;
    }

    return it->second.is_active &&
           it->second.status == ActivityStatus::IN_PROGRESS;
}

bool ActivityModule::InitPlayerActivity(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = player_activity_cache_.find(role_id);
    if (it != player_activity_cache_.end()) {
        return true;
    }

    // 初始化所有活动数据
    player_activity_cache_[role_id] =
        std::unordered_map<int32_t, PlayerActivityData>();

    for (const auto& pair : activity_configs_) {
        if (pair.second.is_active) {
            PlayerActivityData data;
            data.activity_id = pair.first;
            data.tasks = pair.second.tasks;
            data.score = 0;
            data.rank = 0;
            data.join_time = 0;
            data.is_joined = false;

            player_activity_cache_[role_id][pair.first] = data;
        }
    }

    LOG_INFO("Player activities initialized: role_id=%llu", role_id);
    return true;
}

bool ActivityModule::GetPlayerActivities(
    uint64_t role_id,
    std::vector<PlayerActivityData>& activities) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = player_activity_cache_.find(role_id);
    if (it == player_activity_cache_.end()) {
        return false;
    }

    activities.clear();
    for (const auto& pair : it->second) {
        activities.push_back(pair.second);
    }

    return true;
}

bool ActivityModule::GetPlayerActivityData(uint64_t role_id,
                                           int32_t activity_id,
                                           PlayerActivityData& data) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = player_activity_cache_.find(role_id);
    if (it == player_activity_cache_.end()) {
        return false;
    }

    auto activity_it = it->second.find(activity_id);
    if (activity_it == it->second.end()) {
        return false;
    }

    data = activity_it->second;
    return true;
}

bool ActivityModule::JoinActivity(uint64_t role_id, int32_t activity_id) {
    if (!IsActivityActive(activity_id)) {
        LOG_ERROR("Activity not active: activity_id=%d", activity_id);
        return false;
    }

    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = player_activity_cache_.find(role_id);
    if (it == player_activity_cache_.end()) {
        player_activity_cache_[role_id] =
            std::unordered_map<int32_t, PlayerActivityData>();
        it = player_activity_cache_.find(role_id);
    }

    auto activity_it = it->second.find(activity_id);
    if (activity_it == it->second.end()) {
        // 创建活动数据
        auto config_it = activity_configs_.find(activity_id);
        if (config_it == activity_configs_.end()) {
            return false;
        }

        PlayerActivityData data;
        data.activity_id = activity_id;
        data.tasks = config_it->second.tasks;
        data.score = 0;
        data.rank = 0;
        data.join_time = time(nullptr);
        data.is_joined = true;

        it->second[activity_id] = data;
    } else {
        activity_it->second.is_joined = true;
        activity_it->second.join_time = time(nullptr);
    }

    LOG_INFO("Activity joined: role_id=%llu, activity_id=%d", role_id,
             activity_id);
    return true;
}

bool ActivityModule::IsJoined(uint64_t role_id, int32_t activity_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = player_activity_cache_.find(role_id);
    if (it == player_activity_cache_.end()) {
        return false;
    }

    auto activity_it = it->second.find(activity_id);
    if (activity_it == it->second.end()) {
        return false;
    }

    return activity_it->second.is_joined;
}

bool ActivityModule::UpdateTaskProgress(uint64_t role_id,
                                        int32_t activity_id,
                                        int32_t task_id,
                                        int32_t progress) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = player_activity_cache_.find(role_id);
    if (it == player_activity_cache_.end()) {
        return false;
    }

    auto activity_it = it->second.find(activity_id);
    if (activity_it == it->second.end()) {
        return false;
    }

    for (auto& task : activity_it->second.tasks) {
        if (task.task_id == task_id) {
            task.current_count = progress;
            if (task.current_count >= task.target_count && !task.is_completed) {
                task.is_completed = true;
                LOG_INFO(
                    "Activity task completed: role_id=%llu, activity_id=%d, "
                    "task_id=%d",
                    role_id, activity_id, task_id);
            }
            return true;
        }
    }

    return false;
}

bool ActivityModule::AddTaskProgress(uint64_t role_id,
                                     ActivityType type,
                                     int32_t target_id,
                                     int32_t count) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = player_activity_cache_.find(role_id);
    if (it == player_activity_cache_.end()) {
        return false;
    }

    bool updated = false;

    for (auto& pair : it->second) {
        auto config_it = activity_configs_.find(pair.first);
        if (config_it == activity_configs_.end()) {
            continue;
        }

        if (config_it->second.type != type ||
            config_it->second.status != ActivityStatus::IN_PROGRESS) {
            continue;
        }

        for (auto& task : pair.second.tasks) {
            if (task.target_id == target_id || task.target_id == 0) {
                task.current_count += count;
                if (task.current_count >= task.target_count &&
                    !task.is_completed) {
                    task.is_completed = true;
                    LOG_INFO(
                        "Activity task auto completed: role_id=%llu, "
                        "activity_id=%d, task_id=%d",
                        role_id, pair.first, task.task_id);
                }
                updated = true;
            }
        }
    }

    return updated;
}

bool ActivityModule::CompleteTask(uint64_t role_id,
                                  int32_t activity_id,
                                  int32_t task_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = player_activity_cache_.find(role_id);
    if (it == player_activity_cache_.end()) {
        return false;
    }

    auto activity_it = it->second.find(activity_id);
    if (activity_it == it->second.end()) {
        return false;
    }

    for (auto& task : activity_it->second.tasks) {
        if (task.task_id == task_id) {
            task.is_completed = true;
            return true;
        }
    }

    return false;
}

bool ActivityModule::GetTaskReward(uint64_t role_id,
                                   int32_t activity_id,
                                   int32_t task_id) {
    if (!CanGetReward(role_id, activity_id, task_id)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = player_activity_cache_.find(role_id);
    if (it == player_activity_cache_.end()) {
        return false;
    }

    auto activity_it = it->second.find(activity_id);
    if (activity_it == it->second.end()) {
        return false;
    }

    for (auto& task : activity_it->second.tasks) {
        if (task.task_id == task_id) {
            // 发放奖励
            for (const auto& reward : task.rewards) {
                // TODO: 根据奖励类型发放奖励
                LOG_INFO(
                    "Activity reward: role_id=%llu, reward_type=%d, "
                    "reward_id=%d, count=%d",
                    role_id, reward.reward_type, reward.reward_id,
                    reward.reward_count);
            }

            task.is_rewarded = true;
            LOG_INFO(
                "Activity task reward taken: role_id=%llu, activity_id=%d, "
                "task_id=%d",
                role_id, activity_id, task_id);
            return true;
        }
    }

    return false;
}

bool ActivityModule::GetAllRewards(uint64_t role_id, int32_t activity_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = player_activity_cache_.find(role_id);
    if (it == player_activity_cache_.end()) {
        return false;
    }

    auto activity_it = it->second.find(activity_id);
    if (activity_it == it->second.end()) {
        return false;
    }

    int32_t reward_count = 0;

    for (auto& task : activity_it->second.tasks) {
        if (task.is_completed && !task.is_rewarded) {
            // 发放奖励
            for (const auto& reward : task.rewards) {
                // TODO: 根据奖励类型发放奖励
                LOG_INFO(
                    "Activity reward: role_id=%llu, reward_type=%d, "
                    "reward_id=%d, count=%d",
                    role_id, reward.reward_type, reward.reward_id,
                    reward.reward_count);
            }

            task.is_rewarded = true;
            reward_count++;
        }
    }

    LOG_INFO(
        "All activity rewards taken: role_id=%llu, activity_id=%d, count=%d",
        role_id, activity_id, reward_count);
    return true;
}

bool ActivityModule::CanGetReward(uint64_t role_id,
                                  int32_t activity_id,
                                  int32_t task_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = player_activity_cache_.find(role_id);
    if (it == player_activity_cache_.end()) {
        return false;
    }

    auto activity_it = it->second.find(activity_id);
    if (activity_it == it->second.end()) {
        return false;
    }

    for (const auto& task : activity_it->second.tasks) {
        if (task.task_id == task_id) {
            return task.is_completed && !task.is_rewarded;
        }
    }

    return false;
}

bool ActivityModule::AddScore(uint64_t role_id,
                              int32_t activity_id,
                              int32_t score) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = player_activity_cache_.find(role_id);
    if (it == player_activity_cache_.end()) {
        return false;
    }

    auto activity_it = it->second.find(activity_id);
    if (activity_it == it->second.end()) {
        return false;
    }

    activity_it->second.score += score;

    // 更新排行榜
    auto ranking_it = ranking_cache_.find(activity_id);
    if (ranking_it == ranking_cache_.end()) {
        ranking_cache_[activity_id] = std::vector<ActivityRankData>();
        ranking_it = ranking_cache_.find(activity_id);
    }

    // 查找或添加排行榜数据
    bool found = false;
    for (auto& rank_data : ranking_it->second) {
        if (rank_data.role_id == role_id) {
            rank_data.score = activity_it->second.score;
            rank_data.update_time = time(nullptr);
            found = true;
            break;
        }
    }

    if (!found) {
        ActivityRankData rank_data;
        rank_data.role_id = role_id;
        rank_data.role_name = "";
        rank_data.score = activity_it->second.score;
        rank_data.rank = 0;
        rank_data.update_time = time(nullptr);
        ranking_it->second.push_back(rank_data);
    }

    LOG_INFO(
        "Activity score added: role_id=%llu, activity_id=%d, score=%d, "
        "total=%d",
        role_id, activity_id, score, activity_it->second.score);
    return true;
}

bool ActivityModule::GetScore(uint64_t role_id,
                              int32_t activity_id,
                              int32_t& score) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = player_activity_cache_.find(role_id);
    if (it == player_activity_cache_.end()) {
        return false;
    }

    auto activity_it = it->second.find(activity_id);
    if (activity_it == it->second.end()) {
        return false;
    }

    score = activity_it->second.score;
    return true;
}

bool ActivityModule::UpdateRanking(int32_t activity_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = ranking_cache_.find(activity_id);
    if (it == ranking_cache_.end()) {
        return false;
    }

    // 按分数排序
    std::sort(it->second.begin(), it->second.end(),
              [](const ActivityRankData& a, const ActivityRankData& b) {
                  return a.score > b.score;
              });

    // 更新排名
    for (size_t i = 0; i < it->second.size(); ++i) {
        it->second[i].rank = i + 1;
    }

    LOG_INFO("Activity ranking updated: activity_id=%d, count=%d", activity_id,
             it->second.size());
    return true;
}

bool ActivityModule::GetRanking(int32_t activity_id,
                                std::vector<ActivityRankData>& ranking,
                                int32_t count) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = ranking_cache_.find(activity_id);
    if (it == ranking_cache_.end()) {
        return false;
    }

    ranking.clear();
    int32_t actual_count =
        std::min(count, static_cast<int32_t>(it->second.size()));
    for (int32_t i = 0; i < actual_count; ++i) {
        ranking.push_back(it->second[i]);
    }

    return true;
}

bool ActivityModule::GetPlayerRank(uint64_t role_id,
                                   int32_t activity_id,
                                   int32_t& rank) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = ranking_cache_.find(activity_id);
    if (it == ranking_cache_.end()) {
        return false;
    }

    for (const auto& rank_data : it->second) {
        if (rank_data.role_id == role_id) {
            rank = rank_data.rank;
            return true;
        }
    }

    return false;
}

void ActivityModule::OnLogin(uint64_t role_id) {
    AddTaskProgress(role_id, ActivityType::LOGIN, 0, 1);
}

void ActivityModule::OnRecharge(uint64_t role_id, int32_t amount) {
    AddTaskProgress(role_id, ActivityType::RECHARGE, 0, amount);
}

void ActivityModule::OnConsume(uint64_t role_id, int32_t amount) {
    AddTaskProgress(role_id, ActivityType::CONSUME, 0, amount);
}

void ActivityModule::OnKillMonster(uint64_t role_id, int32_t monster_id) {
    AddTaskProgress(role_id, ActivityType::KILL, monster_id, 1);
}

void ActivityModule::OnCollectItem(uint64_t role_id,
                                   int32_t item_id,
                                   int32_t count) {
    AddTaskProgress(role_id, ActivityType::COLLECT, item_id, count);
}

void ActivityModule::OnCompleteInstance(uint64_t role_id, int32_t instance_id) {
    AddTaskProgress(role_id, ActivityType::INSTANCE, instance_id, 1);
}

bool ActivityModule::LoadActivityData(uint64_t role_id) {
    // TODO: 从数据库加载活动数据
    InitPlayerActivity(role_id);
    return true;
}

bool ActivityModule::SaveActivityData(uint64_t role_id) {
    // TODO: 保存活动数据到数据库
    return true;
}

void ActivityModule::OnTimer() {
    UpdateActivityStatus();
}

void ActivityModule::OnActivityStart(int32_t activity_id) {
    NotifyActivityStart(activity_id);
    LOG_INFO("Activity started: activity_id=%d", activity_id);
}

void ActivityModule::OnActivityEnd(int32_t activity_id) {
    NotifyActivityEnd(activity_id);
    LOG_INFO("Activity ended: activity_id=%d", activity_id);
}

void ActivityModule::UpdateActivityStatus() {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    time_t now = time(nullptr);

    for (auto& pair : activity_configs_) {
        ActivityStatus old_status = pair.second.status;

        if (now < pair.second.start_time) {
            pair.second.status = ActivityStatus::NOT_STARTED;
        } else if (now < pair.second.end_time) {
            pair.second.status = ActivityStatus::IN_PROGRESS;
        } else if (now < pair.second.reward_end_time) {
            pair.second.status = ActivityStatus::REWARDING;
        } else {
            pair.second.status = ActivityStatus::ENDED;
            pair.second.is_active = false;
        }

        // 状态变化通知
        if (old_status != pair.second.status) {
            if (old_status == ActivityStatus::NOT_STARTED &&
                pair.second.status == ActivityStatus::IN_PROGRESS) {
                OnActivityStart(pair.first);
            } else if (old_status == ActivityStatus::IN_PROGRESS &&
                       pair.second.status == ActivityStatus::REWARDING) {
                OnActivityEnd(pair.first);
            }
        }
    }
}

void ActivityModule::NotifyActivityStart(int32_t activity_id) {
    // TODO: 发送活动开始通知
}

void ActivityModule::NotifyActivityEnd(int32_t activity_id) {
    // TODO: 发送活动结束通知
}

}  // namespace game_server
