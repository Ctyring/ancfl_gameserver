#ifndef __BUFF_MODULE_H__
#define __BUFF_MODULE_H__

#include "logic_service.h"
#include <unordered_map>
#include <vector>

namespace game_server {

// Buff类型
enum class BuffType {
    BUFF = 1,        // 增益效果
    DEBUFF = 2,      // 减益效果
    DOT = 3,         // 持续伤害
    HOT = 4,         // 持续治疗
    CONTROL = 5,     // 控制效果
    SHIELD = 6       // 护盾效果
};

// Buff效果类型
enum class BuffEffectType {
    NONE = 0,
    ATTACK_UP = 1,       // 攻击提升
    ATTACK_DOWN = 2,     // 攻击降低
    DEFENSE_UP = 3,      // 防御提升
    DEFENSE_DOWN = 4,    // 防御降低
    SPEED_UP = 5,        // 速度提升
    SPEED_DOWN = 6,      // 速度降低
    HP_REGEN = 7,        // 生命恢复
    MP_REGEN = 8,        // 法力恢复
    STUN = 9,            // 眩晕
    SILENCE = 10,        // 沉默
    POISON = 11,         // 中毒
    BURN = 12,           // 燃烧
    FREEZE = 13,         // 冰冻
    SHIELD = 14          // 护盾
};

// Buff信息
struct BuffInfo {
    uint64_t buff_id;
    int32_t buff_config_id;
    uint64_t caster_id;
    uint64_t target_id;
    BuffType type;
    BuffEffectType effect_type;
    int32_t effect_value;
    int32_t duration;
    int32_t interval;
    int32_t stack_count;
    int32_t max_stack;
    time_t start_time;
    time_t end_time;
    bool is_active;
};

// Buff配置
struct BuffConfig {
    int32_t buff_config_id;
    std::string buff_name;
    BuffType type;
    BuffEffectType effect_type;
    int32_t effect_value;
    int32_t duration;
    int32_t interval;
    int32_t max_stack;
    bool is_debuff;
    bool can_dispel;
};

// Buff模块类
class BuffModule {
public:
    BuffModule(LogicService* service);
    ~BuffModule();
    
    // Buff管理
    bool AddBuff(uint64_t target_id, uint64_t caster_id, int32_t buff_config_id);
    bool RemoveBuff(uint64_t target_id, uint64_t buff_id);
    bool RemoveBuffByConfigId(uint64_t target_id, int32_t buff_config_id);
    bool RemoveAllBuffs(uint64_t target_id);
    bool RemoveDebuffs(uint64_t target_id);
    
    // Buff查询
    bool GetBuffs(uint64_t target_id, std::vector<BuffInfo>& buffs);
    bool GetBuffInfo(uint64_t target_id, uint64_t buff_id, BuffInfo& info);
    bool HasBuff(uint64_t target_id, int32_t buff_config_id);
    bool HasDebuff(uint64_t target_id);
    
    // Buff效果计算
    bool CalculateBuffEffects(uint64_t target_id, int32_t& attack_bonus, int32_t& defense_bonus, int32_t& speed_bonus);
    bool ApplyBuffEffects(uint64_t target_id);
    bool ProcessDotHot(uint64_t target_id);
    
    // Buff堆叠
    bool StackBuff(uint64_t target_id, uint64_t buff_id);
    bool RefreshBuff(uint64_t target_id, uint64_t buff_id);
    
    // Buff更新
    void OnUpdate();
    void OnTimer();
    
    // Buff数据加载和保存
    bool LoadBuffData(uint64_t target_id);
    bool SaveBuffData(uint64_t target_id);
    
private:
    // 获取Buff配置
    bool GetBuffConfig(int32_t buff_config_id, BuffConfig& config);
    
    // 生成Buff ID
    uint64_t GenerateBuffId();
    
    // 应用单个Buff效果
    bool ApplySingleBuffEffect(uint64_t target_id, const BuffInfo& buff);
    bool RemoveSingleBuffEffect(uint64_t target_id, const BuffInfo& buff);
    
    // Buff缓存
    std::unordered_map<uint64_t, std::vector<BuffInfo>> buff_cache_;
    std::unordered_map<int32_t, BuffConfig> buff_configs_;
    
    // 逻辑服务指针
    LogicService* service_;
    
    // 互斥锁
    std::mutex cache_mutex_;
    
    // 更新间隔（毫秒）
    static const int32_t UPDATE_INTERVAL = 1000;
};

} // namespace game_server

#endif // __BUFF_MODULE_H__
