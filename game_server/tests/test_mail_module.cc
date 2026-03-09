#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "logic_server/mail_module.h"

using namespace game_server;
using namespace testing;

class MockLogicServiceForMail : public LogicService {
public:
    MOCK_METHOD(bool, SendToClient, (uint64_t role_id, int32_t msg_id, const std::string& data), (override));
    MOCK_METHOD(bool, SendToDB, (int32_t msg_id, const std::string& data), (override));
};

class MailModuleTest : public Test {
protected:
    void SetUp() override {
        mock_service_ = new MockLogicServiceForMail();
        mail_module_ = new MailModule(mock_service_);
        test_role_id_ = 12345;
    }
    
    void TearDown() override {
        delete mail_module_;
        delete mock_service_;
    }
    
    MockLogicServiceForMail* mock_service_;
    MailModule* mail_module_;
    uint64_t test_role_id_;
};

TEST_F(MailModuleTest, InitMail) {
    EXPECT_TRUE(mail_module_->InitMail(test_role_id_));
}

TEST_F(MailModuleTest, SendMail) {
    mail_module_->InitMail(test_role_id_);
    
    MailInfo mail;
    mail.mail_id = 0;
    mail.sender_id = 0;
    mail.sender_name = "System";
    mail.title = "Test Mail";
    mail.content = "This is a test mail";
    
    EXPECT_TRUE(mail_module_->SendMail(test_role_id_, mail));
    EXPECT_NE(mail.mail_id, 0);
}

TEST_F(MailModuleTest, GetMailList) {
    mail_module_->InitMail(test_role_id_);
    
    MailInfo mail;
    mail.title = "Test Mail 1";
    mail_module_->SendMail(test_role_id_, mail);
    mail.title = "Test Mail 2";
    mail_module_->SendMail(test_role_id_, mail);
    
    std::vector<MailInfo> mails;
    EXPECT_TRUE(mail_module_->GetMailList(test_role_id_, mails));
    EXPECT_EQ(mails.size(), 2);
}

TEST_F(MailModuleTest, ReadMail) {
    mail_module_->InitMail(test_role_id_);
    
    MailInfo mail;
    mail.title = "Test Mail";
    mail_module_->SendMail(test_role_id_, mail);
    
    EXPECT_TRUE(mail_module_->ReadMail(test_role_id_, mail.mail_id));
    
    MailInfo read_mail;
    mail_module_->GetMailInfo(test_role_id_, mail.mail_id, read_mail);
    EXPECT_TRUE(read_mail.is_read);
}

TEST_F(MailModuleTest, DeleteMail) {
    mail_module_->InitMail(test_role_id_);
    
    MailInfo mail;
    mail.title = "Test Mail";
    mail_module_->SendMail(test_role_id_, mail);
    
    EXPECT_TRUE(mail_module_->DeleteMail(test_role_id_, mail.mail_id));
    
    std::vector<MailInfo> mails;
    mail_module_->GetMailList(test_role_id_, mails);
    EXPECT_EQ(mails.size(), 0);
}

TEST_F(MailModuleTest, TakeAttachment) {
    mail_module_->InitMail(test_role_id_);
    
    MailInfo mail;
    mail.title = "Test Mail";
    MailAttachment attachment;
    attachment.item_config_id = 1001;
    attachment.item_count = 10;
    mail.attachments.push_back(attachment);
    mail_module_->SendMail(test_role_id_, mail);
    
    EXPECT_TRUE(mail_module_->TakeAttachment(test_role_id_, mail.mail_id, 0));
}

TEST_F(MailModuleTest, TakeAllAttachments) {
    mail_module_->InitMail(test_role_id_);
    
    MailInfo mail;
    mail.title = "Test Mail";
    MailAttachment attachment1;
    attachment1.item_config_id = 1001;
    attachment1.item_count = 10;
    mail.attachments.push_back(attachment1);
    
    MailAttachment attachment2;
    attachment2.item_config_id = 1002;
    attachment2.item_count = 20;
    mail.attachments.push_back(attachment2);
    
    mail_module_->SendMail(test_role_id_, mail);
    
    EXPECT_TRUE(mail_module_->TakeAllAttachments(test_role_id_, mail.mail_id));
}

TEST_F(MailModuleTest, GetUnreadCount) {
    mail_module_->InitMail(test_role_id_);
    
    MailInfo mail;
    mail.title = "Test Mail";
    mail_module_->SendMail(test_role_id_, mail);
    
    int32_t count = 0;
    EXPECT_TRUE(mail_module_->GetUnreadCount(test_role_id_, count));
    EXPECT_EQ(count, 1);
}

TEST_F(MailModuleTest, SendSystemMail) {
    mail_module_->InitMail(test_role_id_);
    
    std::vector<uint64_t> role_ids = {test_role_id_};
    EXPECT_TRUE(mail_module_->SendSystemMail(role_ids, "System Mail", "This is a system mail"));
}

TEST_F(MailModuleTest, DeleteReadMails) {
    mail_module_->InitMail(test_role_id_);
    
    MailInfo mail1;
    mail1.title = "Test Mail 1";
    mail_module_->SendMail(test_role_id_, mail1);
    mail_module_->ReadMail(test_role_id_, mail1.mail_id);
    
    MailInfo mail2;
    mail2.title = "Test Mail 2";
    mail_module_->SendMail(test_role_id_, mail2);
    
    EXPECT_TRUE(mail_module_->DeleteReadMails(test_role_id_));
    
    std::vector<MailInfo> mails;
    mail_module_->GetMailList(test_role_id_, mails);
    EXPECT_EQ(mails.size(), 1);
}
