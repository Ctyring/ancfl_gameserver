#include "mail_module.h"
#include "ancfl/log.h"
#include <mutex>

namespace game_server {

MailModule::MailModule(LogicService* service) : service_(service) {
}

MailModule::~MailModule() {
}

bool MailModule::InitMail(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    // 检查邮件是否已初始化
    auto it = mail_cache_.find(role_id);
    if (it != mail_cache_.end()) {
        return true;
    }
    
    // 初始化邮件列表
    mail_cache_[role_id] = std::vector<MailInfo>();
    new_mail_notify_[role_id] = false;
    
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Mail initialized: role_id=" << role_id;
    return true;
}

bool MailModule::GetMails(uint64_t role_id, std::vector<MailInfo>& mails) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = mail_cache_.find(role_id);
    if (it == mail_cache_.end()) {
        return false;
    }
    
    mails.clear();
    for (const auto& mail : it->second) {
        if (mail.status != MailStatus::DELETED) {
            mails.push_back(mail);
        }
    }
    
    return true;
}

bool MailModule::GetMailInfo(uint64_t role_id, uint64_t mail_id, MailInfo& info) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = mail_cache_.find(role_id);
    if (it == mail_cache_.end()) {
        return false;
    }
    
    for (const auto& mail : it->second) {
        if (mail.mail_id == mail_id && mail.status != MailStatus::DELETED) {
            info = mail;
            return true;
        }
    }
    
    return false;
}

bool MailModule::SendMail(uint64_t sender_id, const std::string& sender_name, uint64_t receiver_id,
                          const std::string& title, const std::string& content, MailType type) {
    std::vector<MailAttachment> empty_attachments;
    return SendMailWithAttachments(sender_id, sender_name, receiver_id, title, content, type, empty_attachments);
}

bool MailModule::SendMailWithAttachments(uint64_t sender_id, const std::string& sender_name, uint64_t receiver_id,
                                         const std::string& title, const std::string& content,
                                         MailType type, const std::vector<MailAttachment>& attachments) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = mail_cache_.find(receiver_id);
    if (it == mail_cache_.end()) {
        mail_cache_[receiver_id] = std::vector<MailInfo>();
        it = mail_cache_.find(receiver_id);
    }
    
    // 检查邮件数量是否超过上限
    if (it->second.size() >= MAX_MAIL_COUNT) {
        // 删除最旧的已读邮件
        auto oldest_it = it->second.begin();
        while (oldest_it != it->second.end()) {
            if (oldest_it->status == MailStatus::READ || oldest_it->status == MailStatus::ATTACHMENT_TAKEN) {
                it->second.erase(oldest_it);
                break;
            }
            ++oldest_it;
        }
        
        // 如果还是超过上限，返回失败
        if (it->second.size() >= MAX_MAIL_COUNT) {
            ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Mail box full: receiver_id=" << receiver_id;
            return false;
        }
    }
    
    // 创建邮件
    MailInfo mail;
    mail.mail_id = GenerateMailId();
    mail.sender_id = sender_id;
    mail.sender_name = sender_name;
    mail.receiver_id = receiver_id;
    mail.title = title;
    mail.content = content;
    mail.type = type;
    mail.status = MailStatus::UNREAD;
    mail.attachments = attachments;
    mail.send_time = time(nullptr);
    mail.expire_time = mail.send_time + MAIL_EXPIRE_DAYS * 86400;
    mail.read_time = 0;
    
    // 如果有附件，设置状态为附件未领取
    if (!attachments.empty()) {
        mail.status = MailStatus::ATTACHMENT_NOT_TAKEN;
    }
    
    it->second.push_back(mail);
    
    // 标记有新邮件
    new_mail_notify_[receiver_id] = true;
    
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Mail sent: mail_id=" << mail.mail_id << ", sender_id=" << sender_id << ", receiver_id=" << receiver_id << ", title=" << title;
    
    // 通知收件人有新邮件
    NotifyNewMail(receiver_id);
    
    return true;
}

bool MailModule::SendSystemMail(uint64_t receiver_id, const std::string& title, const std::string& content) {
    return SendMail(SYSTEM_MAIL_SENDER_ID, "System", receiver_id, title, content, MailType::SYSTEM);
}

bool MailModule::SendSystemMailWithAttachments(uint64_t receiver_id, const std::string& title, 
                                               const std::string& content,
                                               const std::vector<MailAttachment>& attachments) {
    return SendMailWithAttachments(SYSTEM_MAIL_SENDER_ID, "System", receiver_id, title, content, 
                                   MailType::SYSTEM, attachments);
}

bool MailModule::BroadcastSystemMail(const std::vector<uint64_t>& receiver_ids, const std::string& title, 
                                     const std::string& content) {
    for (uint64_t receiver_id : receiver_ids) {
        SendSystemMail(receiver_id, title, content);
    }
    
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "System mail broadcast: receiver_count=" << receiver_ids.size() << ", title=" << title;
    return true;
}

bool MailModule::ReadMail(uint64_t role_id, uint64_t mail_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = mail_cache_.find(role_id);
    if (it == mail_cache_.end()) {
        return false;
    }
    
    for (auto& mail : it->second) {
        if (mail.mail_id == mail_id && mail.status != MailStatus::DELETED) {
            if (mail.status == MailStatus::UNREAD) {
                mail.status = MailStatus::READ;
                mail.read_time = time(nullptr);
                ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Mail read: role_id=" << role_id << ", mail_id=" << mail_id;
            }
            return true;
        }
    }
    
    return false;
}

bool MailModule::GetUnreadMailCount(uint64_t role_id, int32_t& count) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = mail_cache_.find(role_id);
    if (it == mail_cache_.end()) {
        count = 0;
        return true;
    }
    
    count = 0;
    for (const auto& mail : it->second) {
        if (mail.status == MailStatus::UNREAD || mail.status == MailStatus::ATTACHMENT_NOT_TAKEN) {
            count++;
        }
    }
    
    return true;
}

bool MailModule::TakeAttachment(uint64_t role_id, uint64_t mail_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = mail_cache_.find(role_id);
    if (it == mail_cache_.end()) {
        return false;
    }
    
    for (auto& mail : it->second) {
        if (mail.mail_id == mail_id && mail.status != MailStatus::DELETED) {
            if (mail.status != MailStatus::ATTACHMENT_NOT_TAKEN) {
                ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Mail has no attachment or already taken: role_id=" << role_id << ", mail_id=" << mail_id;
                return false;
            }
            
            // 发放附件
            for (const auto& attachment : mail.attachments) {
                // TODO: 根据附件类型发放奖励
                switch (attachment.attachment_type) {
                case 1: // 金币
                    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Attachment - Gold: role_id=" << role_id << ", gold=" << attachment.item_count;
                    break;
                case 2: // 物品
                    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Attachment - Item: role_id=" << role_id << ", item_id=" << attachment.item_id << ", count=" << attachment.item_count;
                    break;
                default:
                    break;
                }
            }
            
            // 更新邮件状态
            mail.status = MailStatus::ATTACHMENT_TAKEN;
            
            ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Attachment taken: role_id=" << role_id << ", mail_id=" << mail_id;
            return true;
        }
    }
    
    return false;
}

bool MailModule::TakeAllAttachments(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = mail_cache_.find(role_id);
    if (it == mail_cache_.end()) {
        return false;
    }
    
    bool has_attachment = false;
    
    for (auto& mail : it->second) {
        if (mail.status == MailStatus::ATTACHMENT_NOT_TAKEN) {
            // 发放附件
            for (const auto& attachment : mail.attachments) {
                // TODO: 根据附件类型发放奖励
                switch (attachment.attachment_type) {
                case 1: // 金币
                    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Attachment - Gold: role_id=" << role_id << ", gold=" << attachment.item_count;
                    break;
                case 2: // 物品
                    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Attachment - Item: role_id=" << role_id << ", item_id=" << attachment.item_id << ", count=" << attachment.item_count;
                    break;
                default:
                    break;
                }
            }
            
            // 更新邮件状态
            mail.status = MailStatus::ATTACHMENT_TAKEN;
            has_attachment = true;
        }
    }
    
    if (has_attachment) {
        ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "All attachments taken: role_id=" << role_id;
    }
    
    return true;
}

bool MailModule::HasAttachment(uint64_t role_id, uint64_t mail_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = mail_cache_.find(role_id);
    if (it == mail_cache_.end()) {
        return false;
    }
    
    for (const auto& mail : it->second) {
        if (mail.mail_id == mail_id && mail.status == MailStatus::ATTACHMENT_NOT_TAKEN) {
            return true;
        }
    }
    
    return false;
}

bool MailModule::DeleteMail(uint64_t role_id, uint64_t mail_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = mail_cache_.find(role_id);
    if (it == mail_cache_.end()) {
        return false;
    }
    
    for (auto& mail : it->second) {
        if (mail.mail_id == mail_id && mail.status != MailStatus::DELETED) {
            // 检查是否有未领取的附件
            if (mail.status == MailStatus::ATTACHMENT_NOT_TAKEN) {
                ANCFL_LOG_ERROR(ANCFL_LOG_ROOT()) << "Mail has unclaimed attachment: role_id=" << role_id << ", mail_id=" << mail_id;
                return false;
            }
            
            mail.status = MailStatus::DELETED;
            ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Mail deleted: role_id=" << role_id << ", mail_id=" << mail_id;
            return true;
        }
    }
    
    return false;
}

bool MailModule::DeleteReadMails(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = mail_cache_.find(role_id);
    if (it == mail_cache_.end()) {
        return false;
    }
    
    int32_t delete_count = 0;
    
    for (auto& mail : it->second) {
        if (mail.status == MailStatus::READ || mail.status == MailStatus::ATTACHMENT_TAKEN) {
            mail.status = MailStatus::DELETED;
            delete_count++;
        }
    }
    
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Read mails deleted: role_id=" << role_id << ", count=" << delete_count;
    return true;
}

bool MailModule::DeleteExpiredMails(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = mail_cache_.find(role_id);
    if (it == mail_cache_.end()) {
        return false;
    }
    
    time_t now = time(nullptr);
    int32_t delete_count = 0;
    
    for (auto& mail : it->second) {
        if (mail.status != MailStatus::DELETED && now > mail.expire_time) {
            mail.status = MailStatus::DELETED;
            delete_count++;
        }
    }
    
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Expired mails deleted: role_id=" << role_id << ", count=" << delete_count;
    return true;
}

bool MailModule::CleanExpiredMails(uint64_t role_id) {
    return DeleteExpiredMails(role_id);
}

bool MailModule::CleanDeletedMails(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = mail_cache_.find(role_id);
    if (it == mail_cache_.end()) {
        return false;
    }
    
    // 删除标记为删除的邮件
    auto mail_it = it->second.begin();
    while (mail_it != it->second.end()) {
        if (mail_it->status == MailStatus::DELETED) {
            mail_it = it->second.erase(mail_it);
        } else {
            ++mail_it;
        }
    }
    
    return true;
}

bool MailModule::NotifyNewMail(uint64_t role_id) {
    // TODO: 发送新邮件通知给客户端
    ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "New mail notification: role_id=" << role_id;
    return true;
}

bool MailModule::HasNewMail(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = new_mail_notify_.find(role_id);
    if (it == new_mail_notify_.end()) {
        return false;
    }
    
    bool has_new = it->second;
    it->second = false; // 重置标记
    
    return has_new;
}

bool MailModule::LoadMailData(uint64_t role_id) {
    // 从数据库加载邮件数据
    // TODO: 实现从数据库加载邮件数据
    
    // 初始化邮件
    InitMail(role_id);
    
    return true;
}

bool MailModule::SaveMailData(uint64_t role_id) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    auto it = mail_cache_.find(role_id);
    if (it == mail_cache_.end()) {
        return false;
    }
    
    // TODO: 由于 proto/msg_mail.pb.h 不存在，暂时返回 true
    return true;
}

bool MailModule::SaveSingleMail(uint64_t role_id, const MailInfo& mail) {
    // TODO: 保存单封邮件到数据库
    return true;
}

void MailModule::OnTimer() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    time_t now = time(nullptr);
    
    // 清理所有过期邮件
    for (auto& pair : mail_cache_) {
        uint64_t role_id = pair.first;
        auto& mails = pair.second;
        
        for (auto& mail : mails) {
            if (mail.status != MailStatus::DELETED && now > mail.expire_time) {
                mail.status = MailStatus::DELETED;
                ANCFL_LOG_INFO(ANCFL_LOG_ROOT()) << "Mail auto expired: role_id=" << role_id << ", mail_id=" << mail.mail_id;
            }
        }
    }
}

uint64_t MailModule::GenerateMailId() {
    static uint64_t next_id = time(nullptr) * 10000 + rand() % 10000;
    return next_id++;
}

bool MailModule::IsMailExpired(const MailInfo& mail) {
    time_t now = time(nullptr);
    return now > mail.expire_time;
}

} // namespace game_server