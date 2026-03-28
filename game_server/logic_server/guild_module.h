#ifndef __GUILD_MODULE_H__
#define __GUILD_MODULE_H__

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <mutex>

namespace msg_guild {
    class GuildInfo;
    class GuildMemberInfo;
}

namespace game_server {

class LogicService;

enum class GuildPosition {
    LEADER = 1,
    VICE_LEADER = 2,
    ELDER = 3,
    MEMBER = 4
};

const int32_t MAX_GUILD_LEVEL = 10;
const int32_t MAX_GUILD_NAME_LENGTH = 32;

struct GuildInfo {
    uint64_t guild_id;
    std::string guild_name;
    uint64_t leader_id;
    std::string leader_name;
    int32_t level;
    int32_t exp;
    int32_t member_count;
    int32_t max_member_count;
    int32_t fund;
    std::string announcement;
    std::string description;
    int64_t create_time;
};

struct GuildMemberInfo {
    uint64_t role_id;
    std::string role_name;
    int32_t level;
    int32_t profession;
    GuildPosition position;
    int32_t contribution;
    int32_t total_contribution;
    int64_t join_time;
    int64_t last_login_time;
};

class GuildModule {
public:
    explicit GuildModule(LogicService* service);
    ~GuildModule();

    bool CreateGuild(uint64_t role_id, const std::string& guild_name, uint64_t& guild_id);
    bool JoinGuild(uint64_t role_id, uint64_t guild_id);
    bool LeaveGuild(uint64_t role_id);
    bool KickMember(uint64_t leader_id, uint64_t target_id);
    bool DissolveGuild(uint64_t leader_id);
    bool GetGuildInfo(uint64_t guild_id, GuildInfo& info);
    bool GetGuildMembers(uint64_t guild_id, std::vector<GuildMemberInfo>& members);
    bool GetGuildMemberInfo(uint64_t role_id, GuildMemberInfo& info);
    bool IsInGuild(uint64_t role_id);
    bool IsGuildNameExist(const std::string& guild_name);
    bool Contribute(uint64_t role_id, int32_t contribution_type, int32_t amount);
    bool UpgradeGuild(uint64_t leader_id);
    bool UpdateAnnouncement(uint64_t leader_id, const std::string& announcement);

    bool SaveGuildData(uint64_t guild_id = 0);
    bool LoadGuildData(uint64_t guild_id = 0);

private:
    uint64_t GenerateGuildId();

    LogicService* service_;

    std::mutex cache_mutex_;

    std::map<uint64_t, msg_guild::GuildInfo> guild_cache_;
    std::map<uint64_t, std::map<uint64_t, msg_guild::GuildMemberInfo>> member_cache_;
    std::map<uint64_t, uint64_t> role_guild_map_;
};

} // namespace game_server

#endif // __GUILD_MODULE_H__
