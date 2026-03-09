#include "task_module.h"
#include "proto/msg_task.pb.h"

namespace game_server {

TaskModule::TaskModule(LogicService* service) : service_(service) {
    // 初始化任务配置
    // TODO: 从配置文件加载任务配置
}

TaskModule::~TaskModule() {
}

bool TaskModule::InitTasks(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 检查任务是否已初始化
    auto it = task_cache_.find(role_id);
    if (it != task_cache_.end()) {
        return true;
    }
    
    // 初始化任务列表
    task_cache_[role_id] = std::vector<TaskInfo>();
    daily_task_cache_[role_id] = std::vector<DailyTaskInfo>();
    
    LOG_INFO("Tasks initialized: role_id=%llu", role_id);
    return true;
}

bool TaskModule::GetTasks(uint64_t role_id, std::vector<TaskInfo>& tasks) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = task_cache_.find(role_id);
    if (it == task_cache_.end()) {
        return false;
    }
    
    tasks = it->second;
    return true;
}

bool TaskModule::GetTaskInfo(uint64_t role_id, int32_t task_config_id, TaskInfo& info) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = task_cache_.find(role_id);
    if (it == task_cache_.end()) {
        return false;
    }
    
    for (const auto& task : it->second) {
        if (task.task_config_id == task_config_id) {
            info = task;
            return true;
        }
    }
    
    return false;
}

bool TaskModule::AcceptTask(uint64_t role_id, int32_t task_config_id) {
    // 检查是否可以接受任务
    if (!CanAcceptTask(role_id, task_config_id)) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 获取任务配置
    TaskConfig config;
    if (!GetTaskConfig(task_config_id, config)) {
        LOG_ERROR("Task config not found: task_config_id=%d", task_config_id);
        return false;
    }
    
    auto it = task_cache_.find(role_id);
    if (it == task_cache_.end()) {
        task_cache_[role_id] = std::vector<TaskInfo>();
        it = task_cache_.find(role_id);
    }
    
    // 创建任务信息
    TaskInfo task;
    task.task_id = GenerateTaskId();
    task.task_config_id = task_config_id;
    task.type = config.type;
    task.status = TaskStatus::ACCEPTED;
    task.conditions = config.conditions;
    task.rewards = config.rewards;
    task.accept_time = time(nullptr);
    task.complete_time = 0;
    task.submit_time = 0;
    task.progress = 0;
    
    it->second.push_back(task);
    
    LOG_INFO("Task accepted: role_id=%llu, task_config_id=%d, task_id=%llu", role_id, task_config_id, task.task_id);
    return true;
}

bool TaskModule::CanAcceptTask(uint64_t role_id, int32_t task_config_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 获取任务配置
    TaskConfig config;
    if (!GetTaskConfig(task_config_id, config)) {
        return false;
    }
    
    auto it = task_cache_.find(role_id);
    if (it != task_cache_.end()) {
        // 检查任务是否已接受或已完成
        for (const auto& task : it->second) {
            if (task.task_config_id == task_config_id) {
                if (task.status == TaskStatus::ACCEPTED || task.status == TaskStatus::COMPLETED) {
                    LOG_ERROR("Task already accepted or completed: role_id=%llu, task_config_id=%d", role_id, task_config_id);
                    return false;
                }
            }
        }
    }
    
    // 检查前置任务
    if (config.pre_task_id > 0) {
        if (!CheckPreTask(role_id, config.pre_task_id)) {
            LOG_ERROR("Pre task not completed: role_id=%llu, pre_task_id=%d", role_id, config.pre_task_id);
            return false;
        }
    }
    
    // 检查等级要求
    if (config.level_requirement > 0) {
        if (!CheckLevelRequirement(role_id, config.level_requirement)) {
            LOG_ERROR("Level requirement not met: role_id=%llu, required_level=%d", role_id, config.level_requirement);
            return false;
        }
    }
    
    return true;
}

bool TaskModule::CheckTaskComplete(uint64_t role_id, int32_t task_config_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = task_cache_.find(role_id);
    if (it == task_cache_.end()) {
        return false;
    }
    
    for (auto& task : it->second) {
        if (task.task_config_id == task_config_id && task.status == TaskStatus::ACCEPTED) {
            // 检查所有条件是否完成
            bool all_completed = true;
            for (const auto& condition : task.conditions) {
                if (condition.current_count < condition.target_count) {
                    all_completed = false;
                    break;
                }
            }
            
            if (all_completed) {
                task.status = TaskStatus::COMPLETED;
                task.complete_time = time(nullptr);
                LOG_INFO("Task completed: role_id=%llu, task_config_id=%d", role_id, task_config_id);
            }
            
            return all_completed;
        }
    }
    
    return false;
}

bool TaskModule::IsTaskCompleted(uint64_t role_id, int32_t task_config_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = task_cache_.find(role_id);
    if (it == task_cache_.end()) {
        return false;
    }
    
    for (const auto& task : it->second) {
        if (task.task_config_id == task_config_id) {
            return task.status == TaskStatus::COMPLETED || task.status == TaskStatus::SUBMITTED;
        }
    }
    
    return false;
}

bool TaskModule::SubmitTask(uint64_t role_id, int32_t task_config_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = task_cache_.find(role_id);
    if (it == task_cache_.end()) {
        return false;
    }
    
    for (auto& task : it->second) {
        if (task.task_config_id == task_config_id && task.status == TaskStatus::COMPLETED) {
            // 发放奖励
            if (!GiveTaskRewards(role_id, task)) {
                LOG_ERROR("Failed to give task rewards: role_id=%llu, task_config_id=%d", role_id, task_config_id);
                return false;
            }
            
            // 更新任务状态
            task.status = TaskStatus::SUBMITTED;
            task.submit_time = time(nullptr);
            
            LOG_INFO("Task submitted: role_id=%llu, task_config_id=%d", role_id, task_config_id);
            return true;
        }
    }
    
    LOG_ERROR("Task not completed or not found: role_id=%llu, task_config_id=%d", role_id, task_config_id);
    return false;
}

bool TaskModule::GiveTaskRewards(uint64_t role_id, const TaskInfo& task) {
    // TODO: 发放任务奖励
    // 根据奖励类型发放经验、金币、物品等
    
    for (const auto& reward : task.rewards) {
        switch (reward.reward_type) {
        case 1: // 经验
            // TODO: 增加角色经验
            LOG_INFO("Task reward - Exp: role_id=%llu, exp=%d", role_id, reward.reward_count);
            break;
        case 2: // 金币
            // TODO: 增加角色金币
            LOG_INFO("Task reward - Gold: role_id=%llu, gold=%d", role_id, reward.reward_count);
            break;
        case 3: // 物品
            // TODO: 添加物品到背包
            LOG_INFO("Task reward - Item: role_id=%llu, item_id=%d, count=%d", role_id, reward.reward_id, reward.reward_count);
            break;
        default:
            break;
        }
    }
    
    return true;
}

bool TaskModule::AbandonTask(uint64_t role_id, int32_t task_config_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = task_cache_.find(role_id);
    if (it == task_cache_.end()) {
        return false;
    }
    
    // 查找并移除任务
    auto task_it = std::remove_if(it->second.begin(), it->second.end(),
        [task_config_id](const TaskInfo& task) {
            return task.task_config_id == task_config_id && task.status == TaskStatus::ACCEPTED;
        });
    
    if (task_it == it->second.end()) {
        LOG_ERROR("Task not found or not in accepted status: role_id=%llu, task_config_id=%d", role_id, task_config_id);
        return false;
    }
    
    it->second.erase(task_it, it->second.end());
    
    LOG_INFO("Task abandoned: role_id=%llu, task_config_id=%d", role_id, task_config_id);
    return true;
}

bool TaskModule::UpdateTaskProgress(uint64_t role_id, int32_t task_config_id, int32_t condition_index, int32_t progress) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = task_cache_.find(role_id);
    if (it == task_cache_.end()) {
        return false;
    }
    
    for (auto& task : it->second) {
        if (task.task_config_id == task_config_id && task.status == TaskStatus::ACCEPTED) {
            if (condition_index >= 0 && condition_index < task.conditions.size()) {
                task.conditions[condition_index].current_count = progress;
                
                // 检查任务是否完成
                bool all_completed = true;
                for (const auto& condition : task.conditions) {
                    if (condition.current_count < condition.target_count) {
                        all_completed = false;
                        break;
                    }
                }
                
                if (all_completed) {
                    task.status = TaskStatus::COMPLETED;
                    task.complete_time = time(nullptr);
                    LOG_INFO("Task auto completed: role_id=%llu, task_config_id=%d", role_id, task_config_id);
                }
                
                return true;
            }
        }
    }
    
    return false;
}

bool TaskModule::AddTaskProgress(uint64_t role_id, TaskConditionType condition_type, int32_t target_id, int32_t count) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = task_cache_.find(role_id);
    if (it == task_cache_.end()) {
        return false;
    }
    
    bool updated = false;
    
    for (auto& task : it->second) {
        if (task.status != TaskStatus::ACCEPTED) {
            continue;
        }
        
        for (auto& condition : task.conditions) {
            if (condition.condition_type == static_cast<int32_t>(condition_type) && 
                condition.target_id == target_id) {
                // 更新进度
                condition.current_count += count;
                if (condition.current_count > condition.target_count) {
                    condition.current_count = condition.target_count;
                }
                
                updated = true;
                LOG_INFO("Task progress updated: role_id=%llu, task_config_id=%d, progress=%d/%d", 
                         role_id, task.task_config_id, condition.current_count, condition.target_count);
            }
        }
        
        // 检查任务是否完成
        bool all_completed = true;
        for (const auto& condition : task.conditions) {
            if (condition.current_count < condition.target_count) {
                all_completed = false;
                break;
            }
        }
        
        if (all_completed) {
            task.status = TaskStatus::COMPLETED;
            task.complete_time = time(nullptr);
            LOG_INFO("Task completed by progress: role_id=%llu, task_config_id=%d", role_id, task.task_config_id);
        }
    }
    
    return updated;
}

bool TaskModule::RefreshDailyTasks(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = daily_task_cache_.find(role_id);
    if (it == daily_task_cache_.end()) {
        daily_task_cache_[role_id] = std::vector<DailyTaskInfo>();
        it = daily_task_cache_.find(role_id);
    }
    
    // 清空旧的任务
    it->second.clear();
    
    // TODO: 从配置中随机生成日常任务
    // 这里简单示例，实际应该从配置表中选择
    
    time_t now = time(nullptr);
    time_t refresh_time = now + 86400; // 24小时后刷新
    
    // 添加一些示例日常任务
    for (int32_t i = 1; i <= 5; ++i) {
        DailyTaskInfo daily_task;
        daily_task.task_config_id = 1000 + i;
        daily_task.status = TaskStatus::NOT_ACCEPTED;
        daily_task.progress = 0;
        daily_task.refresh_time = refresh_time;
        it->second.push_back(daily_task);
    }
    
    LOG_INFO("Daily tasks refreshed: role_id=%llu, count=%d", role_id, it->second.size());
    return true;
}

bool TaskModule::GetDailyTasks(uint64_t role_id, std::vector<DailyTaskInfo>& tasks) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = daily_task_cache_.find(role_id);
    if (it == daily_task_cache_.end()) {
        return false;
    }
    
    tasks = it->second;
    return true;
}

bool TaskModule::AcceptDailyTask(uint64_t role_id, int32_t task_config_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = daily_task_cache_.find(role_id);
    if (it == daily_task_cache_.end()) {
        return false;
    }
    
    for (auto& daily_task : it->second) {
        if (daily_task.task_config_id == task_config_id && daily_task.status == TaskStatus::NOT_ACCEPTED) {
            daily_task.status = TaskStatus::ACCEPTED;
            
            // 同时添加到主任务列表
            AcceptTask(role_id, task_config_id);
            
            LOG_INFO("Daily task accepted: role_id=%llu, task_config_id=%d", role_id, task_config_id);
            return true;
        }
    }
    
    return false;
}

bool TaskModule::TrackTask(uint64_t role_id, int32_t task_config_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 检查任务是否存在
    auto it = task_cache_.find(role_id);
    if (it != task_cache_.end()) {
        for (const auto& task : it->second) {
            if (task.task_config_id == task_config_id) {
                tracked_tasks_[role_id] = task_config_id;
                LOG_INFO("Task tracked: role_id=%llu, task_config_id=%d", role_id, task_config_id);
                return true;
            }
        }
    }
    
    return false;
}

bool TaskModule::GetTrackedTask(uint64_t role_id, int32_t& task_config_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = tracked_tasks_.find(role_id);
    if (it == tracked_tasks_.end()) {
        return false;
    }
    
    task_config_id = it->second;
    return true;
}

void TaskModule::OnKillMonster(uint64_t role_id, int32_t monster_id) {
    AddTaskProgress(role_id, TaskConditionType::KILL_MONSTER, monster_id, 1);
}

void TaskModule::OnCollectItem(uint64_t role_id, int32_t item_id, int32_t count) {
    AddTaskProgress(role_id, TaskConditionType::COLLECT_ITEM, item_id, count);
}

void TaskModule::OnReachLevel(uint64_t role_id, int32_t level) {
    AddTaskProgress(role_id, TaskConditionType::REACH_LEVEL, level, 1);
}

void TaskModule::OnCompleteInstance(uint64_t role_id, int32_t instance_id) {
    AddTaskProgress(role_id, TaskConditionType::COMPLETE_INSTANCE, instance_id, 1);
}

void TaskModule::OnEquipItem(uint64_t role_id, int32_t item_id) {
    AddTaskProgress(role_id, TaskConditionType::EQUIP_ITEM, item_id, 1);
}

void TaskModule::OnSkillUpgrade(uint64_t role_id, int32_t skill_id, int32_t level) {
    AddTaskProgress(role_id, TaskConditionType::SKILL_LEVEL, skill_id, level);
}

bool TaskModule::LoadTaskData(uint64_t role_id) {
    // 从数据库加载任务数据
    // TODO: 实现从数据库加载任务数据
    
    // 初始化任务
    InitTasks(role_id);
    
    // 刷新日常任务
    RefreshDailyTasks(role_id);
    
    return true;
}

bool TaskModule::SaveTaskData(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = task_cache_.find(role_id);
    if (it == task_cache_.end()) {
        return false;
    }
    
    // 保存任务数据到数据库
    msg_task::TaskDataSyncReq req;
    req.set_role_id(role_id);
    
    for (const auto& task : it->second) {
        auto task_data = req.add_tasks();
        task_data->set_task_id(task.task_id);
        task_data->set_task_config_id(task.task_config_id);
        task_data->set_status(static_cast<int32_t>(task.status));
        task_data->set_accept_time(task.accept_time);
        task_data->set_complete_time(task.complete_time);
        task_data->set_submit_time(task.submit_time);
        
        // 保存条件进度
        for (const auto& condition : task.conditions) {
            auto condition_data = task_data->add_conditions();
            condition_data->set_condition_type(condition.condition_type);
            condition_data->set_target_id(condition.target_id);
            condition_data->set_target_count(condition.target_count);
            condition_data->set_current_count(condition.current_count);
        }
    }
    
    service_->SendMsgToDBServer(static_cast<uint32_t>(MessageID::MSG_TASK_DATA_SYNC_REQ), req);
    
    return true;
}

bool TaskModule::GetTaskConfig(int32_t task_config_id, TaskConfig& config) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = task_configs_.find(task_config_id);
    if (it == task_configs_.end()) {
        // TODO: 从配置表加载任务配置
        return false;
    }
    
    config = it->second;
    return true;
}

uint64_t TaskModule::GenerateTaskId() {
    static uint64_t next_id = time(nullptr) * 10000 + rand() % 10000;
    return next_id++;
}

bool TaskModule::CheckPreTask(uint64_t role_id, int32_t pre_task_id) {
    return IsTaskCompleted(role_id, pre_task_id);
}

bool TaskModule::CheckLevelRequirement(uint64_t role_id, int32_t level) {
    // TODO: 从角色模块获取角色等级
    return true;
}

} // namespace game_server
