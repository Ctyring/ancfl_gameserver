#ifndef __GUILD_MODULE_H__
#define __GUILD_MODULE_H__

#include "logic_service.h"
#include <unordered_map>
#include <vector>

namespace game_server {

// 公会职位
enum class GuildPosition {
    MEMBER = 0,     // 成员
    ELITE = 1,      // 精英
    ELDER = 2,      // 长老
    VICE_LEADER = 3, // 副会长
    LEADER = 4      // 会长
};

// 公会申请状态
enum class GuildApplyStatus {
    PENDING = 0,    // 待审核
    ACCEPTED = 1,   // 已通过
    REJECTED = 2    // 已拒绝
};

// 公会成员信息
struct GuildMember {
    uint64_t role_id;
    std::string role_name;
    int32_t level;
    int32_t profession;
    GuildPosition position;
    int32_t contribution;
    int32_t total_contribution;
    time_t join_time;
    time_t last_active_time;
    bool is_online;
};

// 公会申请信息
struct GuildApplyInfo {
    uint64_t apply_id;
    uint64_t role_id;
    std::string role_name;
    int32_t level;
    int32_t profession;
    GuildApplyStatus status;
    time_t apply_time;
    std::string message;
};

// 公会信息
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
    int32_t join_condition_level;
    bool need_approval;
    time_t create_time;
    bool is_active;
};

// 公会技能
struct GuildSkill {
    int32_t skill_id;
    int32_t level;
    int32_t max_level;
    bool is_unlocked;
};

// 公会模块类
class GuildModule {
public:
    GuildModule(LogicService* service);
    ~GuildModule();
    
    // 公会管理
    bool CreateGuild(uint64_t role_id, const std::string& guild_name, uint64_t& guild_id);
    bool DismissGuild(uint64_t role_id);
    bool GetGuildInfo(uint64_t guild_id, GuildInfo& info);
    bool GetGuildByName(const std::string& guild_name, GuildInfo& info);
    bool IsGuildNameExist(const std::string& guild_name);
    
    // 成员管理
    bool GetGuildMembers(uint64_t guild_id, std::vector<GuildMember>& members);
    bool GetMemberInfo(uint64_t guild_id, uint64_t role_id, GuildMember& member);
    bool GetMemberGuild(uint64_t role_id, uint64_t& guild_id);
    bool IsInGuild(uint64_t role_id);
    
    // 加入公会
    bool ApplyJoinGuild(uint64_t role_id, uint64_t guild_id, const std::string& message);
    bool HandleApply(uint64_t guild_id, uint64_t apply_id, bool accept);
    bool InviteJoinGuild(uint64_t inviter_id, uint64_t target_id);
    bool HandleInvite(uint64_t role_id, uint64_t guild_id, bool accept);
    bool DirectJoinGuild(uint64_t role_id, uint64_t guild_id);
    
    // 退出公会
    bool LeaveGuild(uint64_t role_id);
    bool KickMember(uint64_t operator_id, uint64_t target_id);
    
    // 职位管理
    bool SetMemberPosition(uint64_t operator_id, uint64_t target_id, GuildPosition position);
    bool CanSetPosition(uint64_t operator_id, uint64_t target_id, GuildPosition position);
    bool TransferLeader(uint64_t operator_id, uint64_t target_id);
    
    // 公会申请
    bool GetGuildApplies(uint64_t guild_id, std::vector<GuildApplyInfo>& applies);
    bool GetMyApplies(uint64_t role_id, std::vector<GuildApplyInfo>& applies);
    
    // 公会贡献
    bool AddContribution(uint64_t role_id, int32_t value);
    bool GetContribution(uint64_t role_id, int32_t& contribution);
    bool UseContribution(uint64_t role_id, int32_t value);
    
    // 公会资金
    bool AddGuildFund(uint64_t guild_id, int32_t value);
    bool UseGuildFund(uint64_t guild_id, int32_t value);
    bool GetGuildFund(uint64_t guild_id, int32_t& fund);
    
    // 公会等级
    bool AddGuildExp(uint64_t guild_id, int32_t exp);
    bool LevelUpGuild(uint64_t guild_id);
    bool GetGuildLevel(uint64_t guild_id, int32_t& level);
    
    // 公会公告
    bool SetAnnouncement(uint64_t role_id, const std::string& announcement);
    bool GetAnnouncement(uint64_t guild_id, std::string& announcement);
    
    // 公会设置
    bool SetJoinCondition(uint64_t role_id, int32_t level, bool need_approval);
    bool GetJoinCondition(uint64_t guild_id, int32_t& level, bool& need_approval);
    
    // 公会技能
    bool GetGuildSkills(uint64_t guild_id, std::vector<GuildSkill>& skills);
    bool UpgradeGuildSkill(uint64_t guild_id, int32_t skill_id);
    bool LearnGuildSkill(uint64_t role_id, int32_t skill_id);
    
    // 公会搜索
    bool SearchGuild(const std::string& name, std::vector<GuildInfo>& guilds);
    bool GetRecommendedGuilds(uint64_t role_id, std::vector<GuildInfo>& guilds);
    bool GetGuildList(std::vector<GuildInfo>& guilds);
    
    // 公会数据加载和保存
    bool LoadGuildData(uint64_t guild_id);
    bool SaveGuildData(uint64_t guild_id);
    bool LoadMemberGuildData(uint64_t role_id);
    bool SaveMemberGuildData(uint64_t role_id);
    
    // 定时处理
    void OnTimer();
    
private:
    // 生成公会ID
    uint64_t GenerateGuildId();
    
    // 生成申请ID
    uint64_t GenerateApplyId();
    
    // 检查权限
    bool HasPermission(uint64_t role_id, uint64_t guild_id, GuildPosition min_position);
    
    // 检查公会是否满员
    bool IsGuildFull(uint64_t guild_id);
    
    // 更新成员在线状态
    bool UpdateMemberOnlineStatus(uint64_t role_id, bool is_online);
    
    // 公会缓存
    std::unordered_map<uint64_t, GuildInfo> guild_cache_;
    std::unordered_map<uint64_t, std::vector<GuildMember>> guild_members_cache_;
    std::unordered_map<uint64_t, std::vector<GuildApplyInfo>> guild_applies_cache_;
    std::unordered_map<uint64_t, std::vector<GuildSkill>> guild_skills_cache_;
    
    // 玩家公会映射
    std::unordered_map<uint64_t, uint64_t> role_guild_map_;
    
    // 玩家申请缓存
    std::unordered_map<uint64_t, std::vector<GuildApplyInfo>> role_applies_cache_;
    
    // 逻辑服务指针
    LogicService* service_;
    
    // 互斥锁
    std::mutex cache_mutex_;
    
    // 公会最大成员数
    static const int32_t MAX_MEMBER_COUNT = 100;
    
    // 公会名称最大长度
    static const int32_t MAX_GUILD_NAME_LENGTH = 20;
};

} // namespace game_server

#endif // __GUILD_MODULE_H__
