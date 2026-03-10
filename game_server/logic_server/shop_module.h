#ifndef __SHOP_MODULE_H__
#define __SHOP_MODULE_H__

#include <unordered_map>
#include <vector>

namespace game_server {

class LogicService;

// 商店类型
enum class ShopType {
    NORMAL = 1,     // 普通商店
    MYSTERY = 2,    // 神秘商店
    GUILD = 3,      // 公会商店
    PVP = 4,        // PVP商店
    ACTIVITY = 5    // 活动商店
};

// 商品信息
struct ShopItem {
    int32_t item_config_id;
    int32_t item_count;
    int32_t price_type;
    int32_t price;
    int32_t discount;
    int32_t limit_count;
    int32_t sold_count;
    int32_t require_level;
    int32_t require_vip;
    bool is_sold_out;
};

// 商店信息
struct ShopInfo {
    int32_t shop_id;
    ShopType type;
    std::string shop_name;
    std::vector<ShopItem> items;
    time_t refresh_time;
    int32_t refresh_cost;
    bool is_open;
};

// 购买记录
struct PurchaseRecord {
    int32_t shop_id;
    int32_t item_config_id;
    int32_t buy_count;
    time_t buy_time;
};

// 神秘商店商品
struct MysteryShopItem {
    int32_t slot_index;
    int32_t item_config_id;
    int32_t item_count;
    int32_t price_type;
    int32_t price;
    int32_t discount;
    bool is_bought;
};

// 神秘商店信息
struct MysteryShopInfo {
    int32_t shop_id;
    std::vector<MysteryShopItem> items;
    time_t expire_time;
    int32_t refresh_count;
};

// 商店模块类
class ShopModule {
public:
    ShopModule(LogicService* service);
    ~ShopModule();
    
    // 商店管理
    bool InitShop(uint64_t role_id);
    bool GetShopList(std::vector<ShopInfo>& shops);
    bool GetShopInfo(int32_t shop_id, ShopInfo& info);
    bool GetShopItems(int32_t shop_id, std::vector<ShopItem>& items);
    
    // 购买商品
    bool BuyItem(uint64_t role_id, int32_t shop_id, int32_t item_config_id, int32_t count);
    bool CanBuyItem(uint64_t role_id, int32_t shop_id, int32_t item_config_id, int32_t count);
    bool CheckBuyConditions(uint64_t role_id, const ShopItem& item);
    
    // 购买限制
    bool GetItemBuyCount(uint64_t role_id, int32_t shop_id, int32_t item_config_id, int32_t& count);
    bool AddItemBuyCount(uint64_t role_id, int32_t shop_id, int32_t item_config_id, int32_t count);
    bool ResetBuyCount(uint64_t role_id, int32_t shop_id);
    bool ResetAllBuyCount(uint64_t role_id);
    
    // 神秘商店
    bool OpenMysteryShop(uint64_t role_id);
    bool GetMysteryShop(uint64_t role_id, MysteryShopInfo& info);
    bool RefreshMysteryShop(uint64_t role_id);
    bool BuyMysteryItem(uint64_t role_id, int32_t slot_index);
    bool IsMysteryShopOpen(uint64_t role_id);
    
    // 商店刷新
    bool RefreshShop(int32_t shop_id);
    bool ManualRefreshShop(uint64_t role_id, int32_t shop_id);
    int32_t GetRefreshCost(int32_t shop_id, int32_t refresh_count);
    
    // 购买记录
    bool GetPurchaseRecords(uint64_t role_id, std::vector<PurchaseRecord>& records);
    bool AddPurchaseRecord(uint64_t role_id, const PurchaseRecord& record);
    
    // 价格计算
    int32_t CalculatePrice(int32_t base_price, int32_t discount);
    bool CheckMoneyEnough(uint64_t role_id, int32_t price_type, int32_t price);
    bool DeductMoney(uint64_t role_id, int32_t price_type, int32_t price);
    
    // 商店数据加载和保存
    bool LoadShopData(uint64_t role_id);
    bool SaveShopData(uint64_t role_id);
    
    // 定时刷新
    void OnTimer();
    
private:
    // 获取商品信息
    bool GetShopItem(int32_t shop_id, int32_t item_config_id, ShopItem& item);
    
    // 生成神秘商店商品
    bool GenerateMysteryShopItems(uint64_t role_id);
    
    // 商店配置缓存
    std::unordered_map<int32_t, ShopInfo> shop_configs_;
    
    // 玩家购买记录缓存
    std::unordered_map<uint64_t, std::unordered_map<int32_t, std::unordered_map<int32_t, int32_t>>> buy_count_cache_;
    std::unordered_map<uint64_t, MysteryShopInfo> mystery_shop_cache_;
    std::unordered_map<uint64_t, std::vector<PurchaseRecord>> purchase_records_cache_;
    
    // 逻辑服务指针
    LogicService* service_;
    
    // 互斥锁
    std::mutex cache_mutex_;
    
    // 神秘商店持续时间（秒）
    static const int32_t MYSTERY_SHOP_DURATION = 7200; // 2小时
    
    // 神秘商店商品数量
    static const int32_t MYSTERY_SHOP_ITEM_COUNT = 6;
};

} // namespace game_server

#endif // __SHOP_MODULE_H__
