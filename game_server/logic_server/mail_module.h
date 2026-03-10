#ifndef __MAIL_MODULE_H__
#define __MAIL_MODULE_H__

#include <unordered_map>
#include <vector>

namespace game_server {

class LogicService;

// 邮件状态
enum class MailStatus {
    UNREAD = 0,     // 未读
    READ = 1,       // 已读
    ATTACHMENT_NOT_TAKEN = 2,  // 附件未领取
    ATTACHMENT_TAKEN = 3,      // 附件已领取
    DELETED = 4     // 已删除
};

// 邮件类型
enum class MailType {
    SYSTEM = 1,     // 系统邮件
    PLAYER = 2,     // 玩家邮件
    GUILD = 3,      // 公会邮件
    ACTIVITY = 4    // 活动邮件
};

// 邮件附件
struct MailAttachment {
    int32_t attachment_type;
    int32_t item_id;
    int32_t item_count;
};

// 邮件信息
struct MailInfo {
    uint64_t mail_id;
    uint64_t sender_id;
    std::string sender_name;
    uint64_t receiver_id;
    std::string title;
    std::string content;
    MailType type;
    MailStatus status;
    std::vector<MailAttachment> attachments;
    time_t send_time;
    time_t expire_time;
    time_t read_time;
};

// 邮件模块类
class MailModule {
public:
    MailModule(LogicService* service);
    ~MailModule();
    
    // 邮件管理
    bool InitMail(uint64_t role_id);
    bool GetMails(uint64_t role_id, std::vector<MailInfo>& mails);
    bool GetMailInfo(uint64_t role_id, uint64_t mail_id, MailInfo& info);
    
    // 发送邮件
    bool SendMail(uint64_t sender_id, const std::string& sender_name, uint64_t receiver_id, 
                  const std::string& title, const std::string& content, MailType type);
    bool SendMailWithAttachments(uint64_t sender_id, const std::string& sender_name, uint64_t receiver_id,
                                 const std::string& title, const std::string& content, 
                                 MailType type, const std::vector<MailAttachment>& attachments);
    bool SendSystemMail(uint64_t receiver_id, const std::string& title, const std::string& content);
    bool SendSystemMailWithAttachments(uint64_t receiver_id, const std::string& title, 
                                       const std::string& content, 
                                       const std::vector<MailAttachment>& attachments);
    bool BroadcastSystemMail(const std::vector<uint64_t>& receiver_ids, const std::string& title, 
                             const std::string& content);
    
    // 读取邮件
    bool ReadMail(uint64_t role_id, uint64_t mail_id);
    bool GetUnreadMailCount(uint64_t role_id, int32_t& count);
    
    // 领取附件
    bool TakeAttachment(uint64_t role_id, uint64_t mail_id);
    bool TakeAllAttachments(uint64_t role_id);
    bool HasAttachment(uint64_t role_id, uint64_t mail_id);
    
    // 删除邮件
    bool DeleteMail(uint64_t role_id, uint64_t mail_id);
    bool DeleteReadMails(uint64_t role_id);
    bool DeleteExpiredMails(uint64_t role_id);
    
    // 邮件清理
    bool CleanExpiredMails(uint64_t role_id);
    bool CleanDeletedMails(uint64_t role_id);
    
    // 邮件提醒
    bool NotifyNewMail(uint64_t role_id);
    bool HasNewMail(uint64_t role_id);
    
    // 邮件数据加载和保存
    bool LoadMailData(uint64_t role_id);
    bool SaveMailData(uint64_t role_id);
    bool SaveSingleMail(uint64_t role_id, const MailInfo& mail);
    
    // 定时清理
    void OnTimer();
    
private:
    // 生成邮件ID
    uint64_t GenerateMailId();
    
    // 检查邮件是否过期
    bool IsMailExpired(const MailInfo& mail);
    
    // 邮件缓存
    std::unordered_map<uint64_t, std::vector<MailInfo>> mail_cache_;
    
    // 新邮件提醒缓存
    std::unordered_map<uint64_t, bool> new_mail_notify_;
    
    // 逻辑服务指针
    LogicService* service_;
    
    // 互斥锁
    std::mutex cache_mutex_;
    
    // 邮件最大数量
    static const int32_t MAX_MAIL_COUNT = 100;
    
    // 邮件过期时间（天）
    static const int32_t MAIL_EXPIRE_DAYS = 30;
    
    // 系统邮件发送者ID
    static const uint64_t SYSTEM_MAIL_SENDER_ID = 0;
};

} // namespace game_server

#endif // __MAIL_MODULE_H__
