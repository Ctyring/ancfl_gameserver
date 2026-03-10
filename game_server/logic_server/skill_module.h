#ifndef __SKILL_MODULE_H__
#define __SKILL_MODULE_H__

#include <unordered_map>
#include <vector>

namespace game_server {

class LogicService;

// 技能类型
enum class SkillType {
    ACTIVE = 1,      // 主动技能
    PASSIVE = 2,     // 被动技能
    BUFF = 3,        // Buff技能
    DEBUFF = 4,      // Debuff技能
    HEAL = 5,        // 治疗技能
    CONTROL = 6      // 控制技能
};

// 技能目标类型
enum class SkillTargetType {
    SELF = 1,        // 自身
    SINGLE = 2,      // 单体
    AOE = 3,         // 范围
    LINE = 4,        // 直线
    SECTOR = 5       // 扇形
};

// 技能信息
struct SkillInfo {
    uint64_t skill_id;
    int32_t skill_config_id;
    int32_t level;
    int32_t exp;
    int32_t cooldown_time;
    time_t last_use_time;
    bool is_unlocked;
};

// 技能效果
struct SkillEffect {
    int32_t effect_type;
    int32_t effect_value;
    int32_t duration;
    int32_t interval;
};

// 技能配置
struct SkillConfig {
    int32_t skill_config_id;
    std::string skill_name;
    SkillType type;
    SkillTargetType target_type;
    int32_t cast_range;
    int32_t aoe_radius;
    int32_t cooldown;
    int32_t mp_cost;
    int32_t damage;
    int32_t heal;
    std::vector<SkillEffect> effects;
};

// 技能模块类
class SkillModule {
public:
    SkillModule(LogicService* service);
    ~SkillModule();
    
    // 技能管理
    bool InitSkills(uint64_t role_id);
    bool GetSkills(uint64_t role_id, std::vector<SkillInfo>& skills);
    bool GetSkillInfo(uint64_t role_id, int32_t skill_config_id, SkillInfo& info);
    
    // 技能学习
    bool LearnSkill(uint64_t role_id, int32_t skill_config_id);
    bool UpgradeSkill(uint64_t role_id, int32_t skill_config_id);
    bool ForgetSkill(uint64_t role_id, int32_t skill_config_id);
    
    // 技能使用
    bool UseSkill(uint64_t role_id, int32_t skill_config_id, uint64_t target_id, float target_x, float target_z);
    bool CanUseSkill(uint64_t role_id, int32_t skill_config_id);
    bool IsSkillInCooldown(uint64_t role_id, int32_t skill_config_id);
    int32_t GetSkillCooldown(uint64_t role_id, int32_t skill_config_id);
    
    // 技能效果计算
    bool CalculateSkillDamage(uint64_t role_id, int32_t skill_config_id, uint64_t target_id, int32_t& damage);
    bool CalculateSkillHeal(uint64_t role_id, int32_t skill_config_id, uint64_t target_id, int32_t& heal);
    bool ApplySkillEffects(uint64_t role_id, int32_t skill_config_id, uint64_t target_id);
    
    // 技能目标选择
    bool GetSkillTargets(uint64_t role_id, int32_t skill_config_id, float target_x, float target_z, std::vector<uint64_t>& targets);
    bool IsTargetInRange(uint64_t role_id, uint64_t target_id, int32_t range);
    
    // 技能数据加载和保存
    bool LoadSkillData(uint64_t role_id);
    bool SaveSkillData(uint64_t role_id);
    
private:
    // 获取技能配置
    bool GetSkillConfig(int32_t skill_config_id, SkillConfig& config);
    
    // 生成技能ID
    uint64_t GenerateSkillId();
    
    // 技能缓存
    std::unordered_map<uint64_t, std::vector<SkillInfo>> skill_cache_;
    std::unordered_map<int32_t, SkillConfig> skill_configs_;
    
    // 逻辑服务指针
    LogicService* service_;
    
    // 互斥锁
    std::mutex cache_mutex_;
};

} // namespace game_server

#endif // __SKILL_MODULE_H__
