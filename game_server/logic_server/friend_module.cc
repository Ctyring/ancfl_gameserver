#include "friend_module.h"
#include "proto/msg_friend.pb.h"

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

    LOG_INFO("Friends initialized: role_id=%llu", role_id);
    return true;
}

bool FriendModule::GetFriends(uint64_t role_id,
                              std::vector<FriendInfo>& friends) {
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

bool FriendModule::GetFriendInfo(uint64_t role_id,
                                 uint64_t friend_id,
                                 FriendInfo& info) {
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
        LOG_ERROR("Friend limit reached: role_id=%llu", role_id);
        return false;
    }

    auto it = friend_cache_.find(role_id);
    if (it == friend_cache_.end()) {
        friend_cache_[role_id] = std::vector<FriendInfo>();
        it = friend_cache_.find(role_id);
    }

    // 检查是否已经是好友
    for (const auto& friend_info : it->second) {
        if (friend_info.friend_id == friend_id &&
            friend_info.relation_type == FriendRelationType::FRIEND) {
            LOG_ERROR("Already friends: role_id=%llu, friend_id=%llu", role_id,
                      friend_id);
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
    info.add_time = time(nullptr);
    info.intimacy = 0;
    info.remark = "";
    info.is_online = false;

    it->second.push_back(info);

    LOG_INFO("Friend added: role_id=%llu, friend_id=%llu", role_id, friend_id);
    return true;
}

bool FriendModule::ApplyAddFriend(uint64_t role_id,
                                  uint64_t target_id,
                                  const std::string& message) {
    // 不能添加自己
    if (role_id == target_id) {
        LOG_ERROR("Cannot add self as friend: role_id=%llu", role_id);
        return false;
    }

    // 检查是否已经是好友
    if (IsFriend(role_id, target_id)) {
        LOG_ERROR("Already friends: role_id=%llu, target_id=%llu", role_id,
                  target_id);
        return false;
    }

    // 检查是否在黑名单中
    if (IsInBlacklist(target_id, role_id)) {
        LOG_ERROR("Target has blocked you: role_id=%llu, target_id=%llu",
                  role_id, target_id);
        return false;
    }

    std::lock_guard<std::mutex> lock(cache_mutex_);

    // 添加到对方的申请列表
    auto it = apply_cache_.find(target_id);
    if (it == apply_cache_.end()) {
        apply_cache_[target_id] = std::vector<FriendApplyInfo>();
        it = apply_cache_.find(target_id);
    }

    // 检查是否已有待处理的申请
    for (const auto& apply : it->second) {
        if (apply.applicant_id == role_id &&
            apply.status == FriendApplyStatus::PENDING) {
            LOG_ERROR(
                "Friend apply already pending: role_id=%llu, target_id=%llu",
                role_id, target_id);
            return false;
        }
    }

    // 创建申请
    FriendApplyInfo apply;
    apply.apply_id = GenerateApplyId();
    apply.applicant_id = role_id;
    apply.applicant_name = "";
    apply.applicant_level = 1;
    apply.applicant_profession = 1;
    apply.status = FriendApplyStatus::PENDING;
    apply.apply_time = time(nullptr);
    apply.message = message;

    it->second.push_back(apply);

    // 添加到自己的已发送申请列表
    auto sent_it = sent_apply_cache_.find(role_id);
    if (sent_it == sent_apply_cache_.end()) {
        sent_apply_cache_[role_id] = std::vector<FriendApplyInfo>();
        sent_it = sent_apply_cache_.find(role_id);
    }
    sent_it->second.push_back(apply);

    LOG_INFO("Friend apply sent: role_id=%llu, target_id=%llu, apply_id=%llu",
             role_id, target_id, apply.apply_id);
    return true;
}

bool FriendModule::HandleFriendApply(uint64_t role_id,
                                     uint64_t apply_id,
                                     bool accept) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = apply_cache_.find(role_id);
    if (it == apply_cache_.end()) {
        return false;
    }

    for (auto& apply : it->second) {
        if (apply.apply_id == apply_id &&
            apply.status == FriendApplyStatus::PENDING) {
            if (accept) {
                apply.status = FriendApplyStatus::ACCEPTED;

                // 添加双方为好友
                AddFriend(role_id, apply.applicant_id);
                AddFriend(apply.applicant_id, role_id);

                LOG_INFO(
                    "Friend apply accepted: role_id=%llu, apply_id=%llu, "
                    "applicant_id=%llu",
                    role_id, apply_id, apply.applicant_id);
            } else {
                apply.status = FriendApplyStatus::REJECTED;
                LOG_INFO("Friend apply rejected: role_id=%llu, apply_id=%llu",
                         role_id, apply_id);
            }

            return true;
        }
    }

    return false;
}

bool FriendModule::RemoveFriend(uint64_t role_id, uint64_t friend_id) {
    return DeleteFriend(role_id, friend_id);
}

bool FriendModule::DeleteFriend(uint64_t role_id, uint64_t friend_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = friend_cache_.find(role_id);
    if (it == friend_cache_.end()) {
        return false;
    }

    auto friend_it = std::remove_if(it->second.begin(), it->second.end(),
                                    [friend_id](const FriendInfo& info) {
                                        return info.friend_id == friend_id &&
                                               info.relation_type ==
                                                   FriendRelationType::FRIEND;
                                    });

    if (friend_it == it->second.end()) {
        LOG_ERROR("Friend not found: role_id=%llu, friend_id=%llu", role_id,
                  friend_id);
        return false;
    }

    it->second.erase(friend_it, it->second.end());

    LOG_INFO("Friend deleted: role_id=%llu, friend_id=%llu", role_id,
             friend_id);
    return true;
}

bool FriendModule::GetFriendApplies(uint64_t role_id,
                                    std::vector<FriendApplyInfo>& applies) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = apply_cache_.find(role_id);
    if (it == apply_cache_.end()) {
        return false;
    }

    applies = it->second;
    return true;
}

bool FriendModule::GetSentFriendApplies(uint64_t role_id,
                                        std::vector<FriendApplyInfo>& applies) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = sent_apply_cache_.find(role_id);
    if (it == sent_apply_cache_.end()) {
        return false;
    }

    applies = it->second;
    return true;
}

bool FriendModule::AddToBlacklist(uint64_t role_id, uint64_t target_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = friend_cache_.find(role_id);
    if (it == friend_cache_.end()) {
        friend_cache_[role_id] = std::vector<FriendInfo>();
        it = friend_cache_.find(role_id);
    }

    // 检查黑名单数量
    int32_t blacklist_count = 0;
    for (const auto& info : it->second) {
        if (info.relation_type == FriendRelationType::BLACKLIST) {
            blacklist_count++;
        }
    }

    if (blacklist_count >= MAX_BLACKLIST_COUNT) {
        LOG_ERROR("Blacklist limit reached: role_id=%llu", role_id);
        return false;
    }

    // 如果已经是好友，先删除好友关系
    auto friend_it = std::remove_if(it->second.begin(), it->second.end(),
                                    [target_id](const FriendInfo& info) {
                                        return info.friend_id == target_id &&
                                               info.relation_type ==
                                                   FriendRelationType::FRIEND;
                                    });
    it->second.erase(friend_it, it->second.end());

    // 添加到黑名单
    FriendInfo info;
    info.friend_id = target_id;
    info.friend_name = "";
    info.level = 1;
    info.profession = 1;
    info.status = FriendStatus::OFFLINE;
    info.relation_type = FriendRelationType::BLACKLIST;
    info.add_time = time(nullptr);
    info.intimacy = 0;
    info.remark = "";
    info.is_online = false;

    it->second.push_back(info);

    LOG_INFO("Added to blacklist: role_id=%llu, target_id=%llu", role_id,
             target_id);
    return true;
}

bool FriendModule::RemoveFromBlacklist(uint64_t role_id, uint64_t target_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = friend_cache_.find(role_id);
    if (it == friend_cache_.end()) {
        return false;
    }

    auto blacklist_it = std::remove_if(
        it->second.begin(), it->second.end(),
        [target_id](const FriendInfo& info) {
            return info.friend_id == target_id &&
                   info.relation_type == FriendRelationType::BLACKLIST;
        });

    if (blacklist_it == it->second.end()) {
        LOG_ERROR("Not in blacklist: role_id=%llu, target_id=%llu", role_id,
                  target_id);
        return false;
    }

    it->second.erase(blacklist_it, it->second.end());

    LOG_INFO("Removed from blacklist: role_id=%llu, target_id=%llu", role_id,
             target_id);
    return true;
}

bool FriendModule::GetBlacklist(uint64_t role_id,
                                std::vector<FriendInfo>& blacklist) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = friend_cache_.find(role_id);
    if (it == friend_cache_.end()) {
        return false;
    }

    blacklist.clear();
    for (const auto& info : it->second) {
        if (info.relation_type == FriendRelationType::BLACKLIST) {
            blacklist.push_back(info);
        }
    }

    return true;
}

bool FriendModule::IsInBlacklist(uint64_t role_id, uint64_t target_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = friend_cache_.find(role_id);
    if (it == friend_cache_.end()) {
        return false;
    }

    for (const auto& info : it->second) {
        if (info.friend_id == target_id &&
            info.relation_type == FriendRelationType::BLACKLIST) {
            return true;
        }
    }

    return false;
}

bool FriendModule::AddRecentContact(uint64_t role_id, uint64_t contact_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = recent_cache_.find(role_id);
    if (it == recent_cache_.end()) {
        recent_cache_[role_id] = std::vector<FriendInfo>();
        it = recent_cache_.find(role_id);
    }

    // 检查是否已存在
    for (auto& info : it->second) {
        if (info.friend_id == contact_id) {
            // 移动到最前面
            FriendInfo temp = info;
            temp.add_time = time(nullptr);
            it->second.erase(
                std::remove_if(it->second.begin(), it->second.end(),
                               [contact_id](const FriendInfo& f) {
                                   return f.friend_id == contact_id;
                               }),
                it->second.end());
            it->second.insert(it->second.begin(), temp);
            return true;
        }
    }

    // 添加新联系人
    FriendInfo info;
    info.friend_id = contact_id;
    info.friend_name = "";
    info.level = 1;
    info.profession = 1;
    info.status = FriendStatus::OFFLINE;
    info.relation_type = FriendRelationType::RECENT;
    info.add_time = time(nullptr);
    info.intimacy = 0;
    info.remark = "";
    info.is_online = false;

    it->second.insert(it->second.begin(), info);

    // 限制数量
    if (it->second.size() > MAX_RECENT_COUNT) {
        it->second.resize(MAX_RECENT_COUNT);
    }

    return true;
}

bool FriendModule::GetRecentContacts(uint64_t role_id,
                                     std::vector<FriendInfo>& contacts) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = recent_cache_.find(role_id);
    if (it == recent_cache_.end()) {
        return false;
    }

    contacts = it->second;
    return true;
}

bool FriendModule::ClearRecentContacts(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = recent_cache_.find(role_id);
    if (it == recent_cache_.end()) {
        return false;
    }

    it->second.clear();
    LOG_INFO("Recent contacts cleared: role_id=%llu", role_id);
    return true;
}

bool FriendModule::UpdateFriendStatus(uint64_t role_id, FriendStatus status) {
    // TODO: 通知所有好友状态变化
    return true;
}

bool FriendModule::NotifyFriendStatusChange(uint64_t role_id,
                                            FriendStatus status) {
    // TODO: 发送状态变化通知给好友
    return true;
}

bool FriendModule::IsFriend(uint64_t role_id, uint64_t target_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = friend_cache_.find(role_id);
    if (it == friend_cache_.end()) {
        return false;
    }

    for (const auto& info : it->second) {
        if (info.friend_id == target_id &&
            info.relation_type == FriendRelationType::FRIEND) {
            return true;
        }
    }

    return false;
}

bool FriendModule::AddIntimacy(uint64_t role_id,
                               uint64_t friend_id,
                               int32_t value) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = friend_cache_.find(role_id);
    if (it == friend_cache_.end()) {
        return false;
    }

    for (auto& info : it->second) {
        if (info.friend_id == friend_id &&
            info.relation_type == FriendRelationType::FRIEND) {
            info.intimacy += value;
            if (info.intimacy < 0) {
                info.intimacy = 0;
            }
            LOG_INFO(
                "Intimacy added: role_id=%llu, friend_id=%llu, value=%d, "
                "total=%d",
                role_id, friend_id, value, info.intimacy);
            return true;
        }
    }

    return false;
}

bool FriendModule::GetIntimacy(uint64_t role_id,
                               uint64_t friend_id,
                               int32_t& intimacy) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = friend_cache_.find(role_id);
    if (it == friend_cache_.end()) {
        return false;
    }

    for (const auto& info : it->second) {
        if (info.friend_id == friend_id) {
            intimacy = info.intimacy;
            return true;
        }
    }

    return false;
}

bool FriendModule::SetRemark(uint64_t role_id,
                             uint64_t friend_id,
                             const std::string& remark) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = friend_cache_.find(role_id);
    if (it == friend_cache_.end()) {
        return false;
    }

    for (auto& info : it->second) {
        if (info.friend_id == friend_id &&
            info.relation_type == FriendRelationType::FRIEND) {
            info.remark = remark;
            LOG_INFO("Remark set: role_id=%llu, friend_id=%llu, remark=%s",
                     role_id, friend_id, remark.c_str());
            return true;
        }
    }

    return false;
}

bool FriendModule::GetRemark(uint64_t role_id,
                             uint64_t friend_id,
                             std::string& remark) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = friend_cache_.find(role_id);
    if (it == friend_cache_.end()) {
        return false;
    }

    for (const auto& info : it->second) {
        if (info.friend_id == friend_id) {
            remark = info.remark;
            return true;
        }
    }

    return false;
}

bool FriendModule::GetRecommendedFriends(uint64_t role_id,
                                         std::vector<FriendInfo>& friends) {
    // TODO: 根据等级、职业等推荐好友
    friends.clear();
    return true;
}

bool FriendModule::SearchFriend(const std::string& name,
                                std::vector<FriendInfo>& friends) {
    // TODO: 从数据库搜索好友
    friends.clear();
    return true;
}

bool FriendModule::LoadFriendData(uint64_t role_id) {
    // TODO: 从数据库加载好友数据
    InitFriends(role_id);
    return true;
}

bool FriendModule::SaveFriendData(uint64_t role_id) {
    // TODO: 保存好友数据到数据库
    return true;
}

void FriendModule::OnTimer() {
    // TODO: 清理过期的申请
}

uint64_t FriendModule::GenerateApplyId() {
    static uint64_t next_id = time(nullptr) * 10000 + rand() % 10000;
    return next_id++;
}

bool FriendModule::CheckFriendLimit(uint64_t role_id) {
    auto it = friend_cache_.find(role_id);
    if (it == friend_cache_.end()) {
        return true;
    }

    int32_t friend_count = 0;
    for (const auto& info : it->second) {
        if (info.relation_type == FriendRelationType::FRIEND) {
            friend_count++;
        }
    }

    return friend_count < MAX_FRIEND_COUNT;
}

}  // namespace game_server
