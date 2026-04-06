#include <gtest/gtest.h>
#include "logic_server/mail_module.h"
#include "logic_server/logic_service.h"

using namespace game_server;

class MailModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        service_ = nullptr;
        mail_module_ = new MailModule(service_);
        test_role_id_ = 12345;
    }
    
    void TearDown() override {
        delete mail_module_;
    }
    
    LogicService* service_;
    MailModule* mail_module_;
    uint64_t test_role_id_;
};

TEST_F(MailModuleTest, InitMail) {
    EXPECT_TRUE(mail_module_->InitMail(test_role_id_));
}

TEST_F(MailModuleTest, SendMail) {
    mail_module_->InitMail(test_role_id_);
    
    EXPECT_TRUE(mail_module_->SendMail(0, "System", test_role_id_, "Test Mail", "This is a test mail", MailType::SYSTEM));
}

TEST_F(MailModuleTest, SendSystemMail) {
    mail_module_->InitMail(test_role_id_);
    
    EXPECT_TRUE(mail_module_->SendSystemMail(test_role_id_, "System Mail", "This is a system mail"));
}

TEST_F(MailModuleTest, GetMails) {
    mail_module_->InitMail(test_role_id_);
    mail_module_->SendSystemMail(test_role_id_, "Test Mail 1", "Content 1");
    mail_module_->SendSystemMail(test_role_id_, "Test Mail 2", "Content 2");
    
    std::vector<MailInfo> mails;
    EXPECT_TRUE(mail_module_->GetMails(test_role_id_, mails));
    EXPECT_EQ(mails.size(), 2);
}

TEST_F(MailModuleTest, GetMailInfo) {
    mail_module_->InitMail(test_role_id_);
    mail_module_->SendSystemMail(test_role_id_, "Test Mail", "Content");
    
    std::vector<MailInfo> mails;
    mail_module_->GetMails(test_role_id_, mails);
    if (!mails.empty()) {
        MailInfo info;
        EXPECT_TRUE(mail_module_->GetMailInfo(test_role_id_, mails[0].mail_id, info));
        EXPECT_EQ(info.title, "Test Mail");
    }
}

TEST_F(MailModuleTest, ReadMail) {
    mail_module_->InitMail(test_role_id_);
    mail_module_->SendSystemMail(test_role_id_, "Test Mail", "Content");
    
    std::vector<MailInfo> mails;
    mail_module_->GetMails(test_role_id_, mails);
    if (!mails.empty()) {
        EXPECT_TRUE(mail_module_->ReadMail(test_role_id_, mails[0].mail_id));
    }
}

TEST_F(MailModuleTest, GetUnreadMailCount) {
    mail_module_->InitMail(test_role_id_);
    mail_module_->SendSystemMail(test_role_id_, "Test Mail", "Content");
    
    int32_t count = 0;
    EXPECT_TRUE(mail_module_->GetUnreadMailCount(test_role_id_, count));
    EXPECT_EQ(count, 1);
}

TEST_F(MailModuleTest, DeleteMail) {
    mail_module_->InitMail(test_role_id_);
    mail_module_->SendSystemMail(test_role_id_, "Test Mail", "Content");
    
    std::vector<MailInfo> mails;
    mail_module_->GetMails(test_role_id_, mails);
    if (!mails.empty()) {
        EXPECT_TRUE(mail_module_->DeleteMail(test_role_id_, mails[0].mail_id));
    }
}

TEST_F(MailModuleTest, SendMailWithAttachments) {
    mail_module_->InitMail(test_role_id_);
    
    std::vector<MailAttachment> attachments;
    MailAttachment att;
    att.attachment_type = 1;
    att.item_id = 1001;
    att.item_count = 10;
    attachments.push_back(att);
    
    EXPECT_TRUE(mail_module_->SendMailWithAttachments(0, "System", test_role_id_, "Mail with attachments", "Content", MailType::SYSTEM, attachments));
}

TEST_F(MailModuleTest, TakeAttachment) {
    mail_module_->InitMail(test_role_id_);
    
    std::vector<MailAttachment> attachments;
    MailAttachment att;
    att.attachment_type = 1;
    att.item_id = 1001;
    att.item_count = 10;
    attachments.push_back(att);
    
    mail_module_->SendMailWithAttachments(0, "System", test_role_id_, "Mail with attachments", "Content", MailType::SYSTEM, attachments);
    
    std::vector<MailInfo> mails;
    mail_module_->GetMails(test_role_id_, mails);
    if (!mails.empty()) {
        EXPECT_TRUE(mail_module_->TakeAttachment(test_role_id_, mails[0].mail_id));
    }
}

TEST_F(MailModuleTest, BroadcastSystemMail) {
    std::vector<uint64_t> receiver_ids = {12345, 12346, 12347};
    
    for (auto role_id : receiver_ids) {
        mail_module_->InitMail(role_id);
    }
    
    EXPECT_TRUE(mail_module_->BroadcastSystemMail(receiver_ids, "Broadcast Mail", "This is a broadcast mail"));
}
