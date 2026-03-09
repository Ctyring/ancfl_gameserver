#ifndef __TASK_MODULE_H__
#define __TASK_MODULE_H__

#include <unordered_map>
#include <vector>
#include "logic_service.h"

namespace game_server {

// 任务状态
enum class TaskStatus {
    NOT_ACCEPTED = 0,  // 未接受
    ACCEPTED = 1,      // 已接受
    COMPLETED = 2,     // 已完成
    SUBMITTED = 3      // 已提交
};

// 任务类型
enum class TaskType {
    MAIN = 1,        // 主线任务
    SUB = 2,         // 支线任务
    DAILY = 3,       // 日常任务
    WEEKLY = 4,      // 周常任务
    ACHIEVEMENT = 5  // 成就任务
};

// 任务条件类型
enum class TaskConditionType {
    KILL_MONSTER = 1,       // 击杀怪物
    COLLECT_ITEM = 2,       // 收集物品
    TALK_TO_NPC = 3,        // 与NPC对话
    REACH_LEVEL = 4,        // 达到等级
    REACH_POSITION = 5,     // 到达位置
    COMPLETE_INSTANCE = 6,  // 完成副本
    EQUIP_ITEM = 7,         // 穿戴装备
    SKILL_LEVEL = 8,        // 技能等级
    KILL_PLAYER = 9         // 击杀玩家
};

// 任务条件
struct TaskCondition {
    int32_t condition_type;
    int32_t target_id;
    int32_t target_count;
    int32_t current_count;
};

// 任务奖励
struct TaskReward {
    int32_t reward_type;
    int32_t reward_id;
    int32_t reward_count;
};

// 任务信息
struct TaskInfo {
    uint64_t task_id;
    int32_t task_config_id;
    TaskType type;
    TaskStatus status;
    std::vector<TaskCondition> conditions;
    std::vector<TaskReward> rewards;
    time_t accept_time;
    time_t complete_time;
    time_t submit_time;
    int32_t progress;
};

// 任务配置
struct TaskConfig {
    int32_t task_config_id;
    std::string task_name;
    TaskType type;
    int32_t level_requirement;
    int32_t pre_task_id;
    std::vector<TaskCondition> conditions;
    std::vector<TaskReward> rewards;
    std::string description;
};

// 日常任务信息
struct DailyTaskInfo {
    int32_t task_config_id;
    TaskStatus status;
    int32_t progress;
    time_t refresh_time;
};

// 任务模块类
class TaskModule {
   public:
    TaskModule(LogicService* service);
    ~TaskModule();

    // 任务管理
    bool InitTasks(uint64_t role_id);
    bool GetTasks(uint64_t role_id, std::vector<TaskInfo>& tasks);
    bool GetTaskInfo(uint64_t role_id, int32_t task_config_id, TaskInfo& info);

    // 任务接受
    bool AcceptTask(uint64_t role_id, int32_t task_config_id);
    bool CanAcceptTask(uint64_t role_id, int32_t task_config_id);

    // 任务完成检查
    bool CheckTaskComplete(uint64_t role_id, int32_t task_config_id);
    bool IsTaskCompleted(uint64_t role_id, int32_t task_config_id);

    // 任务提交
    bool SubmitTask(uint64_t role_id, int32_t task_config_id);
    bool GiveTaskRewards(uint64_t role_id, const TaskInfo& task);

    // 任务放弃
    bool AbandonTask(uint64_t role_id, int32_t task_config_id);

    // 任务进度更新
    bool UpdateTaskProgress(uint64_t role_id,
                            int32_t task_config_id,
                            int32_t condition_index,
                            int32_t progress);
    bool AddTaskProgress(uint64_t role_id,
                         TaskConditionType condition_type,
                         int32_t target_id,
                         int32_t count);

    // 日常任务
    bool RefreshDailyTasks(uint64_t role_id);
    bool GetDailyTasks(uint64_t role_id, std::vector<DailyTaskInfo>& tasks);
    bool AcceptDailyTask(uint64_t role_id, int32_t task_config_id);

    // 任务追踪
    bool TrackTask(uint64_t role_id, int32_t task_config_id);
    bool GetTrackedTask(uint64_t role_id, int32_t& task_config_id);

    // 任务事件处理
    void OnKillMonster(uint64_t role_id, int32_t monster_id);
    void OnCollectItem(uint64_t role_id, int32_t item_id, int32_t count);
    void OnReachLevel(uint64_t role_id, int32_t level);
    void OnCompleteInstance(uint64_t role_id, int32_t instance_id);
    void OnEquipItem(uint64_t role_id, int32_t item_id);
    void OnSkillUpgrade(uint64_t role_id, int32_t skill_id, int32_t level);

    // 任务数据加载和保存
    bool LoadTaskData(uint64_t role_id);
    bool SaveTaskData(uint64_t role_id);

   private:
    // 获取任务配置
    bool GetTaskConfig(int32_t task_config_id, TaskConfig& config);

    // 生成任务ID
    uint64_t GenerateTaskId();

    // 检查前置任务
    bool CheckPreTask(uint64_t role_id, int32_t pre_task_id);

    // 检查等级要求
    bool CheckLevelRequirement(uint64_t role_id, int32_t level);

    // 任务缓存
    std::unordered_map<uint64_t, std::vector<TaskInfo>> task_cache_;
    std::unordered_map<uint64_t, std::vector<DailyTaskInfo>> daily_task_cache_;
    std::unordered_map<int32_t, TaskConfig> task_configs_;
    std::unordered_map<uint64_t, int32_t> tracked_tasks_;

    // 逻辑服务指针
    LogicService* service_;

    // 互斥锁
    std::mutex cache_mutex_;
};

}  // namespace game_server

#endif  // __TASK_MODULE_H__
