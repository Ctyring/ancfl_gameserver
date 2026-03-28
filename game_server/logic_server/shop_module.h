#ifndef __SHOP_MODULE_H__
#define __SHOP_MODULE_H__

#include <unordered_map>
#include <vector>
#include <mutex>

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
    int32_t config_id;
    int32_t price_type;
    int32_t price;
    int32_t discount;
    int32_t buy_limit;
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
    
    // 购买限制
    bool GetItemBuyCount(uint64_t role_id, int32_t shop_id, int32_t item_config_id);
    bool AddItemBuyCount(uint64_t role_id, int32_t shop_id, int32_t item_config_id, int32_t count);
    
    // 购买记录
    bool GetPurchaseRecords(uint64_t role_id, std::vector<PurchaseRecord>& records);
    
    // 数据持久化
    bool SaveShopData(uint64_t role_id);
    bool LoadShopData(uint64_t role_id);

private:
    // 获取商店商品
    bool GetShopItem(int32_t shop_id, int32_t item_config_id, ShopItem& item);
    
    // 计算价格
    bool CalculatePrice(int32_t base_price, int32_t discount);
    
    // 检查货币是否足够
    bool CheckMoneyEnough(uint64_t role_id, int32_t price_type, int32_t amount);
    
    // 扣除货币
    bool DeductMoney(uint64_t role_id, int32_t price_type, int32_t amount);
    
    // 加载商店配置
    bool LoadShopConfigs();
    
    // 服务指针
    LogicService* service_;
    
    // 商店配置
    std::unordered_map<int32_t, ShopInfo> shop_configs_;
    
    // 购买数量缓存
    std::unordered_map<uint64_t, std::unordered_map<int32_t, std::unordered_map<int32_t, int32_t>>> buy_count_cache_;
    
    // 购买记录缓存
    std::unordered_map<uint64_t, std::vector<PurchaseRecord>> purchase_records_cache_;
    
    // 互斥锁
    std::mutex cache_mutex_;
    
    // 常量
    static constexpr int32_t MAX_PURCHASE_RECORDS = 100;
};

} // namespace game_server

#endif // __SHOP_MODULE_H__