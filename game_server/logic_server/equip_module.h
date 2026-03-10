#ifndef __EQUIP_MODULE_H__
#define __EQUIP_MODULE_H__

#include <unordered_map>

namespace game_server {

class LogicService;

// 装备位置
enum class EquipPosition {
    WEAPON = 1,      // 武器
    HELMET = 2,       // 头盔
    ARMOR = 3,        // 护甲
    NECKLACE = 4,     // 项链
    RING = 5,         // 戒指
    BOOTS = 6,         // 鞋子
    BELT = 7,         // 腰带
    GLOVES = 8,        // 手套
    CLOAK = 9,         // 披风
    MAX = 10
};

// 装备信息
struct EquipInfo {
    uint64_t equip_id;
    int32_t equip_config_id;
    int32_t position;
    int32_t level;
    int32_t star_level;
    int32_t quality;
    int32_t强化等级;
    std::vector<int32_t> gems;
    std::string extra_data;
};

// 装备属性加成
struct EquipAttribute {
    int32_t hp;
    int32_t mp;
    int32_t attack;
    int32_t defense;
    int32_t speed;
    int32_t critical;
    int32_t critical_damage;
    int32_t dodge;
    int32_t hit;
    int32_t armor;
    int32_t magic_resist;
};

// 装备模块类
class EquipModule {
public:
    EquipModule(LogicService* service);
    ~EquipModule();
    
    // 装备管理
    bool InitEquip(uint64_t role_id);
    bool GetEquipList(uint64_t role_id, std::vector<EquipInfo>& equips);
    bool GetEquipInfo(uint64_t role_id, uint64_t equip_id, EquipInfo& info);
    
    // 装备穿戴
    bool WearEquip(uint64_t role_id, uint64_t equip_id, int32_t position);
    bool TakeOffEquip(uint64_t role_id, int32_t position);
    bool GetWornEquips(uint64_t role_id, std::vector<EquipInfo>& equips);
    
    // 装备强化
    bool StrengthenEquip(uint64_t role_id, uint64_t equip_id, int32_t level);
    bool GetStrengthenLevel(uint64_t role_id, uint64_t equip_id, int32_t& level);
    
    // 装备升星
    bool UpgradeStar(uint64_t role_id, uint64_t equip_id, int32_t star_level);
    bool GetStarLevel(uint64_t role_id, uint64_t equip_id, int32_t& star_level);
    
    // 宝石镶嵌
    bool InlayGem(uint64_t role_id, uint64_t equip_id, int32_t gem_config_id, int32_t gem_index);
    bool RemoveGem(uint64_t role_id, uint64_t equip_id, int32_t gem_index);
    bool GetGems(uint64_t role_id, uint64_t equip_id, std::vector<int32_t>& gems);
    
    // 装备属性计算
    bool CalculateEquipAttribute(uint64_t role_id, EquipAttribute& attribute);
    bool CalculateSingleEquipAttribute(const EquipInfo& equip, EquipAttribute& attribute);
    
    // 装备数据加载和保存
    bool LoadEquipData(uint64_t role_id);
    bool SaveEquipData(uint64_t role_id);
    
private:
    // 检查装备位置是否可用
    bool IsPositionAvailable(uint64_t role_id, int32_t position);
    
    // 生成装备ID
    uint64_t GenerateEquipId();
    
    // 装备缓存
    std::unordered_map<uint64_t, std::vector<EquipInfo>> equip_cache_;
    std::unordered_map<uint64_t, std::unordered_map<int32_t, EquipInfo>> worn_equips_;
    
    // 逻辑服务指针
    LogicService* service_;
    
    // 互斥锁
    std::mutex cache_mutex_;
};

} // namespace game_server

#endif // __EQUIP_MODULE_H__
