#ifndef __ACTIVITY_MODULE_H__
#define __ACTIVITY_MODULE_H__

#include <unordered_map>
#include <vector>

namespace game_server {

class LogicService;

// 活动状态
enum class ActivityStatus {
    NOT_STARTED = 0,  // 未开始
    IN_PROGRESS = 1,  // 进行中
    ENDED = 2,        // 已结束
    REWARDING = 3     // 领奖中
};

// 活动类型
enum class ActivityType {
    LOGIN = 1,     // 登录活动
    RECHARGE = 2,  // 充值活动
    CONSUME = 3,   // 消费活动
    KILL = 4,      // 击杀活动
    COLLECT = 5,   // 收集活动
    RANKING = 6,   // 排行榜活动
    INSTANCE = 7,  // 副本活动
    GUILD = 8      // 公会活动
};

// 活动奖励
struct ActivityReward {
    int32_t reward_id;
    int32_t reward_type;
    int32_t reward_value;
    int32_t reward_count;
};

// 活动任务
struct ActivityTask {
    int32_t task_id;
    int32_t task_type;
    int32_t target_id;
    int32_t target_count;
    int32_t current_count;
    bool is_completed;
    bool is_rewarded;
    std::vector<ActivityReward> rewards;
};

// 活动信息
struct ActivityInfo {
    int32_t activity_id;
    std::string activity_name;
    ActivityType type;
    ActivityStatus status;
    time_t start_time;
    time_t end_time;
    time_t reward_end_time;
    std::string description;
    std::vector<ActivityTask> tasks;
    bool is_active;
};

// 玩家活动数据
struct PlayerActivityData {
    int32_t activity_id;
    std::vector<ActivityTask> tasks;
    int32_t score;
    int32_t rank;
    time_t join_time;
    bool is_joined;
};

// 排行榜数据
struct ActivityRankData {
    uint64_t role_id;
    std::string role_name;
    int32_t score;
    int32_t rank;
    time_t update_time;
};

// 活动模块类
class ActivityModule {
   public:
    ActivityModule(LogicService* service);
    ~ActivityModule();

    // 活动管理
    bool InitActivities();
    bool GetActivityList(std::vector<ActivityInfo>& activities);
    bool GetActivityInfo(int32_t activity_id, ActivityInfo& info);
    bool IsActivityActive(int32_t activity_id);

    // 玩家活动数据
    bool InitPlayerActivity(uint64_t role_id);
    bool GetPlayerActivities(uint64_t role_id,
                             std::vector<PlayerActivityData>& activities);
    bool GetPlayerActivityData(uint64_t role_id,
                               int32_t activity_id,
                               PlayerActivityData& data);

    // 活动参与
    bool JoinActivity(uint64_t role_id, int32_t activity_id);
    bool IsJoined(uint64_t role_id, int32_t activity_id);

    // 任务进度
    bool UpdateTaskProgress(uint64_t role_id,
                            int32_t activity_id,
                            int32_t task_id,
                            int32_t progress);
    bool AddTaskProgress(uint64_t role_id,
                         ActivityType type,
                         int32_t target_id,
                         int32_t count);
    bool CompleteTask(uint64_t role_id, int32_t activity_id, int32_t task_id);

    // 领取奖励
    bool GetTaskReward(uint64_t role_id, int32_t activity_id, int32_t task_id);
    bool GetAllRewards(uint64_t role_id, int32_t activity_id);
    bool CanGetReward(uint64_t role_id, int32_t activity_id, int32_t task_id);

    // 积分系统
    bool AddScore(uint64_t role_id, int32_t activity_id, int32_t score);
    bool GetScore(uint64_t role_id, int32_t activity_id, int32_t& score);

    // 排行榜
    bool UpdateRanking(int32_t activity_id);
    bool GetRanking(int32_t activity_id,
                    std::vector<ActivityRankData>& ranking,
                    int32_t count = 100);
    bool GetPlayerRank(uint64_t role_id, int32_t activity_id, int32_t& rank);

    // 活动事件处理
    void OnLogin(uint64_t role_id);
    void OnRecharge(uint64_t role_id, int32_t amount);
    void OnConsume(uint64_t role_id, int32_t amount);
    void OnKillMonster(uint64_t role_id, int32_t monster_id);
    void OnCollectItem(uint64_t role_id, int32_t item_id, int32_t count);
    void OnCompleteInstance(uint64_t role_id, int32_t instance_id);

    // 活动数据加载和保存
    bool LoadActivityData(uint64_t role_id);
    bool SaveActivityData(uint64_t role_id);

    // 定时处理
    void OnTimer();
    void OnActivityStart(int32_t activity_id);
    void OnActivityEnd(int32_t activity_id);

   private:
    // 检查活动时间
    bool CheckActivityTime(int32_t activity_id);

    // 更新活动状态
    void UpdateActivityStatus();

    // 发送活动通知
    void NotifyActivityStart(int32_t activity_id);
    void NotifyActivityEnd(int32_t activity_id);

    // 活动配置缓存
    std::unordered_map<int32_t, ActivityInfo> activity_configs_;

    // 玩家活动数据缓存
    std::unordered_map<uint64_t,
                       std::unordered_map<int32_t, PlayerActivityData>>
        player_activity_cache_;

    // 排行榜缓存
    std::unordered_map<int32_t, std::vector<ActivityRankData>> ranking_cache_;

    // 逻辑服务指针
    LogicService* service_;

    // 互斥锁
    std::mutex cache_mutex_;

    // 定时器ID
    int32_t timer_id_;
};

}  // namespace game_server

#endif  // __ACTIVITY_MODULE_H__
