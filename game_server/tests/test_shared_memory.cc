#include <gtest/gtest.h>
#include "common/shared_memory.h"

using namespace game_server;

class TestShareObject : public ShareObject {
public:
    int32_t value;
    char name[32];
    
    TestShareObject() : value(0) {
        memset(name, 0, sizeof(name));
    }
};

class SharedMemoryTest : public ::testing::Test {
protected:
    void SetUp() override {
    }
    
    void TearDown() override {
    }
};

TEST_F(SharedMemoryTest, ShareObjectStatus) {
    TestShareObject obj;
    
    EXPECT_EQ(obj.GetCheckCode(), 0x5A);
    EXPECT_EQ(obj.GetStatus(), SharedMemoryStatus::USE);
}

TEST_F(SharedMemoryTest, ShareObjectLock) {
    TestShareObject obj;
    
    obj.Lock();
    EXPECT_TRUE(obj.IsLock());
    EXPECT_EQ(obj.GetStatus(), SharedMemoryStatus::LOCK);
    
    obj.Unlock();
    EXPECT_FALSE(obj.IsLock());
    EXPECT_EQ(obj.GetStatus(), SharedMemoryStatus::USE);
}

TEST_F(SharedMemoryTest, ShareObjectRelease) {
    TestShareObject obj;
    
    obj.Release();
    EXPECT_TRUE(obj.IsRelease());
    EXPECT_EQ(obj.GetStatus(), SharedMemoryStatus::RELEASE);
}

TEST_F(SharedMemoryTest, ShareObjectDestroy) {
    TestShareObject obj;
    
    obj.Destroy();
    EXPECT_TRUE(obj.IsDestroy());
    EXPECT_EQ(obj.GetStatus(), SharedMemoryStatus::DELETE);
}

TEST_F(SharedMemoryTest, SharedMemoryAllocate) {
    SharedMemory<TestShareObject> sm(1, 10);
    
    TestShareObject* obj = sm.Allocate();
    EXPECT_NE(obj, nullptr);
    
    sm.Free(obj);
}

TEST_F(SharedMemoryTest, SharedMemoryBlockCount) {
    SharedMemory<TestShareObject> sm(2, 10);
    
    EXPECT_EQ(sm.GetBlockCount(), 0);
    
    sm.Allocate();
    EXPECT_EQ(sm.GetUsedBlockCount(), 1);
}
