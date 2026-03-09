#include "guild_module.h"
#include "proto/msg_guild.pb.h"

namespace game_server {

GuildModule::GuildModule(LogicService* service) : service_(service) {}

GuildModule::~GuildModule() {}

bool GuildModule::CreateGuild(uint64_t role_id,
                              const std::string& guild_name,
                              uint64_t& guild_id) {
    // 检查是否已加入公会
    if (IsInGuild(role_id)) {
        LOG_ERROR("Already in guild: role_id=%llu", role_id);
        return false;
    }

    // 检查公会名称
    if (guild_name.empty() || guild_name.length() > MAX_GUILD_NAME_LENGTH) {
        LOG_ERROR("Invalid guild name: role_id=%llu, name=%s", role_id,
                  guild_name.c_str());
        return false;
    }

    // 检查公会名称是否已存在
    if (IsGuildNameExist(guild_name)) {
        LOG_ERROR("Guild name already exists: name=%s", guild_name.c_str());
        return false;
    }

    std::lock_guard<std::mutex> lock(cache_mutex_);

    // TODO: 检查创建公会的条件（金币、道具等）

    // 创建公会
    guild_id = GenerateGuildId();

    GuildInfo guild;
    guild.guild_id = guild_id;
    guild.guild_name = guild_name;
    guild.leader_id = role_id;
    guild.leader_name = "";
    guild.level = 1;
    guild.exp = 0;
    guild.member_count = 1;
    guild.max_member_count = 30;
    guild.fund = 0;
    guild.announcement = "";
    guild.description = "";
    guild.join_condition_level = 1;
    guild.need_approval = true;
    guild.create_time = time(nullptr);
    guild.is_active = true;

    guild_cache_[guild_id] = guild;

    // 添加会长为成员
    GuildMember leader;
    leader.role_id = role_id;
    leader.role_name = "";
    leader.level = 1;
    leader.profession = 1;
    leader.position = GuildPosition::LEADER;
    leader.contribution = 0;
    leader.total_contribution = 0;
    leader.join_time = time(nullptr);
    leader.last_active_time = time(nullptr);
    leader.is_online = true;

    guild_members_cache_[guild_id] = std::vector<GuildMember>();
    guild_members_cache_[guild_id].push_back(leader);

    // 更新玩家公会映射
    role_guild_map_[role_id] = guild_id;

    LOG_INFO("Guild created: guild_id=%llu, name=%s, leader_id=%llu", guild_id,
             guild_name.c_str(), role_id);
    return true;
}

bool GuildModule::DismissGuild(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    // 检查是否在公会中
    auto guild_it = role_guild_map_.find(role_id);
    if (guild_it == role_guild_map_.end()) {
        LOG_ERROR("Not in guild: role_id=%llu", role_id);
        return false;
    }

    uint64_t guild_id = guild_it->second;

    // 检查是否是会长
    auto info_it = guild_cache_.find(guild_id);
    if (info_it == guild_cache_.end()) {
        return false;
    }

    if (info_it->second.leader_id != role_id) {
        LOG_ERROR("Not guild leader: role_id=%llu, guild_id=%llu", role_id,
                  guild_id);
        return false;
    }

    // 清理公会数据
    guild_members_cache_.erase(guild_id);
    guild_applies_cache_.erase(guild_id);
    guild_skills_cache_.erase(guild_id);
    guild_cache_.erase(guild_id);

    // 清理成员的公会映射
    auto members_it = guild_members_cache_.find(guild_id);
    if (members_it != guild_members_cache_.end()) {
        for (const auto& member : members_it->second) {
            role_guild_map_.erase(member.role_id);
        }
    }

    role_guild_map_.erase(role_id);

    LOG_INFO("Guild dismissed: guild_id=%llu, leader_id=%llu", guild_id,
             role_id);
    return true;
}

bool GuildModule::GetGuildInfo(uint64_t guild_id, GuildInfo& info) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = guild_cache_.find(guild_id);
    if (it == guild_cache_.end()) {
        return false;
    }

    info = it->second;
    return true;
}

bool GuildModule::GetGuildByName(const std::string& guild_name,
                                 GuildInfo& info) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    for (const auto& pair : guild_cache_) {
        if (pair.second.guild_name == guild_name) {
            info = pair.second;
            return true;
        }
    }

    return false;
}

bool GuildModule::IsGuildNameExist(const std::string& guild_name) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    for (const auto& pair : guild_cache_) {
        if (pair.second.guild_name == guild_name) {
            return true;
        }
    }

    return false;
}

bool GuildModule::GetGuildMembers(uint64_t guild_id,
                                  std::vector<GuildMember>& members) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = guild_members_cache_.find(guild_id);
    if (it == guild_members_cache_.end()) {
        return false;
    }

    members = it->second;
    return true;
}

bool GuildModule::GetMemberInfo(uint64_t guild_id,
                                uint64_t role_id,
                                GuildMember& member) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = guild_members_cache_.find(guild_id);
    if (it == guild_members_cache_.end()) {
        return false;
    }

    for (const auto& m : it->second) {
        if (m.role_id == role_id) {
            member = m;
            return true;
        }
    }

    return false;
}

bool GuildModule::GetMemberGuild(uint64_t role_id, uint64_t& guild_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = role_guild_map_.find(role_id);
    if (it == role_guild_map_.end()) {
        return false;
    }

    guild_id = it->second;
    return true;
}

bool GuildModule::IsInGuild(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return role_guild_map_.find(role_id) != role_guild_map_.end();
}

bool GuildModule::ApplyJoinGuild(uint64_t role_id,
                                 uint64_t guild_id,
                                 const std::string& message) {
    // 检查是否已加入公会
    if (IsInGuild(role_id)) {
        LOG_ERROR("Already in guild: role_id=%llu", role_id);
        return false;
    }

    std::lock_guard<std::mutex> lock(cache_mutex_);

    // 检查公会是否存在
    auto guild_it = guild_cache_.find(guild_id);
    if (guild_it == guild_cache_.end()) {
        LOG_ERROR("Guild not found: guild_id=%llu", guild_id);
        return false;
    }

    // 检查公会是否满员
    if (IsGuildFull(guild_id)) {
        LOG_ERROR("Guild is full: guild_id=%llu", guild_id);
        return false;
    }

    // 检查是否需要审核
    if (!guild_it->second.need_approval) {
        // 直接加入
        return DirectJoinGuild(role_id, guild_id);
    }

    // 创建申请
    GuildApplyInfo apply;
    apply.apply_id = GenerateApplyId();
    apply.role_id = role_id;
    apply.role_name = "";
    apply.level = 1;
    apply.profession = 1;
    apply.status = GuildApplyStatus::PENDING;
    apply.apply_time = time(nullptr);
    apply.message = message;

    // 添加到公会申请列表
    auto applies_it = guild_applies_cache_.find(guild_id);
    if (applies_it == guild_applies_cache_.end()) {
        guild_applies_cache_[guild_id] = std::vector<GuildApplyInfo>();
        applies_it = guild_applies_cache_.find(guild_id);
    }
    applies_it->second.push_back(apply);

    // 添加到玩家申请列表
    auto role_applies_it = role_applies_cache_.find(role_id);
    if (role_applies_it == role_applies_cache_.end()) {
        role_applies_cache_[role_id] = std::vector<GuildApplyInfo>();
        role_applies_it = role_applies_cache_.find(role_id);
    }
    role_applies_it->second.push_back(apply);

    LOG_INFO(
        "Guild apply submitted: role_id=%llu, guild_id=%llu, apply_id=%llu",
        role_id, guild_id, apply.apply_id);
    return true;
}

bool GuildModule::HandleApply(uint64_t guild_id,
                              uint64_t apply_id,
                              bool accept) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto applies_it = guild_applies_cache_.find(guild_id);
    if (applies_it == guild_applies_cache_.end()) {
        return false;
    }

    for (auto& apply : applies_it->second) {
        if (apply.apply_id == apply_id &&
            apply.status == GuildApplyStatus::PENDING) {
            if (accept) {
                apply.status = GuildApplyStatus::ACCEPTED;

                // 添加成员
                GuildMember member;
                member.role_id = apply.role_id;
                member.role_name = apply.role_name;
                member.level = apply.level;
                member.profession = apply.profession;
                member.position = GuildPosition::MEMBER;
                member.contribution = 0;
                member.total_contribution = 0;
                member.join_time = time(nullptr);
                member.last_active_time = time(nullptr);
                member.is_online = true;

                auto members_it = guild_members_cache_.find(guild_id);
                if (members_it != guild_members_cache_.end()) {
                    members_it->second.push_back(member);
                }

                // 更新公会成员数
                auto guild_it = guild_cache_.find(guild_id);
                if (guild_it != guild_cache_.end()) {
                    guild_it->second.member_count++;
                }

                // 更新玩家公会映射
                role_guild_map_[apply.role_id] = guild_id;

                LOG_INFO(
                    "Guild apply accepted: guild_id=%llu, apply_id=%llu, "
                    "role_id=%llu",
                    guild_id, apply_id, apply.role_id);
            } else {
                apply.status = GuildApplyStatus::REJECTED;
                LOG_INFO("Guild apply rejected: guild_id=%llu, apply_id=%llu",
                         guild_id, apply_id);
            }

            return true;
        }
    }

    return false;
}

bool GuildModule::DirectJoinGuild(uint64_t role_id, uint64_t guild_id) {
    // 添加成员
    GuildMember member;
    member.role_id = role_id;
    member.role_name = "";
    member.level = 1;
    member.profession = 1;
    member.position = GuildPosition::MEMBER;
    member.contribution = 0;
    member.total_contribution = 0;
    member.join_time = time(nullptr);
    member.last_active_time = time(nullptr);
    member.is_online = true;

    auto members_it = guild_members_cache_.find(guild_id);
    if (members_it == guild_members_cache_.end()) {
        guild_members_cache_[guild_id] = std::vector<GuildMember>();
        members_it = guild_members_cache_.find(guild_id);
    }
    members_it->second.push_back(member);

    // 更新公会成员数
    auto guild_it = guild_cache_.find(guild_id);
    if (guild_it != guild_cache_.end()) {
        guild_it->second.member_count++;
    }

    // 更新玩家公会映射
    role_guild_map_[role_id] = guild_id;

    LOG_INFO("Direct join guild: role_id=%llu, guild_id=%llu", role_id,
             guild_id);
    return true;
}

bool GuildModule::LeaveGuild(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    // 检查是否在公会中
    auto guild_it = role_guild_map_.find(role_id);
    if (guild_it == role_guild_map_.end()) {
        LOG_ERROR("Not in guild: role_id=%llu", role_id);
        return false;
    }

    uint64_t guild_id = guild_it->second;

    // 检查是否是会长
    auto info_it = guild_cache_.find(guild_id);
    if (info_it != guild_cache_.end() && info_it->second.leader_id == role_id) {
        LOG_ERROR("Guild leader cannot leave: role_id=%llu", role_id);
        return false;
    }

    // 从成员列表中移除
    auto members_it = guild_members_cache_.find(guild_id);
    if (members_it != guild_members_cache_.end()) {
        auto member_it = std::remove_if(
            members_it->second.begin(), members_it->second.end(),
            [role_id](const GuildMember& m) { return m.role_id == role_id; });
        members_it->second.erase(member_it, members_it->second.end());
    }

    // 更新公会成员数
    if (info_it != guild_cache_.end()) {
        info_it->second.member_count--;
    }

    // 移除玩家公会映射
    role_guild_map_.erase(role_id);

    LOG_INFO("Left guild: role_id=%llu, guild_id=%llu", role_id, guild_id);
    return true;
}

bool GuildModule::KickMember(uint64_t operator_id, uint64_t target_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    // 检查操作者是否在公会中
    auto operator_guild_it = role_guild_map_.find(operator_id);
    if (operator_guild_it == role_guild_map_.end()) {
        LOG_ERROR("Operator not in guild: operator_id=%llu", operator_id);
        return false;
    }

    // 检查目标是否在同一公会
    auto target_guild_it = role_guild_map_.find(target_id);
    if (target_guild_it == role_guild_map_.end() ||
        target_guild_it->second != operator_guild_it->second) {
        LOG_ERROR("Target not in same guild: operator_id=%llu, target_id=%llu",
                  operator_id, target_id);
        return false;
    }

    uint64_t guild_id = operator_guild_it->second;

    // 检查权限
    if (!HasPermission(operator_id, guild_id, GuildPosition::ELDER)) {
        LOG_ERROR("No permission to kick: operator_id=%llu", operator_id);
        return false;
    }

    // 不能踢会长
    auto info_it = guild_cache_.find(guild_id);
    if (info_it != guild_cache_.end() &&
        info_it->second.leader_id == target_id) {
        LOG_ERROR("Cannot kick leader: target_id=%llu", target_id);
        return false;
    }

    // 从成员列表中移除
    auto members_it = guild_members_cache_.find(guild_id);
    if (members_it != guild_members_cache_.end()) {
        auto member_it =
            std::remove_if(members_it->second.begin(), members_it->second.end(),
                           [target_id](const GuildMember& m) {
                               return m.role_id == target_id;
                           });
        members_it->second.erase(member_it, members_it->second.end());
    }

    // 更新公会成员数
    if (info_it != guild_cache_.end()) {
        info_it->second.member_count--;
    }

    // 移除玩家公会映射
    role_guild_map_.erase(target_id);

    LOG_INFO("Member kicked: guild_id=%llu, operator_id=%llu, target_id=%llu",
             guild_id, operator_id, target_id);
    return true;
}

bool GuildModule::SetMemberPosition(uint64_t operator_id,
                                    uint64_t target_id,
                                    GuildPosition position) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    // 检查操作者是否在公会中
    auto operator_guild_it = role_guild_map_.find(operator_id);
    if (operator_guild_it == role_guild_map_.end()) {
        return false;
    }

    uint64_t guild_id = operator_guild_it->second;

    // 检查权限
    if (!HasPermission(operator_id, guild_id, GuildPosition::LEADER)) {
        return false;
    }

    // 更新成员职位
    auto members_it = guild_members_cache_.find(guild_id);
    if (members_it != guild_members_cache_.end()) {
        for (auto& member : members_it->second) {
            if (member.role_id == target_id) {
                member.position = position;
                LOG_INFO(
                    "Member position set: guild_id=%llu, target_id=%llu, "
                    "position=%d",
                    guild_id, target_id, static_cast<int32_t>(position));
                return true;
            }
        }
    }

    return false;
}

bool GuildModule::TransferLeader(uint64_t operator_id, uint64_t target_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    // 检查操作者是否在公会中
    auto operator_guild_it = role_guild_map_.find(operator_id);
    if (operator_guild_it == role_guild_map_.end()) {
        return false;
    }

    uint64_t guild_id = operator_guild_it->second;

    // 检查是否是会长
    auto info_it = guild_cache_.find(guild_id);
    if (info_it == guild_cache_.end() ||
        info_it->second.leader_id != operator_id) {
        return false;
    }

    // 更新会长
    auto members_it = guild_members_cache_.find(guild_id);
    if (members_it != guild_members_cache_.end()) {
        for (auto& member : members_it->second) {
            if (member.role_id == operator_id) {
                member.position = GuildPosition::MEMBER;
            }
            if (member.role_id == target_id) {
                member.position = GuildPosition::LEADER;
                info_it->second.leader_id = target_id;
                info_it->second.leader_name = member.role_name;
            }
        }
    }

    LOG_INFO(
        "Leader transferred: guild_id=%llu, old_leader=%llu, new_leader=%llu",
        guild_id, operator_id, target_id);
    return true;
}

bool GuildModule::GetGuildApplies(uint64_t guild_id,
                                  std::vector<GuildApplyInfo>& applies) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = guild_applies_cache_.find(guild_id);
    if (it == guild_applies_cache_.end()) {
        return false;
    }

    applies = it->second;
    return true;
}

bool GuildModule::GetMyApplies(uint64_t role_id,
                               std::vector<GuildApplyInfo>& applies) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = role_applies_cache_.find(role_id);
    if (it == role_applies_cache_.end()) {
        return false;
    }

    applies = it->second;
    return true;
}

bool GuildModule::AddContribution(uint64_t role_id, int32_t value) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto guild_it = role_guild_map_.find(role_id);
    if (guild_it == role_guild_map_.end()) {
        return false;
    }

    uint64_t guild_id = guild_it->second;

    auto members_it = guild_members_cache_.find(guild_id);
    if (members_it != guild_members_cache_.end()) {
        for (auto& member : members_it->second) {
            if (member.role_id == role_id) {
                member.contribution += value;
                member.total_contribution += value;
                LOG_INFO("Contribution added: role_id=%llu, value=%d, total=%d",
                         role_id, value, member.contribution);
                return true;
            }
        }
    }

    return false;
}

bool GuildModule::GetContribution(uint64_t role_id, int32_t& contribution) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto guild_it = role_guild_map_.find(role_id);
    if (guild_it == role_guild_map_.end()) {
        return false;
    }

    uint64_t guild_id = guild_it->second;

    auto members_it = guild_members_cache_.find(guild_id);
    if (members_it != guild_members_cache_.end()) {
        for (const auto& member : members_it->second) {
            if (member.role_id == role_id) {
                contribution = member.contribution;
                return true;
            }
        }
    }

    return false;
}

bool GuildModule::UseContribution(uint64_t role_id, int32_t value) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto guild_it = role_guild_map_.find(role_id);
    if (guild_it == role_guild_map_.end()) {
        return false;
    }

    uint64_t guild_id = guild_it->second;

    auto members_it = guild_members_cache_.find(guild_id);
    if (members_it != guild_members_cache_.end()) {
        for (auto& member : members_it->second) {
            if (member.role_id == role_id) {
                if (member.contribution < value) {
                    return false;
                }
                member.contribution -= value;
                return true;
            }
        }
    }

    return false;
}

bool GuildModule::AddGuildFund(uint64_t guild_id, int32_t value) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = guild_cache_.find(guild_id);
    if (it == guild_cache_.end()) {
        return false;
    }

    it->second.fund += value;
    return true;
}

bool GuildModule::UseGuildFund(uint64_t guild_id, int32_t value) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = guild_cache_.find(guild_id);
    if (it == guild_cache_.end()) {
        return false;
    }

    if (it->second.fund < value) {
        return false;
    }

    it->second.fund -= value;
    return true;
}

bool GuildModule::GetGuildFund(uint64_t guild_id, int32_t& fund) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = guild_cache_.find(guild_id);
    if (it == guild_cache_.end()) {
        return false;
    }

    fund = it->second.fund;
    return true;
}

bool GuildModule::SetAnnouncement(uint64_t role_id,
                                  const std::string& announcement) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto guild_it = role_guild_map_.find(role_id);
    if (guild_it == role_guild_map_.end()) {
        return false;
    }

    uint64_t guild_id = guild_it->second;

    // 检查权限
    if (!HasPermission(role_id, guild_id, GuildPosition::ELDER)) {
        return false;
    }

    auto it = guild_cache_.find(guild_id);
    if (it != guild_cache_.end()) {
        it->second.announcement = announcement;
        LOG_INFO("Announcement set: guild_id=%llu", guild_id);
        return true;
    }

    return false;
}

bool GuildModule::GetAnnouncement(uint64_t guild_id,
                                  std::string& announcement) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = guild_cache_.find(guild_id);
    if (it == guild_cache_.end()) {
        return false;
    }

    announcement = it->second.announcement;
    return true;
}

bool GuildModule::SetJoinCondition(uint64_t role_id,
                                   int32_t level,
                                   bool need_approval) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto guild_it = role_guild_map_.find(role_id);
    if (guild_it == role_guild_map_.end()) {
        return false;
    }

    uint64_t guild_id = guild_it->second;

    // 检查权限
    if (!HasPermission(role_id, guild_id, GuildPosition::LEADER)) {
        return false;
    }

    auto it = guild_cache_.find(guild_id);
    if (it != guild_cache_.end()) {
        it->second.join_condition_level = level;
        it->second.need_approval = need_approval;
        return true;
    }

    return false;
}

bool GuildModule::GetGuildSkills(uint64_t guild_id,
                                 std::vector<GuildSkill>& skills) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = guild_skills_cache_.find(guild_id);
    if (it == guild_skills_cache_.end()) {
        return false;
    }

    skills = it->second;
    return true;
}

bool GuildModule::SearchGuild(const std::string& name,
                              std::vector<GuildInfo>& guilds) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    guilds.clear();
    for (const auto& pair : guild_cache_) {
        if (pair.second.guild_name.find(name) != std::string::npos) {
            guilds.push_back(pair.second);
        }
    }

    return true;
}

bool GuildModule::GetGuildList(std::vector<GuildInfo>& guilds) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    guilds.clear();
    for (const auto& pair : guild_cache_) {
        guilds.push_back(pair.second);
    }

    return true;
}

bool GuildModule::LoadGuildData(uint64_t guild_id) {
    // TODO: 从数据库加载公会数据
    return true;
}

bool GuildModule::SaveGuildData(uint64_t guild_id) {
    // TODO: 保存公会数据到数据库
    return true;
}

bool GuildModule::LoadMemberGuildData(uint64_t role_id) {
    // TODO: 从数据库加载玩家公会数据
    return true;
}

bool GuildModule::SaveMemberGuildData(uint64_t role_id) {
    // TODO: 保存玩家公会数据到数据库
    return true;
}

void GuildModule::OnTimer() {
    // TODO: 定时清理过期的申请
}

uint64_t GuildModule::GenerateGuildId() {
    static uint64_t next_id = time(nullptr) * 10000 + rand() % 10000;
    return next_id++;
}

uint64_t GuildModule::GenerateApplyId() {
    static uint64_t next_id = time(nullptr) * 10000 + rand() % 10000;
    return next_id++;
}

bool GuildModule::HasPermission(uint64_t role_id,
                                uint64_t guild_id,
                                GuildPosition min_position) {
    auto members_it = guild_members_cache_.find(guild_id);
    if (members_it == guild_members_cache_.end()) {
        return false;
    }

    for (const auto& member : members_it->second) {
        if (member.role_id == role_id) {
            return static_cast<int32_t>(member.position) >=
                   static_cast<int32_t>(min_position);
        }
    }

    return false;
}

bool GuildModule::IsGuildFull(uint64_t guild_id) {
    auto guild_it = guild_cache_.find(guild_id);
    if (guild_it == guild_cache_.end()) {
        return true;
    }

    return guild_it->second.member_count >= guild_it->second.max_member_count;
}

}  // namespace game_server
