#include "friend_module.h"
#include "ancfl/log.h"
#include <mutex>

namespace game_server {

FriendModule::FriendModule(LogicService* service) : service_(service) {}

FriendModule::~FriendModule() {}

bool FriendModule::InitFriends(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = friend_cache_.find(role_id);
    if (it != friend_cache_.end()) {
        return true;
    }

    friend_cache_[role_id] = std::vector<FriendInfo>();
    apply_cache_[role_id] = std::vector<FriendApplyInfo>();
    sent_apply_cache_[role_id] = std::vector<FriendApplyInfo>();
    recent_cache_[role_id] = std::vector<FriendInfo>();

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Friends initialized: role_id=" << role_id;
    return true;
}

bool FriendModule::GetFriends(uint64_t role_id, std::vector<FriendInfo>& friends) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = friend_cache_.find(role_id);
    if (it == friend_cache_.end()) {
        return false;
    }

    friends.clear();
    for (const auto& friend_info : it->second) {
        if (friend_info.relation_type == FriendRelationType::FRIEND) {
            friends.push_back(friend_info);
        }
    }

    return true;
}

bool FriendModule::GetFriendInfo(uint64_t role_id, uint64_t friend_id, FriendInfo& info) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = friend_cache_.find(role_id);
    if (it == friend_cache_.end()) {
        return false;
    }

    for (const auto& friend_info : it->second) {
        if (friend_info.friend_id == friend_id) {
            info = friend_info;
            return true;
        }
    }

    return false;
}

bool FriendModule::AddFriend(uint64_t role_id, uint64_t friend_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    if (!CheckFriendLimit(role_id)) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Friend limit reached: role_id=" << role_id;
        return false;
    }

    auto it = friend_cache_.find(role_id);
    if (it == friend_cache_.end()) {
        friend_cache_[role_id] = std::vector<FriendInfo>();
        it = friend_cache_.find(role_id);
    }

    // 检查是否已经是好友
    for (const auto& friend_info : it->second) {
        if (friend_info.friend_id == friend_id && friend_info.relation_type == FriendRelationType::FRIEND) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Already friends: role_id=" << role_id << ", friend_id=" << friend_id;
            return false;
        }
    }

    // 创建好友信息
    FriendInfo info;
    info.friend_id = friend_id;
    info.friend_name = "";
    info.level = 1;
    info.profession = 1;
    info.status = FriendStatus::OFFLINE;
    info.relation_type = FriendRelationType::FRIEND;
    info.last_login_time = time(nullptr);
    info.friend_since = time(nullptr);

    it->second.push_back(info);

    // 同时更新对方的好友列表
    // TODO: 实现跨服务器好友同步

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Friend added: role_id=" << role_id << ", friend_id=" << friend_id;
    return true;
}

bool FriendModule::RemoveFriend(uint64_t role_id, uint64_t friend_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = friend_cache_.find(role_id);
    if (it == friend_cache_.end()) {
        return false;
    }

    auto friend_it = it->second.begin();
    while (friend_it != it->second.end()) {
        if (friend_it->friend_id == friend_id) {
            friend_it = it->second.erase(friend_it);
            ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Friend removed: role_id=" << role_id << ", friend_id=" << friend_id;
            return true;
        } else {
            ++friend_it;
        }
    }

    ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Friend not found: role_id=" << role_id << ", friend_id=" << friend_id;
    return false;
}

bool FriendModule::SendFriendRequest(uint64_t role_id, uint64_t target_id, const std::string& message) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    // 检查是否已经是好友
    auto it = friend_cache_.find(role_id);
    if (it != friend_cache_.end()) {
        for (const auto& friend_info : it->second) {
            if (friend_info.friend_id == target_id && friend_info.relation_type == FriendRelationType::FRIEND) {
                ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Already friends: role_id=" << role_id << ", target_id=" << target_id;
                return false;
            }
        }
    }

    // 检查是否已经发送过请求
    auto sent_it = sent_apply_cache_.find(role_id);
    if (sent_it != sent_apply_cache_.end()) {
        for (const auto& apply : sent_it->second) {
            if (apply.target_id == target_id) {
                ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Friend request already sent: role_id=" << role_id << ", target_id=" << target_id;
                return false;
            }
        }
    }

    // 创建请求信息
    FriendApplyInfo apply;
    apply.apply_id = GenerateApplyId();
    apply.requester_id = role_id;
    apply.target_id = target_id;
    apply.message = message;
    apply.status = FriendApplyStatus::PENDING;
    apply.send_time = time(nullptr);

    // 添加到发送请求缓存
    if (sent_apply_cache_.find(role_id) == sent_apply_cache_.end()) {
        sent_apply_cache_[role_id] = std::vector<FriendApplyInfo>();
    }
    sent_apply_cache_[role_id].push_back(apply);

    // 添加到接收请求缓存
    if (apply_cache_.find(target_id) == apply_cache_.end()) {
        apply_cache_[target_id] = std::vector<FriendApplyInfo>();
    }
    apply_cache_[target_id].push_back(apply);

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Friend request sent: role_id=" << role_id << ", target_id=" << target_id;
    return true;
}

bool FriendModule::GetFriendRequests(uint64_t role_id, std::vector<FriendApplyInfo>& requests) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = apply_cache_.find(role_id);
    if (it == apply_cache_.end()) {
        return false;
    }

    requests.clear();
    for (const auto& apply : it->second) {
        if (apply.status == FriendApplyStatus::PENDING) {
            requests.push_back(apply);
        }
    }

    return true;
}

bool FriendModule::RespondToFriendRequest(uint64_t role_id, uint64_t apply_id, bool accept) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = apply_cache_.find(role_id);
    if (it == apply_cache_.end()) {
        return false;
    }

    for (auto& apply : it->second) {
        if (apply.apply_id == apply_id) {
            if (apply.status != FriendApplyStatus::PENDING) {
                ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Friend request already processed: role_id=" << role_id << ", apply_id=" << apply_id;
                return false;
            }

            if (accept) {
                // 接受好友请求
                if (AddFriend(role_id, apply.requester_id)) {
                    apply.status = FriendApplyStatus::ACCEPTED;
                    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Friend request accepted: role_id=" << role_id << ", requester_id=" << apply.requester_id;
                } else {
                    return false;
                }
            } else {
                // 拒绝好友请求
                apply.status = FriendApplyStatus::REJECTED;
                ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Friend request rejected: role_id=" << role_id << ", requester_id=" << apply.requester_id;
            }

            // 更新发送者的请求状态
            auto sent_it = sent_apply_cache_.find(apply.requester_id);
            if (sent_it != sent_apply_cache_.end()) {
                for (auto& sent_apply : sent_it->second) {
                    if (sent_apply.apply_id == apply_id) {
                        sent_apply.status = apply.status;
                        break;
                    }
                }
            }

            return true;
        }
    }

    ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Friend request not found: role_id=" << role_id << ", apply_id=" << apply_id;
    return false;
}

bool FriendModule::CheckFriendLimit(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = friend_cache_.find(role_id);
    if (it == friend_cache_.end()) {
        return true;
    }

    return it->second.size() < MAX_FRIEND_COUNT;
}

bool FriendModule::GetRecentPlayers(uint64_t role_id, std::vector<FriendInfo>& players) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = recent_cache_.find(role_id);
    if (it == recent_cache_.end()) {
        return false;
    }

    players = it->second;
    return true;
}

bool FriendModule::AddRecentPlayer(uint64_t role_id, uint64_t player_id, const std::string& player_name, int32_t level, int32_t profession) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    if (recent_cache_.find(role_id) == recent_cache_.end()) {
        recent_cache_[role_id] = std::vector<FriendInfo>();
    }

    auto& recent_players = recent_cache_[role_id];

    // 检查是否已经在最近列表中
    for (auto& player : recent_players) {
        if (player.friend_id == player_id) {
            // 更新信息
            player.friend_name = player_name;
            player.level = level;
            player.profession = profession;
            player.last_login_time = time(nullptr);
            return true;
        }
    }

    // 添加到最近列表
    FriendInfo info;
    info.friend_id = player_id;
    info.friend_name = player_name;
    info.level = level;
    info.profession = profession;
    info.status = FriendStatus::OFFLINE;
    info.relation_type = FriendRelationType::NONE;
    info.last_login_time = time(nullptr);
    info.friend_since = 0;

    recent_players.push_back(info);

    // 保持最近列表不超过最大数量
    if (recent_players.size() > MAX_RECENT_COUNT) {
        recent_players.erase(recent_players.begin());
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Recent player added: role_id=" << role_id << ", player_id=" << player_id;
    return true;
}

bool FriendModule::DeleteRecentPlayer(uint64_t role_id, uint64_t player_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = recent_cache_.find(role_id);
    if (it == recent_cache_.end()) {
        return false;
    }

    auto player_it = it->second.begin();
    while (player_it != it->second.end()) {
        if (player_it->friend_id == player_id) {
            player_it = it->second.erase(player_it);
            ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Recent player deleted: role_id=" << role_id << ", player_id=" << player_id;
            return true;
        } else {
            ++player_it;
        }
    }

    return false;
}

bool FriendModule::UpdateFriendStatus(uint64_t role_id, uint64_t friend_id, FriendStatus status) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = friend_cache_.find(role_id);
    if (it == friend_cache_.end()) {
        return false;
    }

    for (auto& friend_info : it->second) {
        if (friend_info.friend_id == friend_id) {
            friend_info.status = status;
            if (status == FriendStatus::ONLINE) {
                friend_info.last_login_time = time(nullptr);
            }
            ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Friend status updated: role_id=" << role_id << ", friend_id=" << friend_id << ", status=" << static_cast<int32_t>(status);
            return true;
        }
    }

    return false;
}

bool FriendModule::SaveFriendData(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = friend_cache_.find(role_id);
    if (it == friend_cache_.end()) {
        return false;
    }

    // TODO: 由于 proto/msg_friend.pb.h 不存在，暂时返回 true
    return true;
}

bool FriendModule::LoadFriendData(uint64_t role_id) {
    // 从数据库加载好友数据
    // TODO: 实现从数据库加载好友数据

    // 初始化好友
    InitFriends(role_id);

    return true;
}

uint64_t FriendModule::GenerateApplyId() {
    static uint64_t next_id = time(nullptr) * 10000 + rand() % 10000;
    return next_id++;
}

} // namespace game_server