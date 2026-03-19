#include "activity_module.h"
#include "proto/msg_activity.pb.h"

namespace game_server {

// 构造函数：初始化活动模块
ActivityModule::ActivityModule(LogicService* service)
    : service_(service), timer_id_(0) {
    InitActivities();
}

ActivityModule::~ActivityModule() {}

// 初始化活动配置
// 创建示例活动配置（实际应从配置文件或数据库加载）
bool ActivityModule::InitActivities() {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    // 创建示例登录活动：连续登录7天活动
    ActivityInfo login_activity;
    login_activity.activity_id = 1;
    login_activity.activity_name = "每日登录活动";
    login_activity.type = ActivityType::LOGIN;
    login_activity.status = ActivityStatus::IN_PROGRESS;
    login_activity.start_time = time(nullptr);
    login_activity.end_time = time(nullptr) + 7 * 86400;  // 持续7天
    login_activity.reward_end_time =
        login_activity.end_time + 86400;  // 领奖期1天
    login_activity.description = "连续登录7天可获得丰厚奖励";
    login_activity.is_active = true;

    // 创建7天的登录任务
    for (int32_t i = 1; i <= 7; ++i) {
        ActivityTask task;
        task.task_id = i;
        task.task_type = 1;     // 登录类型
        task.target_id = 0;     // 通用目标
        task.target_count = i;  // 第i天需要累计登录i次
        task.current_count = 0;
        task.is_completed = false;
        task.is_rewarded = false;

        // 设置任务奖励
        ActivityReward reward;
        reward.reward_id = i;
        reward.reward_type = 2;        // 物品类型
        reward.reward_id = 1000 + i;   // 物品ID
        reward.reward_count = i * 10;  // 物品数量
        task.rewards.push_back(reward);

        login_activity.tasks.push_back(task);
    }

    activity_configs_[login_activity.activity_id] = login_activity;

    LOG_INFO("Activities initialized: count=%d", activity_configs_.size());
    return true;
}

// 获取所有活跃活动列表
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

// 获取指定活动的详细信息
bool ActivityModule::GetActivityInfo(int32_t activity_id, ActivityInfo& info) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = activity_configs_.find(activity_id);
    if (it == activity_configs_.end()) {
        return false;
    }

    info = it->second;
    return true;
}

// 检查活动是否处于活跃状态
// 活跃条件：is_active=true 且 status=IN_PROGRESS
bool ActivityModule::IsActivityActive(int32_t activity_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = activity_configs_.find(activity_id);
    if (it == activity_configs_.end()) {
        return false;
    }

    return it->second.is_active &&
           it->second.status == ActivityStatus::IN_PROGRESS;
}

// 初始化玩家活动数据
// 为玩家创建所有活跃活动的数据副本
bool ActivityModule::InitPlayerActivity(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    // 检查是否已初始化
    auto it = player_activity_cache_.find(role_id);
    if (it != player_activity_cache_.end()) {
        return true;
    }

    // 创建玩家活动数据容器
    player_activity_cache_[role_id] =
        std::unordered_map<int32_t, PlayerActivityData>();

    // 为每个活跃活动初始化玩家数据
    for (const auto& pair : activity_configs_) {
        if (pair.second.is_active) {
            PlayerActivityData data;
            data.activity_id = pair.first;
            data.tasks = pair.second.tasks;  // 复制任务列表
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

// 获取玩家的所有活动数据
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

// 获取玩家指定活动的数据
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

// 玩家参与活动
bool ActivityModule::JoinActivity(uint64_t role_id, int32_t activity_id) {
    // 先检查活动是否活跃
    if (!IsActivityActive(activity_id)) {
        LOG_ERROR("Activity not active: activity_id=%d", activity_id);
        return false;
    }

    std::lock_guard<std::mutex> lock(cache_mutex_);

    // 确保玩家数据容器存在
    auto it = player_activity_cache_.find(role_id);
    if (it == player_activity_cache_.end()) {
        player_activity_cache_[role_id] =
            std::unordered_map<int32_t, PlayerActivityData>();
        it = player_activity_cache_.find(role_id);
    }

    // 查找或创建活动数据
    auto activity_it = it->second.find(activity_id);
    if (activity_it == it->second.end()) {
        // 活动数据不存在，从配置创建
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
        // 活动数据已存在，更新参与状态
        activity_it->second.is_joined = true;
        activity_it->second.join_time = time(nullptr);
    }

    LOG_INFO("Activity joined: role_id=%llu, activity_id=%d", role_id,
             activity_id);
    return true;
}

// 检查玩家是否已参与活动
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

// 设置任务进度（直接设置）
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

    // 查找并更新任务进度
    for (auto& task : activity_it->second.tasks) {
        if (task.task_id == task_id) {
            task.current_count = progress;
            // 检查是否达到目标
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

// 增加任务进度（核心方法）
// 根据活动类型自动匹配所有相关活动并增加进度
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

    // 遍历玩家所有活动
    for (auto& pair : it->second) {
        // 获取活动配置
        auto config_it = activity_configs_.find(pair.first);
        if (config_it == activity_configs_.end()) {
            continue;
        }

        // 检查活动类型和状态是否匹配
        if (config_it->second.type != type ||
            config_it->second.status != ActivityStatus::IN_PROGRESS) {
            continue;
        }

        // 遍历活动中的所有任务
        for (auto& task : pair.second.tasks) {
            // target_id=0 表示通用目标，匹配所有
            // target_id匹配表示特定目标
            if (task.target_id == target_id || task.target_id == 0) {
                task.current_count += count;
                // 检查是否完成
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

// 手动完成任务（GM命令或特殊处理）
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

// 领取单个任务奖励
bool ActivityModule::GetTaskReward(uint64_t role_id,
                                   int32_t activity_id,
                                   int32_t task_id) {
    // 先检查是否可以领取
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
            // 发放所有奖励
            for (const auto& reward : task.rewards) {
                // TODO: 集成背包模块发放奖励
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

// 一键领取所有可领取的奖励
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

    // 遍历所有任务，领取已完成未领取的奖励
    for (auto& task : activity_it->second.tasks) {
        if (task.is_completed && !task.is_rewarded) {
            // 发放奖励
            for (const auto& reward : task.rewards) {
                // TODO: 集成背包模块发放奖励
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

// 检查是否可以领取奖励
// 条件：任务已完成且未领取
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

// 添加积分
// 同时更新排行榜数据
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

    // 更新玩家积分
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
        // 新增排行榜条目
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

// 获取玩家积分
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

// 更新排行榜
// 按分数降序排序并更新排名
bool ActivityModule::UpdateRanking(int32_t activity_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = ranking_cache_.find(activity_id);
    if (it == ranking_cache_.end()) {
        return false;
    }

    // 按分数降序排序
    std::sort(it->second.begin(), it->second.end(),
              [](const ActivityRankData& a, const ActivityRankData& b) {
                  return a.score > b.score;
              });

    // 更新排名（从1开始）
    for (size_t i = 0; i < it->second.size(); ++i) {
        it->second[i].rank = i + 1;
    }

    LOG_INFO("Activity ranking updated: activity_id=%d, count=%d", activity_id,
             it->second.size());
    return true;
}

// 获取排行榜数据
bool ActivityModule::GetRanking(int32_t activity_id,
                                std::vector<ActivityRankData>& ranking,
                                int32_t count) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = ranking_cache_.find(activity_id);
    if (it == ranking_cache_.end()) {
        return false;
    }

    ranking.clear();
    // 返回前count名
    int32_t actual_count =
        std::min(count, static_cast<int32_t>(it->second.size()));
    for (int32_t i = 0; i < actual_count; ++i) {
        ranking.push_back(it->second[i]);
    }

    return true;
}

// 获取玩家排名
bool ActivityModule::GetPlayerRank(uint64_t role_id,
                                   int32_t activity_id,
                                   int32_t& rank) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = ranking_cache_.find(activity_id);
    if (it == ranking_cache_.end()) {
        return false;
    }

    // 遍历查找玩家排名
    for (const auto& rank_data : it->second) {
        if (rank_data.role_id == role_id) {
            rank = rank_data.rank;
            return true;
        }
    }

    return false;
}

// ========== 事件触发接口 ==========
// 这些方法由其他模块调用，触发活动进度更新

// 玩家登录事件
void ActivityModule::OnLogin(uint64_t role_id) {
    AddTaskProgress(role_id, ActivityType::LOGIN, 0, 1);
}

// 玩家充值事件
void ActivityModule::OnRecharge(uint64_t role_id, int32_t amount) {
    AddTaskProgress(role_id, ActivityType::RECHARGE, 0, amount);
}

// 玩家消费事件
void ActivityModule::OnConsume(uint64_t role_id, int32_t amount) {
    AddTaskProgress(role_id, ActivityType::CONSUME, 0, amount);
}

// 击杀怪物事件
void ActivityModule::OnKillMonster(uint64_t role_id, int32_t monster_id) {
    AddTaskProgress(role_id, ActivityType::KILL, monster_id, 1);
}

// 收集物品事件
void ActivityModule::OnCollectItem(uint64_t role_id,
                                   int32_t item_id,
                                   int32_t count) {
    AddTaskProgress(role_id, ActivityType::COLLECT, item_id, count);
}

// 完成副本事件
void ActivityModule::OnCompleteInstance(uint64_t role_id, int32_t instance_id) {
    AddTaskProgress(role_id, ActivityType::INSTANCE, instance_id, 1);
}

// ========== 数据持久化 ==========

// 从数据库加载活动数据
bool ActivityModule::LoadActivityData(uint64_t role_id) {
    // TODO: 实现从数据库加载
    InitPlayerActivity(role_id);
    return true;
}

// 保存活动数据到数据库
bool ActivityModule::SaveActivityData(uint64_t role_id) {
    // TODO: 实现保存到数据库
    return true;
}

// ========== 定时处理 ==========

// 定时器回调
void ActivityModule::OnTimer() {
    UpdateActivityStatus();
}

// 活动开始回调
void ActivityModule::OnActivityStart(int32_t activity_id) {
    NotifyActivityStart(activity_id);
    LOG_INFO("Activity started: activity_id=%d", activity_id);
}

// 活动结束回调
void ActivityModule::OnActivityEnd(int32_t activity_id) {
    NotifyActivityEnd(activity_id);
    LOG_INFO("Activity ended: activity_id=%d", activity_id);
}

// 更新活动状态
// 根据当前时间更新所有活动的状态
void ActivityModule::UpdateActivityStatus() {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    time_t now = time(nullptr);

    for (auto& pair : activity_configs_) {
        ActivityStatus old_status = pair.second.status;

        // 根据时间判断状态
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

        // 状态变化时触发回调
        if (old_status != pair.second.status) {
            // 活动开始：NOT_STARTED -> IN_PROGRESS
            if (old_status == ActivityStatus::NOT_STARTED &&
                pair.second.status == ActivityStatus::IN_PROGRESS) {
                OnActivityStart(pair.first);
            }
            // 活动结束：IN_PROGRESS -> REWARDING
            else if (old_status == ActivityStatus::IN_PROGRESS &&
                     pair.second.status == ActivityStatus::REWARDING) {
                OnActivityEnd(pair.first);
            }
        }
    }
}

// 发送活动开始通知（待实现）
void ActivityModule::NotifyActivityStart(int32_t activity_id) {
    // TODO: 发送活动开始通知给所有在线玩家
}

// 发送活动结束通知（待实现）
void ActivityModule::NotifyActivityEnd(int32_t activity_id) {
    // TODO: 发送活动结束通知给所有在线玩家
}

}  // namespace game_server
