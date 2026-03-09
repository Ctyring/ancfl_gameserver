#include "bag_module.h"
#include "proto/msg_bag.pb.h"

namespace game_server {

BagModule::BagModule(LogicService* service) : service_(service) {
}

BagModule::~BagModule() {
}

bool BagModule::InitBag(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 检查背包是否已初始化
    auto it = bag_cache_.find(role_id);
    if (it != bag_cache_.end()) {
        return true;
    }
    
    // 初始化背包
    std::vector<BagSlot> slots;
    slots.resize(BASE_BAG_SIZE);
    for (int32_t i = 0; i < BASE_BAG_SIZE; ++i) {
        slots[i].slot_index = i;
        slots[i].is_empty = true;
    }
    
    bag_cache_[role_id] = slots;
    
    LOG_INFO("Bag initialized: role_id=%llu, size=%d", role_id, BASE_BAG_SIZE);
    return true;
}

bool BagModule::GetBag(uint64_t role_id, std::vector<BagSlot>& slots) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = bag_cache_.find(role_id);
    if (it == bag_cache_.end()) {
        return false;
    }
    
    slots = it->second;
    return true;
}

bool BagModule::GetBagSlot(uint64_t role_id, int32_t slot_index, BagSlot& slot) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = bag_cache_.find(role_id);
    if (it == bag_cache_.end()) {
        return false;
    }
    
    if (slot_index < 0 || slot_index >= it->second.size()) {
        return false;
    }
    
    slot = it->second[slot_index];
    return true;
}

bool BagModule::AddItem(uint64_t role_id, int32_t item_config_id, int32_t count, int32_t quality) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = bag_cache_.find(role_id);
    if (it == bag_cache_.end()) {
        return false;
    }
    
    // 查找可堆叠的格子
    int32_t stackable_slot = FindStackableSlot(role_id, item_config_id);
    if (stackable_slot >= 0) {
        // 堆叠物品
        it->second[stackable_slot].item.count += count;
        LOG_INFO("Item stacked: role_id=%llu, slot=%d, item_id=%d, count=%d", role_id, stackable_slot, item_config_id, it->second[stackable_slot].item.count);
        return true;
    }
    
    // 查找空格子
    int32_t empty_slot = FindEmptySlot(role_id);
    if (empty_slot < 0) {
        LOG_ERROR("Bag is full: role_id=%llu", role_id);
        return false;
    }
    
    // 在空格子中添加物品
    it->second[empty_slot].is_empty = false;
    it->second[empty_slot].item.item_id = GenerateItemId();
    it->second[empty_slot].item.item_config_id = item_config_id;
    it->second[empty_slot].item.count = count;
    it->second[empty_slot].item.quality = quality;
    it->second[empty_slot].item.level = 1;
    it->second[empty_slot].item.bind_type = 0;
    it->second[empty_slot].item.expire_time = 0;
    
    LOG_INFO("Item added: role_id=%llu, slot=%d, item_id=%d, count=%d", role_id, empty_slot, item_config_id, count);
    return true;
}

bool BagModule::RemoveItem(uint64_t role_id, uint64_t item_id, int32_t count) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = bag_cache_.find(role_id);
    if (it == bag_cache_.end()) {
        return false;
    }
    
    // 查找物品
    for (auto& slot : it->second) {
        if (!slot.is_empty && slot.item.item_id == item_id) {
            if (slot.item.count < count) {
                LOG_ERROR("Item count not enough: role_id=%llu, item_id=%llu, have=%d, need=%d", role_id, item_id, slot.item.count, count);
                return false;
            }
            
            slot.item.count -= count;
            if (slot.item.count == 0) {
                slot.is_empty = true;
                slot.item = ItemInfo();
            }
            
            LOG_INFO("Item removed: role_id=%llu, item_id=%llu, count=%d", role_id, item_id, count);
            return true;
        }
    }
    
    LOG_ERROR("Item not found: role_id=%llu, item_id=%llu", role_id, item_id);
    return false;
}

bool BagModule::UseItem(uint64_t role_id, uint64_t item_id, int32_t count) {
    // 检查物品是否存在
    int32_t current_count = 0;
    if (!GetItemCount(role_id, item_id, current_count)) {
        return false;
    }
    
    if (current_count < count) {
        LOG_ERROR("Item count not enough: role_id=%llu, item_id=%llu, have=%d, need=%d", role_id, item_id, current_count, count);
        return false;
    }
    
    // TODO: 实现物品使用逻辑
    // 根据物品类型执行不同的效果
    
    // 扣除物品
    RemoveItem(role_id, item_id, count);
    
    LOG_INFO("Item used: role_id=%llu, item_id=%llu, count=%d", role_id, item_id, count);
    return true;
}

bool BagModule::DropItem(uint64_t role_id, uint64_t item_id, int32_t count) {
    // 检查物品是否存在
    int32_t current_count = 0;
    if (!GetItemCount(role_id, item_id, current_count)) {
        return false;
    }
    
    if (current_count < count) {
        LOG_ERROR("Item count not enough: role_id=%llu, item_id=%llu, have=%d, need=%d", role_id, item_id, current_count, count);
        return false;
    }
    
    // TODO: 实现物品丢弃逻辑
    // 生成掉落物到场景中
    
    // 扣除物品
    RemoveItem(role_id, item_id, count);
    
    LOG_INFO("Item dropped: role_id=%llu, item_id=%llu, count=%d", role_id, item_id, count);
    return true;
}

bool BagModule::MoveItem(uint64_t role_id, int32_t from_slot, int32_t to_slot) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = bag_cache_.find(role_id);
    if (it == bag_cache_.end()) {
        return false;
    }
    
    if (from_slot < 0 || from_slot >= it->second.size() || to_slot < 0 || to_slot >= it->second.size()) {
        LOG_ERROR("Invalid slot index: role_id=%llu, from=%d, to=%d", role_id, from_slot, to_slot);
        return false;
    }
    
    if (from_slot == to_slot) {
        return true;
    }
    
    // 检查目标格子是否为空
    if (!it->second[to_slot].is_empty) {
        LOG_ERROR("Target slot not empty: role_id=%llu, slot=%d", role_id, to_slot);
        return false;
    }
    
    // 移动物品
    it->second[to_slot] = it->second[from_slot];
    it->second[to_slot].slot_index = to_slot;
    it->second[from_slot].is_empty = true;
    it->second[from_slot].slot_index = from_slot;
    it->second[from_slot].item = ItemInfo();
    
    LOG_INFO("Item moved: role_id=%llu, from=%d, to=%d", role_id, from_slot, to_slot);
    return true;
}

bool BagModule::SwapItem(uint64_t role_id, int32_t slot1, int32_t slot2) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = bag_cache_.find(role_id);
    if (it == bag_cache_.end()) {
        return false;
    }
    
    if (slot1 < 0 || slot1 >= it->second.size() || slot2 < 0 || slot2 >= it->second.size()) {
        LOG_ERROR("Invalid slot index: role_id=%llu, slot1=%d, slot2=%d", role_id, slot1, slot2);
        return false;
    }
    
    if (slot1 == slot2) {
        return true;
    }
    
    // 交换物品
    std::swap(it->second[slot1], it->second[slot2]);
    it->second[slot1].slot_index = slot1;
    it->second[slot2].slot_index = slot2;
    
    LOG_INFO("Item swapped: role_id=%llu, slot1=%d, slot2=%d", role_id, slot1, slot2);
    return true;
}

bool BagModule::GetItemCount(uint64_t role_id, int32_t item_config_id, int32_t& count) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = bag_cache_.find(role_id);
    if (it == bag_cache_.end()) {
        return false;
    }
    
    count = 0;
    for (const auto& slot : it->second) {
        if (!slot.is_empty && slot.item.item_config_id == item_config_id) {
            count += slot.item.count;
        }
    }
    
    return true;
}

bool BagModule::HasItem(uint64_t role_id, int32_t item_config_id, int32_t count) {
    int32_t current_count = 0;
    if (!GetItemCount(role_id, item_config_id, current_count)) {
        return false;
    }
    
    return current_count >= count;
}

bool BagModule::ExpandBag(uint64_t role_id, int32_t expand_count) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = bag_cache_.find(role_id);
    if (it == bag_cache_.end()) {
        return false;
    }
    
    int32_t current_size = it->second.size();
    int32_t new_size = current_size + expand_count;
    
    if (new_size > MAX_BAG_SIZE) {
        LOG_ERROR("Bag size exceeds max: role_id=%llu, current=%d, add=%d, max=%d", role_id, current_size, expand_count, MAX_BAG_SIZE);
        return false;
    }
    
    // 扩展背包
    for (int32_t i = current_size; i < new_size; ++i) {
        BagSlot slot;
        slot.slot_index = i;
        slot.is_empty = true;
        it->second.push_back(slot);
    }
    
    LOG_INFO("Bag expanded: role_id=%llu, old_size=%d, new_size=%d", role_id, current_size, new_size);
    return true;
}

bool BagModule::LoadBagData(uint64_t role_id) {
    // 从数据库加载背包数据
    // TODO: 实现从数据库加载背包数据
    
    // 初始化背包
    InitBag(role_id);
    
    return true;
}

bool BagModule::SaveBagData(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = bag_cache_.find(role_id);
    if (it == bag_cache_.end()) {
        return false;
    }
    
    // 保存背包数据到数据库
    msg_bag::BagDataSyncReq req;
    req.set_role_id(role_id);
    
    for (const auto& slot : it->second) {
        if (!slot.is_empty) {
            auto item = req.add_items();
            item->set_item_id(slot.item.item_id);
            item->set_item_config_id(slot.item.item_config_id);
            item->set_count(slot.item.count);
            item->set_quality(slot.item.quality);
            item->set_level(slot.item.level);
            item->set_bind_type(slot.item.bind_type);
            item->set_expire_time(slot.item.expire_time);
            item->set_slot_index(slot.slot_index);
        }
    }
    
    service_->SendMsgToDBServer(static_cast<uint32_t>(MessageID::MSG_BAG_DATA_SYNC_REQ), req);
    
    return true;
}

int32_t BagModule::FindEmptySlot(uint64_t role_id) {
    auto it = bag_cache_.find(role_id);
    if (it == bag_cache_.end()) {
        return -1;
    }
    
    for (const auto& slot : it->second) {
        if (slot.is_empty) {
            return slot.slot_index;
        }
    }
    
    return -1;
}

int32_t BagModule::FindStackableSlot(uint64_t role_id, int32_t item_config_id) {
    auto it = bag_cache_.find(role_id);
    if (it == bag_cache_.end()) {
        return -1;
    }
    
    for (const auto& slot : it->second) {
        if (!slot.is_empty && slot.item.item_config_id == item_config_id) {
            // TODO: 检查物品是否可堆叠
            return slot.slot_index;
        }
    }
    
    return -1;
}

uint64_t BagModule::GenerateItemId() {
    // 生成物品ID
    static uint64_t next_id = time(nullptr) * 10000 + rand() % 10000;
    return next_id++;
}

} // namespace game_server
