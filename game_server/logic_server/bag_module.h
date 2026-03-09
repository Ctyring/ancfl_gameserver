#ifndef __BAG_MODULE_H__
#define __BAG_MODULE_H__

#include "logic_service.h"
#include <unordered_map>

namespace game_server {

// 物品信息
struct ItemInfo {
    uint64_t item_id;
    int32_t item_config_id;
    int32_t count;
    int32_t quality;
    int32_t level;
    int32_t bind_type;
    int64_t expire_time;
    std::string extra_data;
};

// 背包格子
struct BagSlot {
    int32_t slot_index;
    ItemInfo item;
    bool is_empty;
};

// 背包模块类
class BagModule {
public:
    BagModule(LogicService* service);
    ~BagModule();
    
    // 背包管理
    bool InitBag(uint64_t role_id);
    bool GetBag(uint64_t role_id, std::vector<BagSlot>& slots);
    bool GetBagSlot(uint64_t role_id, int32_t slot_index, BagSlot& slot);
    
    // 物品操作
    bool AddItem(uint64_t role_id, int32_t item_config_id, int32_t count, int32_t quality = 1);
    bool RemoveItem(uint64_t role_id, uint64_t item_id, int32_t count);
    bool UseItem(uint64_t role_id, uint64_t item_id, int32_t count);
    bool DropItem(uint64_t role_id, uint64_t item_id, int32_t count);
    
    // 物品移动
    bool MoveItem(uint64_t role_id, int32_t from_slot, int32_t to_slot);
    bool SwapItem(uint64_t role_id, int32_t slot1, int32_t slot2);
    
    // 物品查询
    bool GetItemCount(uint64_t role_id, int32_t item_config_id, int32_t& count);
    bool HasItem(uint64_t role_id, int32_t item_config_id, int32_t count);
    
    // 背包扩容
    bool ExpandBag(uint64_t role_id, int32_t expand_count);
    
    // 背包数据加载和保存
    bool LoadBagData(uint64_t role_id);
    bool SaveBagData(uint64_t role_id);
    
private:
    // 查找空格子
    int32_t FindEmptySlot(uint64_t role_id);
    
    // 查找可堆叠的格子
    int32_t FindStackableSlot(uint64_t role_id, int32_t item_config_id);
    
    // 生成物品ID
    uint64_t GenerateItemId();
    
    // 背包缓存
    std::unordered_map<uint64_t, std::vector<BagSlot>> bag_cache_;
    
    // 逻辑服务指针
    LogicService* service_;
    
    // 互斥锁
    std::mutex cache_mutex_;
    
    // 背包基础格子数
    static const int32_t BASE_BAG_SIZE = 40;
    
    // 最大背包格子数
    static const int32_t MAX_BAG_SIZE = 200;
};

} // namespace game_server

#endif // __BAG_MODULE_H__
