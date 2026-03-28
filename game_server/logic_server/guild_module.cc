#include "guild_module.h"
#include "ancfl/log.h"
#include "proto/msg_guild.pb.h"
#include <mutex>

namespace game_server {

GuildModule::GuildModule(LogicService* service)
    : service_(service) {
    LoadGuildData();
}

GuildModule::~GuildModule() {
    SaveGuildData();
}

bool GuildModule::CreateGuild(uint64_t role_id, const std::string& guild_name, uint64_t& guild_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    if (IsInGuild(role_id)) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Already in guild: role_id=" << role_id;
        return false;
    }

    if (IsGuildNameExist(guild_name)) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Guild name already exists: " << guild_name;
        return false;
    }

    if (guild_name.length() > MAX_GUILD_NAME_LENGTH) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Guild name too long: " << guild_name.length();
        return false;
    }

    guild_id = GenerateGuildId();

    msg_guild::GuildInfo guild;
    guild.set_guild_id(guild_id);
    guild.set_guild_name(guild_name);
    guild.set_leader_id(role_id);
    guild.set_level(1);
    guild.set_exp(0);
    guild.set_member_count(1);
    guild.set_max_member_count(50);
    guild.set_fund(0);
    guild.set_announcement("");
    guild.set_description("");
    guild.set_create_time(time(nullptr));
    guild.set_need_approval(true);

    guild_cache_[guild_id] = guild;
    role_guild_map_[role_id] = guild_id;

    msg_guild::GuildMemberInfo member;
    member.set_role_id(role_id);
    member.set_role_name("");
    member.set_level(0);
    member.set_profession(0);
    member.set_position(static_cast<int32_t>(GuildPosition::LEADER));
    member.set_contribution(0);
    member.set_total_contribution(0);
    member.set_join_time(time(nullptr));
    member.set_last_active_time(time(nullptr));
    member.set_is_online(true);

    member_cache_[guild_id][role_id] = member;

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Guild created: guild_id=" << guild_id << ", guild_name=" << guild_name;
    return true;
}

bool GuildModule::JoinGuild(uint64_t role_id, uint64_t guild_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    if (IsInGuild(role_id)) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Already in guild: role_id=" << role_id;
        return false;
    }

    auto it = guild_cache_.find(guild_id);
    if (it == guild_cache_.end()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Guild not found: guild_id=" << guild_id;
        return false;
    }

    msg_guild::GuildInfo& guild = it->second;
    if (guild.member_count() >= guild.max_member_count()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Guild is full: guild_id=" << guild_id;
        return false;
    }

    msg_guild::GuildMemberInfo member;
    member.set_role_id(role_id);
    member.set_role_name("");
    member.set_level(0);
    member.set_profession(0);
    member.set_position(static_cast<int32_t>(GuildPosition::MEMBER));
    member.set_contribution(0);
    member.set_total_contribution(0);
    member.set_join_time(time(nullptr));
    member.set_last_active_time(time(nullptr));
    member.set_is_online(true);

    member_cache_[guild_id][role_id] = member;
    role_guild_map_[role_id] = guild_id;
    guild.set_member_count(guild.member_count() + 1);

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Joined guild: role_id=" << role_id << ", guild_id=" << guild_id;
    return true;
}

bool GuildModule::LeaveGuild(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = role_guild_map_.find(role_id);
    if (it == role_guild_map_.end()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Not in guild: role_id=" << role_id;
        return false;
    }

    uint64_t guild_id = it->second;
    auto guild_it = guild_cache_.find(guild_id);
    if (guild_it == guild_cache_.end()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Guild not found: guild_id=" << guild_id;
        return false;
    }

    msg_guild::GuildInfo& guild = guild_it->second;
    if (guild.leader_id() == role_id) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Cannot leave guild as leader: role_id=" << role_id;
        return false;
    }

    member_cache_[guild_id].erase(role_id);
    role_guild_map_.erase(role_id);
    guild.set_member_count(guild.member_count() - 1);

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Left guild: role_id=" << role_id << ", guild_id=" << guild_id;
    return true;
}

bool GuildModule::KickMember(uint64_t leader_id, uint64_t target_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = role_guild_map_.find(leader_id);
    if (it == role_guild_map_.end()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Not in guild: role_id=" << leader_id;
        return false;
    }

    uint64_t guild_id = it->second;
    auto guild_it = guild_cache_.find(guild_id);
    if (guild_it == guild_cache_.end()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Guild not found: guild_id=" << guild_id;
        return false;
    }

    msg_guild::GuildInfo& guild = guild_it->second;
    if (guild.leader_id() != leader_id) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Not guild leader: role_id=" << leader_id;
        return false;
    }

    if (guild.leader_id() == target_id) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Cannot kick leader: role_id=" << target_id;
        return false;
    }

    auto member_it = member_cache_[guild_id].find(target_id);
    if (member_it == member_cache_[guild_id].end()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Member not found: role_id=" << target_id;
        return false;
    }

    member_cache_[guild_id].erase(member_it);
    role_guild_map_.erase(target_id);
    guild.set_member_count(guild.member_count() - 1);

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Kicked member: leader_id=" << leader_id << ", target_id=" << target_id;
    return true;
}

bool GuildModule::DissolveGuild(uint64_t leader_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = role_guild_map_.find(leader_id);
    if (it == role_guild_map_.end()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Not in guild: role_id=" << leader_id;
        return false;
    }

    uint64_t guild_id = it->second;
    auto guild_it = guild_cache_.find(guild_id);
    if (guild_it == guild_cache_.end()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Guild not found: guild_id=" << guild_id;
        return false;
    }

    msg_guild::GuildInfo& guild = guild_it->second;
    if (guild.leader_id() != leader_id) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Not guild leader: role_id=" << leader_id;
        return false;
    }

    for (const auto& member_pair : member_cache_[guild_id]) {
        role_guild_map_.erase(member_pair.first);
    }
    member_cache_.erase(guild_id);
    guild_cache_.erase(guild_id);

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Guild dissolved: guild_id=" << guild_id;
    return true;
}

bool GuildModule::GetGuildInfo(uint64_t guild_id, GuildInfo& info) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = guild_cache_.find(guild_id);
    if (it == guild_cache_.end()) {
        return false;
    }

    const msg_guild::GuildInfo& guild = it->second;
    info.guild_id = guild.guild_id();
    info.guild_name = guild.guild_name();
    info.leader_id = guild.leader_id();
    info.leader_name = guild.leader_name();
    info.level = guild.level();
    info.exp = guild.exp();
    info.member_count = guild.member_count();
    info.max_member_count = guild.max_member_count();
    info.fund = guild.fund();
    info.announcement = guild.announcement();
    info.description = guild.description();
    info.create_time = guild.create_time();

    return true;
}

bool GuildModule::GetGuildMembers(uint64_t guild_id, std::vector<GuildMemberInfo>& members) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = member_cache_.find(guild_id);
    if (it == member_cache_.end()) {
        return false;
    }

    members.clear();
    for (const auto& member_pair : it->second) {
        const msg_guild::GuildMemberInfo& member = member_pair.second;
        GuildMemberInfo info;
        info.role_id = member.role_id();
        info.role_name = member.role_name();
        info.level = member.level();
        info.profession = member.profession();
        info.position = static_cast<GuildPosition>(member.position());
        info.contribution = member.contribution();
        info.total_contribution = member.total_contribution();
        info.join_time = member.join_time();
        info.last_login_time = member.last_active_time();
        members.push_back(info);
    }

    return true;
}

bool GuildModule::GetGuildMemberInfo(uint64_t role_id, GuildMemberInfo& info) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = role_guild_map_.find(role_id);
    if (it == role_guild_map_.end()) {
        return false;
    }

    uint64_t guild_id = it->second;
    auto member_it = member_cache_[guild_id].find(role_id);
    if (member_it == member_cache_[guild_id].end()) {
        return false;
    }

    const msg_guild::GuildMemberInfo& member = member_it->second;
    info.role_id = member.role_id();
    info.role_name = member.role_name();
    info.level = member.level();
    info.profession = member.profession();
    info.position = static_cast<GuildPosition>(member.position());
    info.contribution = member.contribution();
    info.total_contribution = member.total_contribution();
    info.join_time = member.join_time();
    info.last_login_time = member.last_active_time();

    return true;
}

bool GuildModule::IsInGuild(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return role_guild_map_.find(role_id) != role_guild_map_.end();
}

bool GuildModule::IsGuildNameExist(const std::string& guild_name) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    for (const auto& pair : guild_cache_) {
        if (pair.second.guild_name() == guild_name) {
            return true;
        }
    }
    return false;
}

bool GuildModule::Contribute(uint64_t role_id, int32_t contribution_type, int32_t amount) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = role_guild_map_.find(role_id);
    if (it == role_guild_map_.end()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Not in guild: role_id=" << role_id;
        return false;
    }

    uint64_t guild_id = it->second;
    auto guild_it = guild_cache_.find(guild_id);
    if (guild_it == guild_cache_.end()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Guild not found: guild_id=" << guild_id;
        return false;
    }

    msg_guild::GuildInfo& guild = guild_it->second;
    guild.set_fund(guild.fund() + amount);

    auto member_it = member_cache_[guild_id].find(role_id);
    if (member_it != member_cache_[guild_id].end()) {
        msg_guild::GuildMemberInfo& member = member_it->second;
        member.set_contribution(member.contribution() + amount);
        member.set_total_contribution(member.total_contribution() + amount);
    }

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Contribution added: role_id=" << role_id << ", amount=" << amount;
    return true;
}

bool GuildModule::UpgradeGuild(uint64_t leader_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = role_guild_map_.find(leader_id);
    if (it == role_guild_map_.end()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Not in guild: role_id=" << leader_id;
        return false;
    }

    uint64_t guild_id = it->second;
    auto guild_it = guild_cache_.find(guild_id);
    if (guild_it == guild_cache_.end()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Guild not found: guild_id=" << guild_id;
        return false;
    }

    msg_guild::GuildInfo& guild = guild_it->second;
    if (guild.leader_id() != leader_id) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Not guild leader: role_id=" << leader_id;
        return false;
    }

    if (guild.level() >= MAX_GUILD_LEVEL) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Guild already at max level: level=" << guild.level();
        return false;
    }

    guild.set_level(guild.level() + 1);

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Guild upgraded: guild_id=" << guild_id << ", new_level=" << guild.level();
    return true;
}

bool GuildModule::UpdateAnnouncement(uint64_t leader_id, const std::string& announcement) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = role_guild_map_.find(leader_id);
    if (it == role_guild_map_.end()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Not in guild: role_id=" << leader_id;
        return false;
    }

    uint64_t guild_id = it->second;
    auto guild_it = guild_cache_.find(guild_id);
    if (guild_it == guild_cache_.end()) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Guild not found: guild_id=" << guild_id;
        return false;
    }

    msg_guild::GuildInfo& guild = guild_it->second;
    if (guild.leader_id() != leader_id) {
        ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Not guild leader: role_id=" << leader_id;
        return false;
    }

    guild.set_announcement(announcement);

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Announcement updated: guild_id=" << guild_id;
    return true;
}

bool GuildModule::SaveGuildData(uint64_t guild_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = guild_cache_.find(guild_id);
    if (it == guild_cache_.end()) {
        return false;
    }

    const msg_guild::GuildInfo& guild = it->second;

    std::string data = guild.SerializeAsString();

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Guild data saved: guild_id=" << guild_id;
    return true;
}

bool GuildModule::LoadGuildData(uint64_t guild_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    msg_guild::GuildInfo guild;
    guild.set_guild_id(guild_id);
    guild.set_guild_name("Test Guild");
    guild.set_leader_id(0);
    guild.set_leader_name("Test Leader");
    guild.set_level(1);
    guild.set_exp(0);
    guild.set_member_count(0);
    guild.set_max_member_count(50);
    guild.set_fund(0);
    guild.set_announcement("");
    guild.set_description("");
    guild.set_create_time(time(nullptr));

    guild_cache_[guild_id] = guild;

    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Guild data loaded: guild_id=" << guild_id;
    return true;
}

uint64_t GuildModule::GenerateGuildId() {
    static uint64_t guild_id_counter = 1000;
    return ++guild_id_counter;
}

} // namespace game_server
