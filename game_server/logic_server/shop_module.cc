#include "shop_module.h"
#include "ancfl/log.h"
#include <mutex>

namespace game_server {

ShopModule::ShopModule(LogicService* service) : service_(service) {
    LoadShopConfigs();
}

ShopModule::~ShopModule() {}

bool ShopModule::InitShop(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    // 初始化购买记录
    buy_count_cache_[role_id] = std::unordered_map<int32_t, std::unordered_map<int32_t, int32_t>>();
    purchase_records_cache_[role_id] = std::vector<PurchaseRecord>();

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Shop initialized: role_id=" << role_id;
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
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Not enough money: role_id=" << role_id << ", price_type=" << item.price_type << ", price=" << total_price;
        return false;
    }

    // 添加物品到背包
    // TODO: 调用背包模块添加物品
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Item added to bag: role_id=" << role_id << ", item_config_id=" << item_config_id << ", count=" << count;

    // 增加购买数量
    AddItemBuyCount(role_id, shop_id, item_config_id, count);

    // 添加购买记录
    PurchaseRecord record;
    record.shop_id = shop_id;
    record.item_config_id = item_config_id;
    record.buy_count = count;
    record.buy_time = time(nullptr);

    if (purchase_records_cache_.find(role_id) == purchase_records_cache_.end()) {
        purchase_records_cache_[role_id] = std::vector<PurchaseRecord>();
    }
    purchase_records_cache_[role_id].push_back(record);

    // 保持购买记录不超过最大数量
    if (purchase_records_cache_[role_id].size() > MAX_PURCHASE_RECORDS) {
        purchase_records_cache_[role_id].erase(purchase_records_cache_[role_id].begin());
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Item bought: role_id=" << role_id << ", shop_id=" << shop_id << ", item_config_id=" << item_config_id << ", count=" << count;
    return true;
}

bool ShopModule::CanBuyItem(uint64_t role_id, int32_t shop_id, int32_t item_config_id, int32_t count) {
    // 检查商品是否存在
    ShopItem item;
    if (!GetShopItem(shop_id, item_config_id, item)) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Item not found: shop_id=" << shop_id << ", item_config_id=" << item_config_id;
        return false;
    }

    // 检查购买数量是否合法
    if (count <= 0) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Invalid count: " << count;
        return false;
    }

    // 检查购买限制
    if (item.buy_limit > 0) {
        int32_t current_count = GetItemBuyCount(role_id, shop_id, item_config_id);
        if (current_count + count > item.buy_limit) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Buy limit reached: role_id=" << role_id << ", shop_id=" << shop_id << ", item_config_id=" << item_config_id;
            return false;
        }
    }

    // 检查货币是否足够
    int32_t total_price = CalculatePrice(item.price, item.discount) * count;
    if (!CheckMoneyEnough(role_id, item.price_type, total_price)) {
        return false;
    }

    return true;
}

bool ShopModule::GetShopItem(int32_t shop_id, int32_t item_config_id, ShopItem& item) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto shop_it = shop_configs_.find(shop_id);
    if (shop_it == shop_configs_.end()) {
        return false;
    }

    for (const auto& shop_item : shop_it->second.items) {
        if (shop_item.config_id == item_config_id) {
            item = shop_item;
            return true;
        }
    }

    return false;
}

bool ShopModule::GetItemBuyCount(uint64_t role_id, int32_t shop_id, int32_t item_config_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto role_it = buy_count_cache_.find(role_id);
    if (role_it == buy_count_cache_.end()) {
        return 0;
    }

    auto shop_it = role_it->second.find(shop_id);
    if (shop_it == role_it->second.end()) {
        return 0;
    }

    auto item_it = shop_it->second.find(item_config_id);
    if (item_it == shop_it->second.end()) {
        return 0;
    }

    return item_it->second;
}

bool ShopModule::AddItemBuyCount(uint64_t role_id, int32_t shop_id, int32_t item_config_id, int32_t count) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    if (buy_count_cache_.find(role_id) == buy_count_cache_.end()) {
        buy_count_cache_[role_id] = std::unordered_map<int32_t, std::unordered_map<int32_t, int32_t>>();
    }

    if (buy_count_cache_[role_id].find(shop_id) == buy_count_cache_[role_id].end()) {
        buy_count_cache_[role_id][shop_id] = std::unordered_map<int32_t, int32_t>();
    }

    buy_count_cache_[role_id][shop_id][item_config_id] += count;
    return true;
}

bool ShopModule::CalculatePrice(int32_t base_price, int32_t discount) {
    if (discount <= 0 || discount >= 100) {
        return base_price;
    }

    return base_price * discount / 100;
}

bool ShopModule::CheckMoneyEnough(uint64_t role_id, int32_t price_type, int32_t amount) {
    // TODO: 实现货币检查逻辑
    // 暂时返回 true
    return true;
}

bool ShopModule::DeductMoney(uint64_t role_id, int32_t price_type, int32_t amount) {
    // TODO: 实现货币扣除逻辑
    // 暂时返回 true
    return true;
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

bool ShopModule::LoadShopConfigs() {
    // TODO: 由于 config_manager.h 不存在，暂时返回 true
    return true;
}

bool ShopModule::SaveShopData(uint64_t role_id) {
    // TODO: 由于 proto/msg_shop.pb.h 不存在，暂时返回 true
    return true;
}

bool ShopModule::LoadShopData(uint64_t role_id) {
    // 从数据库加载商店数据
    // TODO: 实现从数据库加载商店数据

    // 初始化商店
    InitShop(role_id);

    return true;
}

} // namespace game_server