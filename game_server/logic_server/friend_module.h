#ifndef __FRIEND_MODULE_H__
#define __FRIEND_MODULE_H__

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "logic_service.h"

namespace game_server {

// 好友状态
enum class FriendStatus {
    OFFLINE = 0,    // 离线
    ONLINE = 1,     // 在线
    IN_BATTLE = 2,  // 战斗中
    IN_TEAM = 3     // 组队中
};

// 好友关系类型
enum class FriendRelationType {
    FRIEND = 1,     // 好友
    BLACKLIST = 2,  // 黑名单
    RECENT = 3      // 最近联系人
};

// 好友申请状态
enum class FriendApplyStatus {
    PENDING = 0,   // 待处理
    ACCEPTED = 1,  // 已接受
    REJECTED = 2   // 已拒绝
};

// 好友信息
struct FriendInfo {
    uint64_t friend_id;
    std::string friend_name;
    int32_t level;
    int32_t profession;
    FriendStatus status;
    FriendRelationType relation_type;
    time_t add_time;
    int32_t intimacy;
    std::string remark;
    bool is_online;
};

// 好友申请信息
struct FriendApplyInfo {
    uint64_t apply_id;
    uint64_t applicant_id;
    std::string applicant_name;
    int32_t applicant_level;
    int32_t applicant_profession;
    FriendApplyStatus status;
    time_t apply_time;
    std::string message;
};

// 好友模块类
class FriendModule {
   public:
    FriendModule(LogicService* service);
    ~FriendModule();

    // 好友管理
    bool InitFriends(uint64_t role_id);
    bool GetFriends(uint64_t role_id, std::vector<FriendInfo>& friends);
    bool GetFriendInfo(uint64_t role_id, uint64_t friend_id, FriendInfo& info);

    // 添加好友
    bool AddFriend(uint64_t role_id, uint64_t friend_id);
    bool ApplyAddFriend(uint64_t role_id,
                        uint64_t target_id,
                        const std::string& message);
    bool HandleFriendApply(uint64_t role_id, uint64_t apply_id, bool accept);

    // 删除好友
    bool RemoveFriend(uint64_t role_id, uint64_t friend_id);
    bool DeleteFriend(uint64_t role_id, uint64_t friend_id);

    // 好友申请
    bool GetFriendApplies(uint64_t role_id,
                          std::vector<FriendApplyInfo>& applies);
    bool GetSentFriendApplies(uint64_t role_id,
                              std::vector<FriendApplyInfo>& applies);

    // 黑名单
    bool AddToBlacklist(uint64_t role_id, uint64_t target_id);
    bool RemoveFromBlacklist(uint64_t role_id, uint64_t target_id);
    bool GetBlacklist(uint64_t role_id, std::vector<FriendInfo>& blacklist);
    bool IsInBlacklist(uint64_t role_id, uint64_t target_id);

    // 最近联系人
    bool AddRecentContact(uint64_t role_id, uint64_t contact_id);
    bool GetRecentContacts(uint64_t role_id, std::vector<FriendInfo>& contacts);
    bool ClearRecentContacts(uint64_t role_id);

    // 好友状态
    bool UpdateFriendStatus(uint64_t role_id, FriendStatus status);
    bool NotifyFriendStatusChange(uint64_t role_id, FriendStatus status);
    bool IsFriend(uint64_t role_id, uint64_t target_id);

    // 亲密度
    bool AddIntimacy(uint64_t role_id, uint64_t friend_id, int32_t value);
    bool GetIntimacy(uint64_t role_id, uint64_t friend_id, int32_t& intimacy);

    // 备注
    bool SetRemark(uint64_t role_id,
                   uint64_t friend_id,
                   const std::string& remark);
    bool GetRemark(uint64_t role_id, uint64_t friend_id, std::string& remark);

    // 推荐好友
    bool GetRecommendedFriends(uint64_t role_id,
                               std::vector<FriendInfo>& friends);
    bool SearchFriend(const std::string& name,
                      std::vector<FriendInfo>& friends);

    // 好友数据加载和保存
    bool LoadFriendData(uint64_t role_id);
    bool SaveFriendData(uint64_t role_id);

    // 定时清理
    void OnTimer();

   private:
    // 生成申请ID
    uint64_t GenerateApplyId();

    // 检查好友数量上限
    bool CheckFriendLimit(uint64_t role_id);

    // 好友缓存
    std::unordered_map<uint64_t, std::vector<FriendInfo>> friend_cache_;
    std::unordered_map<uint64_t, std::vector<FriendApplyInfo>> apply_cache_;
    std::unordered_map<uint64_t, std::vector<FriendApplyInfo>>
        sent_apply_cache_;
    std::unordered_map<uint64_t, std::vector<FriendInfo>> recent_cache_;

    // 逻辑服务指针
    LogicService* service_;

    // 互斥锁
    std::mutex cache_mutex_;

    // 好友数量上限
    static const int32_t MAX_FRIEND_COUNT = 100;
    static const int32_t MAX_BLACKLIST_COUNT = 50;
    static const int32_t MAX_RECENT_COUNT = 20;
    static const int32_t MAX_APPLY_COUNT = 50;
};

}  // namespace game_server

#endif  // __FRIEND_MODULE_H__
