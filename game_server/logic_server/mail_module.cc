#include "mail_module.h"
#include "proto/msg_mail.pb.h"

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
    
    LOG_INFO("Mail initialized: role_id=%llu", role_id);
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
            LOG_ERROR("Mail box full: receiver_id=%llu", receiver_id);
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
    
    LOG_INFO("Mail sent: mail_id=%llu, sender_id=%llu, receiver_id=%llu, title=%s", 
             mail.mail_id, sender_id, receiver_id, title.c_str());
    
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
    
    LOG_INFO("System mail broadcast: receiver_count=%d, title=%s", receiver_ids.size(), title.c_str());
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
                LOG_INFO("Mail read: role_id=%llu, mail_id=%llu", role_id, mail_id);
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
                LOG_ERROR("Mail has no attachment or already taken: role_id=%llu, mail_id=%llu", role_id, mail_id);
                return false;
            }
            
            // 发放附件
            for (const auto& attachment : mail.attachments) {
                // TODO: 根据附件类型发放奖励
                switch (attachment.attachment_type) {
                case 1: // 金币
                    LOG_INFO("Attachment - Gold: role_id=%llu, gold=%d", role_id, attachment.item_count);
                    break;
                case 2: // 物品
                    LOG_INFO("Attachment - Item: role_id=%llu, item_id=%d, count=%d", 
                             role_id, attachment.item_id, attachment.item_count);
                    break;
                default:
                    break;
                }
            }
            
            // 更新邮件状态
            mail.status = MailStatus::ATTACHMENT_TAKEN;
            
            LOG_INFO("Attachment taken: role_id=%llu, mail_id=%llu", role_id, mail_id);
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
                    LOG_INFO("Attachment - Gold: role_id=%llu, gold=%d", role_id, attachment.item_count);
                    break;
                case 2: // 物品
                    LOG_INFO("Attachment - Item: role_id=%llu, item_id=%d, count=%d", 
                             role_id, attachment.item_id, attachment.item_count);
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
        LOG_INFO("All attachments taken: role_id=%llu", role_id);
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
                LOG_ERROR("Mail has unclaimed attachment: role_id=%llu, mail_id=%llu", role_id, mail_id);
                return false;
            }
            
            mail.status = MailStatus::DELETED;
            LOG_INFO("Mail deleted: role_id=%llu, mail_id=%llu", role_id, mail_id);
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
    
    LOG_INFO("Read mails deleted: role_id=%llu, count=%d", role_id, delete_count);
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
    
    LOG_INFO("Expired mails deleted: role_id=%llu, count=%d", role_id, delete_count);
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
    LOG_INFO("New mail notification: role_id=%llu", role_id);
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
    
    // 保存邮件数据到数据库
    msg_mail::MailDataSyncReq req;
    req.set_role_id(role_id);
    
    for (const auto& mail : it->second) {
        if (mail.status != MailStatus::DELETED) {
            auto mail_data = req.add_mails();
            mail_data->set_mail_id(mail.mail_id);
            mail_data->set_sender_id(mail.sender_id);
            mail_data->set_sender_name(mail.sender_name);
            mail_data->set_title(mail.title);
            mail_data->set_content(mail.content);
            mail_data->set_type(static_cast<int32_t>(mail.type));
            mail_data->set_status(static_cast<int32_t>(mail.status));
            mail_data->set_send_time(mail.send_time);
            mail_data->set_expire_time(mail.expire_time);
            mail_data->set_read_time(mail.read_time);
            
            // 保存附件
            for (const auto& attachment : mail.attachments) {
                auto attachment_data = mail_data->add_attachments();
                attachment_data->set_attachment_type(attachment.attachment_type);
                attachment_data->set_item_id(attachment.item_id);
                attachment_data->set_item_count(attachment.item_count);
            }
        }
    }
    
    service_->SendMsgToDBServer(static_cast<uint32_t>(MessageID::MSG_MAIL_DATA_SYNC_REQ), req);
    
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
                LOG_INFO("Mail auto expired: role_id=%llu, mail_id=%llu", role_id, mail.mail_id);
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
