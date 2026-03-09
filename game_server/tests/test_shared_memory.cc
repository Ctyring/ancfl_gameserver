#include <gtest/gtest.h>
#include "shared_memory.h"

using namespace game_server;

class SharedMemoryTest : public testing::Test {
protected:
    void SetUp() override {
        shm_name_ = "/test_shm_" + std::to_string(getpid());
        shm_size_ = 4096;
    }
    
    void TearDown() override {
    }
    
    std::string shm_name_;
    size_t shm_size_;
};

TEST_F(SharedMemoryTest, CreateAndOpen) {
    SharedMemory shm;
    EXPECT_TRUE(shm.Create(shm_name_, shm_size_));
    EXPECT_TRUE(shm.IsOpen());
    EXPECT_EQ(shm.GetSize(), shm_size_);
    shm.Close();
}

TEST_F(SharedMemoryTest, OpenExisting) {
    SharedMemory shm1;
    EXPECT_TRUE(shm1.Create(shm_name_, shm_size_));
    
    SharedMemory shm2;
    EXPECT_TRUE(shm2.Open(shm_name_));
    EXPECT_TRUE(shm2.IsOpen());
    
    shm1.Close();
    shm2.Close();
}

TEST_F(SharedMemoryTest, ReadWrite) {
    SharedMemory shm;
    EXPECT_TRUE(shm.Create(shm_name_, shm_size_));
    
    const char* write_data = "Hello, Shared Memory!";
    size_t write_len = strlen(write_data);
    
    EXPECT_TRUE(shm.Write(0, write_data, write_len));
    
    char read_data[256] = {0};
    EXPECT_TRUE(shm.Read(0, read_data, write_len));
    
    EXPECT_STREQ(read_data, write_data);
    
    shm.Close();
}

TEST_F(SharedMemoryTest, WriteAtOffset) {
    SharedMemory shm;
    EXPECT_TRUE(shm.Create(shm_name_, shm_size_));
    
    const char* data1 = "AAAA";
    const char* data2 = "BBBB";
    
    EXPECT_TRUE(shm.Write(0, data1, 4));
    EXPECT_TRUE(shm.Write(100, data2, 4));
    
    char read1[5] = {0};
    char read2[5] = {0};
    
    EXPECT_TRUE(shm.Read(0, read1, 4));
    EXPECT_TRUE(shm.Read(100, read2, 4));
    
    EXPECT_STREQ(read1, data1);
    EXPECT_STREQ(read2, data2);
    
    shm.Close();
}

TEST_F(SharedMemoryTest, WriteBeyondSize) {
    SharedMemory shm;
    EXPECT_TRUE(shm.Create(shm_name_, 100));
    
    char data[200] = {0};
    EXPECT_FALSE(shm.Write(50, data, sizeof(data)));
    
    shm.Close();
}

TEST_F(SharedMemoryTest, Clear) {
    SharedMemory shm;
    EXPECT_TRUE(shm.Create(shm_name_, shm_size_));
    
    const char* data = "TestData";
    shm.Write(0, data, strlen(data));
    
    EXPECT_TRUE(shm.Clear());
    
    char read[64] = {0};
    shm.Read(0, read, strlen(data));
    
    for (size_t i = 0; i < strlen(data); ++i) {
        EXPECT_EQ(read[i], 0);
    }
    
    shm.Close();
}

TEST_F(SharedMemoryTest, GetPointer) {
    SharedMemory shm;
    EXPECT_TRUE(shm.Create(shm_name_, shm_size_));
    
    void* ptr = shm.GetPointer();
    EXPECT_NE(ptr, nullptr);
    
    shm.Close();
}

TEST_F(SharedMemoryTest, DoubleCreate) {
    SharedMemory shm1;
    EXPECT_TRUE(shm1.Create(shm_name_, shm_size_));
    
    SharedMemory shm2;
    EXPECT_FALSE(shm2.Create(shm_name_, shm_size_));
    
    shm1.Close();
}

TEST_F(SharedMemoryTest, OpenNonExisting) {
    SharedMemory shm;
    EXPECT_FALSE(shm.Open("/non_existing_shm"));
    EXPECT_FALSE(shm.IsOpen());
}

TEST_F(SharedMemoryTest, Reopen) {
    SharedMemory shm;
    EXPECT_TRUE(shm.Create(shm_name_, shm_size_));
    shm.Close();
    
    EXPECT_FALSE(shm.IsOpen());
    
    EXPECT_TRUE(shm.Open(shm_name_));
    EXPECT_TRUE(shm.IsOpen());
    
    shm.Close();
}
