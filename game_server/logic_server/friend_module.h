#ifndef __FRIEND_MODULE_H__
#define __FRIEND_MODULE_H__

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>

namespace game_server {

class LogicService;

// 好友状态
enum class FriendStatus {
    OFFLINE = 0,    // 离线
    ONLINE = 1,     // 在线
    IN_BATTLE = 2,  // 战斗中
    IN_TEAM = 3     // 组队中
};

// 好友关系类型
enum class FriendRelationType {
    NONE = 0,       // 无关系
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
    time_t last_login_time;
    time_t friend_since;
};

// 好友申请信息
struct FriendApplyInfo {
    uint64_t apply_id;
    uint64_t requester_id;
    uint64_t target_id;
    std::string message;
    FriendApplyStatus status;
    time_t send_time;
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
    bool SendFriendRequest(uint64_t role_id, uint64_t target_id, const std::string& message);
    bool RespondToFriendRequest(uint64_t role_id, uint64_t apply_id, bool accept);

    // 删除好友
    bool RemoveFriend(uint64_t role_id, uint64_t friend_id);

    // 好友申请
    bool GetFriendRequests(uint64_t role_id, std::vector<FriendApplyInfo>& requests);

    // 最近联系人
    bool GetRecentPlayers(uint64_t role_id, std::vector<FriendInfo>& players);
    bool AddRecentPlayer(uint64_t role_id, uint64_t player_id, const std::string& player_name, int32_t level, int32_t profession);
    bool DeleteRecentPlayer(uint64_t role_id, uint64_t player_id);

    // 好友状态
    bool UpdateFriendStatus(uint64_t role_id, uint64_t friend_id, FriendStatus status);

    // 数据持久化
    bool SaveFriendData(uint64_t role_id);
    bool LoadFriendData(uint64_t role_id);

   private:
    // 检查好友数量限制
    bool CheckFriendLimit(uint64_t role_id);

    // 生成申请ID
    uint64_t GenerateApplyId();

    // 服务指针
    LogicService* service_;

    // 好友缓存
    std::unordered_map<uint64_t, std::vector<FriendInfo>> friend_cache_;

    // 好友申请缓存
    std::unordered_map<uint64_t, std::vector<FriendApplyInfo>> apply_cache_;

    // 发送的好友申请缓存
    std::unordered_map<uint64_t, std::vector<FriendApplyInfo>> sent_apply_cache_;

    // 最近联系人缓存
    std::unordered_map<uint64_t, std::vector<FriendInfo>> recent_cache_;

    // 黑名单缓存
    std::unordered_map<uint64_t, std::unordered_set<uint64_t>> blacklist_cache_;

    // 互斥锁
    std::mutex cache_mutex_;

    // 常量
    static constexpr int32_t MAX_FRIEND_COUNT = 50;
    static constexpr int32_t MAX_RECENT_COUNT = 20;
};

} // namespace game_server

#endif // __FRIEND_MODULE_H__