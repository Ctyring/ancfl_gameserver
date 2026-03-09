#include "shop_module.h"
#include "proto/msg_shop.pb.h"

namespace game_server {

ShopModule::ShopModule(LogicService* service) : service_(service) {
    // 初始化商店配置
    // TODO: 从配置文件加载商店配置
}

ShopModule::~ShopModule() {
}

bool ShopModule::InitShop(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 初始化购买记录
    buy_count_cache_[role_id] = std::unordered_map<int32_t, std::unordered_map<int32_t, int32_t>>();
    purchase_records_cache_[role_id] = std::vector<PurchaseRecord>();
    
    LOG_INFO("Shop initialized: role_id=%llu", role_id);
    return true;
}

bool ShopModule::GetShopList(std::vector<ShopInfo>& shops) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    shops.clear();
    for (const auto& pair : shop_configs_) {
        shops.push_back(pair.second);
    }
    
    return true;
}

bool ShopModule::GetShopInfo(int32_t shop_id, ShopInfo& info) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = shop_configs_.find(shop_id);
    if (it == shop_configs_.end()) {
        return false;
    }
    
    info = it->second;
    return true;
}

bool ShopModule::GetShopItems(int32_t shop_id, std::vector<ShopItem>& items) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = shop_configs_.find(shop_id);
    if (it == shop_configs_.end()) {
        return false;
    }
    
    items = it->second.items;
    return true;
}

bool ShopModule::BuyItem(uint64_t role_id, int32_t shop_id, int32_t item_config_id, int32_t count) {
    // 检查是否可以购买
    if (!CanBuyItem(role_id, shop_id, item_config_id, count)) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 获取商品信息
    ShopItem item;
    if (!GetShopItem(shop_id, item_config_id, item)) {
        return false;
    }
    
    // 计算价格
    int32_t total_price = CalculatePrice(item.price, item.discount) * count;
    
    // 扣除货币
    if (!DeductMoney(role_id, item.price_type, total_price)) {
        LOG_ERROR("Not enough money: role_id=%llu, price_type=%d, price=%d", role_id, item.price_type, total_price);
        return false;
    }
    
    // 添加物品到背包
    // TODO: 调用背包模块添加物品
    LOG_INFO("Item added to bag: role_id=%llu, item_config_id=%d, count=%d", role_id, item_config_id, count);
    
    // 增加购买数量
    AddItemBuyCount(role_id, shop_id, item_config_id, count);
    
    // 添加购买记录
    PurchaseRecord record;
    record.shop_id = shop_id;
    record.item_config_id = item_config_id;
    record.buy_count = count;
    record.buy_time = time(nullptr);
    AddPurchaseRecord(role_id, record);
    
    LOG_INFO("Item bought: role_id=%llu, shop_id=%d, item_config_id=%d, count=%d, price=%d", 
             role_id, shop_id, item_config_id, count, total_price);
    return true;
}

bool ShopModule::CanBuyItem(uint64_t role_id, int32_t shop_id, int32_t item_config_id, int32_t count) {
    // 获取商品信息
    ShopItem item;
    if (!GetShopItem(shop_id, item_config_id, item)) {
        LOG_ERROR("Shop item not found: shop_id=%d, item_config_id=%d", shop_id, item_config_id);
        return false;
    }
    
    // 检查购买条件
    if (!CheckBuyConditions(role_id, item)) {
        return false;
    }
    
    // 检查购买数量限制
    int32_t bought_count = 0;
    GetItemBuyCount(role_id, shop_id, item_config_id, bought_count);
    if (bought_count + count > item.limit_count && item.limit_count > 0) {
        LOG_ERROR("Buy limit reached: role_id=%llu, bought=%d, want=%d, limit=%d", 
                  role_id, bought_count, count, item.limit_count);
        return false;
    }
    
    // 检查货币是否足够
    int32_t total_price = CalculatePrice(item.price, item.discount) * count;
    if (!CheckMoneyEnough(role_id, item.price_type, total_price)) {
        LOG_ERROR("Not enough money: role_id=%llu, price_type=%d, need=%d", role_id, item.price_type, total_price);
        return false;
    }
    
    return true;
}

bool ShopModule::CheckBuyConditions(uint64_t role_id, const ShopItem& item) {
    // TODO: 检查等级要求
    if (item.require_level > 0) {
        // 检查角色等级
    }
    
    // TODO: 检查VIP等级要求
    if (item.require_vip > 0) {
        // 检查VIP等级
    }
    
    return true;
}

bool ShopModule::GetItemBuyCount(uint64_t role_id, int32_t shop_id, int32_t item_config_id, int32_t& count) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = buy_count_cache_.find(role_id);
    if (it == buy_count_cache_.end()) {
        count = 0;
        return true;
    }
    
    auto shop_it = it->second.find(shop_id);
    if (shop_it == it->second.end()) {
        count = 0;
        return true;
    }
    
    auto item_it = shop_it->second.find(item_config_id);
    if (item_it == shop_it->second.end()) {
        count = 0;
        return true;
    }
    
    count = item_it->second;
    return true;
}

bool ShopModule::AddItemBuyCount(uint64_t role_id, int32_t shop_id, int32_t item_config_id, int32_t count) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = buy_count_cache_.find(role_id);
    if (it == buy_count_cache_.end()) {
        buy_count_cache_[role_id] = std::unordered_map<int32_t, std::unordered_map<int32_t, int32_t>>();
        it = buy_count_cache_.find(role_id);
    }
    
    auto shop_it = it->second.find(shop_id);
    if (shop_it == it->second.end()) {
        it->second[shop_id] = std::unordered_map<int32_t, int32_t>();
        shop_it = it->second.find(shop_id);
    }
    
    shop_it->second[item_config_id] += count;
    return true;
}

bool ShopModule::ResetBuyCount(uint64_t role_id, int32_t shop_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = buy_count_cache_.find(role_id);
    if (it == buy_count_cache_.end()) {
        return false;
    }
    
    it->second.erase(shop_id);
    LOG_INFO("Buy count reset: role_id=%llu, shop_id=%d", role_id, shop_id);
    return true;
}

bool ShopModule::ResetAllBuyCount(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = buy_count_cache_.find(role_id);
    if (it == buy_count_cache_.end()) {
        return false;
    }
    
    it->second.clear();
    LOG_INFO("All buy count reset: role_id=%llu", role_id);
    return true;
}

bool ShopModule::OpenMysteryShop(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 生成神秘商店商品
    MysteryShopInfo info;
    info.shop_id = 9999; // 神秘商店ID
    info.refresh_count = 0;
    info.expire_time = time(nullptr) + MYSTERY_SHOP_DURATION;
    
    // 生成随机商品
    for (int32_t i = 0; i < MYSTERY_SHOP_ITEM_COUNT; ++i) {
        MysteryShopItem item;
        item.slot_index = i;
        item.item_config_id = 1000 + rand() % 100; // 随机物品
        item.item_count = 1;
        item.price_type = 1; // 金币
        item.price = 100 + rand() % 900;
        item.discount = 50 + rand() % 50; // 5-10折
        item.is_bought = false;
        info.items.push_back(item);
    }
    
    mystery_shop_cache_[role_id] = info;
    
    LOG_INFO("Mystery shop opened: role_id=%llu, expire_time=%lld", role_id, info.expire_time);
    return true;
}

bool ShopModule::GetMysteryShop(uint64_t role_id, MysteryShopInfo& info) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = mystery_shop_cache_.find(role_id);
    if (it == mystery_shop_cache_.end()) {
        return false;
    }
    
    // 检查是否过期
    if (time(nullptr) > it->second.expire_time) {
        mystery_shop_cache_.erase(it);
        return false;
    }
    
    info = it->second;
    return true;
}

bool ShopModule::RefreshMysteryShop(uint64_t role_id) {
    // 重新生成神秘商店
    return OpenMysteryShop(role_id);
}

bool ShopModule::BuyMysteryItem(uint64_t role_id, int32_t slot_index) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = mystery_shop_cache_.find(role_id);
    if (it == mystery_shop_cache_.end()) {
        LOG_ERROR("Mystery shop not open: role_id=%llu", role_id);
        return false;
    }
    
    // 检查是否过期
    if (time(nullptr) > it->second.expire_time) {
        mystery_shop_cache_.erase(it);
        LOG_ERROR("Mystery shop expired: role_id=%llu", role_id);
        return false;
    }
    
    // 查找商品
    for (auto& item : it->second.items) {
        if (item.slot_index == slot_index) {
            if (item.is_bought) {
                LOG_ERROR("Mystery item already bought: role_id=%llu, slot_index=%d", role_id, slot_index);
                return false;
            }
            
            // 计算价格
            int32_t price = CalculatePrice(item.price, item.discount);
            
            // 扣除货币
            if (!DeductMoney(role_id, item.price_type, price)) {
                LOG_ERROR("Not enough money for mystery item: role_id=%llu, price=%d", role_id, price);
                return false;
            }
            
            // 添加物品到背包
            // TODO: 调用背包模块添加物品
            LOG_INFO("Mystery item added to bag: role_id=%llu, item_config_id=%d", role_id, item.item_config_id);
            
            // 标记为已购买
            item.is_bought = true;
            
            LOG_INFO("Mystery item bought: role_id=%llu, slot_index=%d, item_config_id=%d, price=%d", 
                     role_id, slot_index, item.item_config_id, price);
            return true;
        }
    }
    
    LOG_ERROR("Mystery item not found: role_id=%llu, slot_index=%d", role_id, slot_index);
    return false;
}

bool ShopModule::IsMysteryShopOpen(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = mystery_shop_cache_.find(role_id);
    if (it == mystery_shop_cache_.end()) {
        return false;
    }
    
    // 检查是否过期
    if (time(nullptr) > it->second.expire_time) {
        mystery_shop_cache_.erase(it);
        return false;
    }
    
    return true;
}

bool ShopModule::RefreshShop(int32_t shop_id) {
    // TODO: 刷新商店商品
    LOG_INFO("Shop refreshed: shop_id=%d", shop_id);
    return true;
}

bool ShopModule::ManualRefreshShop(uint64_t role_id, int32_t shop_id) {
    // TODO: 手动刷新商店，扣除刷新费用
    return RefreshShop(shop_id);
}

int32_t ShopModule::GetRefreshCost(int32_t shop_id, int32_t refresh_count) {
    // TODO: 根据刷新次数计算刷新费用
    return 100 * (refresh_count + 1);
}

bool ShopModule::GetPurchaseRecords(uint64_t role_id, std::vector<PurchaseRecord>& records) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = purchase_records_cache_.find(role_id);
    if (it == purchase_records_cache_.end()) {
        return false;
    }
    
    records = it->second;
    return true;
}

bool ShopModule::AddPurchaseRecord(uint64_t role_id, const PurchaseRecord& record) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = purchase_records_cache_.find(role_id);
    if (it == purchase_records_cache_.end()) {
        purchase_records_cache_[role_id] = std::vector<PurchaseRecord>();
        it = purchase_records_cache_.find(role_id);
    }
    
    it->second.push_back(record);
    
    // 限制记录数量
    if (it->second.size() > 100) {
        it->second.erase(it->second.begin());
    }
    
    return true;
}

int32_t ShopModule::CalculatePrice(int32_t base_price, int32_t discount) {
    return base_price * discount / 100;
}

bool ShopModule::CheckMoneyEnough(uint64_t role_id, int32_t price_type, int32_t price) {
    // TODO: 检查货币是否足够
    return true;
}

bool ShopModule::DeductMoney(uint64_t role_id, int32_t price_type, int32_t price) {
    // TODO: 扣除货币
    return true;
}

bool ShopModule::LoadShopData(uint64_t role_id) {
    // TODO: 从数据库加载商店数据
    InitShop(role_id);
    return true;
}

bool ShopModule::SaveShopData(uint64_t role_id) {
    // TODO: 保存商店数据到数据库
    return true;
}

void ShopModule::OnTimer() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    time_t now = time(nullptr);
    
    // 清理过期的神秘商店
    auto it = mystery_shop_cache_.begin();
    while (it != mystery_shop_cache_.end()) {
        if (now > it->second.expire_time) {
            LOG_INFO("Mystery shop expired and removed: role_id=%llu", it->first);
            it = mystery_shop_cache_.erase(it);
        } else {
            ++it;
        }
    }
}

bool ShopModule::GetShopItem(int32_t shop_id, int32_t item_config_id, ShopItem& item) {
    auto it = shop_configs_.find(shop_id);
    if (it == shop_configs_.end()) {
        return false;
    }
    
    for (const auto& shop_item : it->second.items) {
        if (shop_item.item_config_id == item_config_id) {
            item = shop_item;
            return true;
        }
    }
    
    return false;
}

} // namespace game_server
