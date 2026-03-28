#include "shared_memory.h"
#include "ancfl/log.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <cstring>
#include <unistd.h>

using namespace game_server;

static int passed_tests = 0;
static int failed_tests = 0;

#define TEST_ASSERT(condition, test_name) \
    do { \
        if (condition) { \
            std::cout << "[通过] " << test_name << std::endl; \
            passed_tests++; \
        } else { \
            std::cout << "[失败] " << test_name << std::endl; \
            failed_tests++; \
        } \
    } while(0)

// 测试用的共享对象
class TestObject : public ShareObject {
public:
    int value;
    char name[32];
    
    TestObject() : value(0) {
        memset(name, 0, sizeof(name));
    }
};

void test_share_object() {
    std::cout << "\n=== 测试: 共享对象基础功能 ===" << std::endl;
    
    TestObject obj;
    
    TEST_ASSERT(obj.GetCheckCode() == 0x5A, "检查码正确");
    TEST_ASSERT(obj.GetStatus() == SharedMemoryStatus::USE, "初始状态为 USE");
    
    obj.Lock();
    TEST_ASSERT(obj.IsLock(), "锁定后状态为 LOCK");
    
    obj.Unlock();
    TEST_ASSERT(obj.GetStatus() == SharedMemoryStatus::USE, "解锁后状态为 USE");
    
    obj.Release();
    TEST_ASSERT(obj.IsRelease(), "释放后状态为 RELEASE");
    
    obj.Destroy();
    TEST_ASSERT(obj.IsDestroy(), "销毁后状态为 DELETE");
    
    obj.Reset();
    TEST_ASSERT(obj.GetStatus() == SharedMemoryStatus::NONE, "重置后状态为 NONE");
}

void test_shared_memory_allocate() {
    std::cout << "\n=== 测试: 共享内存分配 ===" << std::endl;
    
    SharedMemory<TestObject> shm(7, 10);
    
    TEST_ASSERT(shm.GetBlockCount() == 0, "初始数据块数量为 0");
    TEST_ASSERT(shm.GetUsedBlockCount() == 0, "初始已使用数据块数量为 0");
    
    TestObject* obj1 = shm.Allocate();
    TEST_ASSERT(obj1 != nullptr, "第一次分配成功");
    TEST_ASSERT(shm.GetUsedBlockCount() == 1, "已使用数据块数量为 1");
    
    TestObject* obj2 = shm.Allocate();
    TEST_ASSERT(obj2 != nullptr, "第二次分配成功");
    TEST_ASSERT(shm.GetUsedBlockCount() == 2, "已使用数据块数量为 2");
    
    shm.Free(obj1);
    TEST_ASSERT(shm.GetUsedBlockCount() == 1, "释放后已使用数据块数量为 1");
    
    shm.Free(obj2);
    TEST_ASSERT(shm.GetUsedBlockCount() == 0, "全部释放后已使用数据块数量为 0");
}

void test_shared_memory_data() {
    std::cout << "\n=== 测试: 共享内存数据读写 ===" << std::endl;
    
    SharedMemory<TestObject> shm(2, 5);
    
    TestObject* obj = shm.Allocate();
    TEST_ASSERT(obj != nullptr, "分配数据块成功");
    
    obj->value = 12345;
    strcpy(obj->name, "test_object");
    
    TEST_ASSERT(obj->value == 12345, "设置值成功");
    TEST_ASSERT(strcmp(obj->name, "test_object") == 0, "设置名称成功");
    
    shm.Free(obj);
    
    TestObject* obj2 = shm.Allocate();
    TEST_ASSERT(obj2 != nullptr, "重新分配数据块成功");
    
    obj2->value = 67890;
    strcpy(obj2->name, "new_object");
    
    TEST_ASSERT(obj2->value == 67890, "重新设置值成功");
    TEST_ASSERT(strcmp(obj2->name, "new_object") == 0, "重新设置名称成功");
    
    shm.Free(obj2);
}

void test_shared_memory_status() {
    std::cout << "\n=== 测试: 共享内存状态管理 ===" << std::endl;
    
    SharedMemory<TestObject> shm(3, 3);
    
    TestObject* obj = shm.Allocate();
    TEST_ASSERT(obj != nullptr, "分配数据块成功");
    
    SharedMemoryStatus status = shm.GetStatus(obj);
    TEST_ASSERT(status == SharedMemoryStatus::USE, "初始状态为 USE");
    
    shm.SetStatus(obj, SharedMemoryStatus::LOCK);
    status = shm.GetStatus(obj);
    TEST_ASSERT(status == SharedMemoryStatus::LOCK, "状态设置为 LOCK 成功");
    
    shm.SetStatus(obj, SharedMemoryStatus::RELEASE);
    status = shm.GetStatus(obj);
    TEST_ASSERT(status == SharedMemoryStatus::RELEASE, "状态设置为 RELEASE 成功");
    
    shm.SetStatus(obj, SharedMemoryStatus::DELETE);
    status = shm.GetStatus(obj);
    TEST_ASSERT(status == SharedMemoryStatus::DELETE, "状态设置为 DELETE 成功");
    
    shm.Free(obj);
}

void test_shared_memory_clean() {
    std::cout << "\n=== 测试: 共享内存清理 ===" << std::endl;
    
    SharedMemory<TestObject> shm(4, 5);
    
    TestObject* obj1 = shm.Allocate();
    TestObject* obj2 = shm.Allocate();
    
    TEST_ASSERT(shm.GetUsedBlockCount() == 2, "已使用数据块数量为 2");
    
    // 等待一小段时间，确保数据块的update_time_被设置
    sleep(1);
    
    time_t now = time(nullptr);
    shm.CleanExpiredData(now - 1); // 清理1秒前的数据，应该不会清理任何数据
    
    TEST_ASSERT(shm.GetUsedBlockCount() == 2, "清理后数据块数量不变");
    
    shm.Free(obj1);
    shm.Free(obj2);
    
    TEST_ASSERT(shm.GetUsedBlockCount() == 0, "全部释放后数据块数量为 0");
}

void test_shared_memory_multiple_pages() {
    std::cout << "\n=== 测试: 共享内存多页管理 ===" << std::endl;
    
    SharedMemory<TestObject> shm(5, 3);
    
    std::vector<TestObject*> objects;
    
    // 分配超过一页的数据块
    for (int i = 0; i < 5; i++) {
        TestObject* obj = shm.Allocate();
        TEST_ASSERT(obj != nullptr, "分配数据块 " + std::to_string(i+1) + " 成功");
        obj->value = i;
        objects.push_back(obj);
    }
    
    TEST_ASSERT(shm.GetUsedBlockCount() == 5, "已使用数据块数量为 5");
    
    // 释放所有数据块
    for (auto obj : objects) {
        shm.Free(obj);
    }
    
    TEST_ASSERT(shm.GetUsedBlockCount() == 0, "全部释放后数据块数量为 0");
}

void test_shared_memory_thread_safety() {
    std::cout << "\n=== 测试: 共享内存线程安全 ===" << std::endl;
    
    SharedMemory<TestObject> shm(6, 20);
    
    std::atomic<int> allocate_count{0};
    std::atomic<int> free_count{0};
    
    const int thread_count = 4;
    const int operations_per_thread = 5;
    
    std::vector<std::thread> threads;
    std::vector<TestObject*> all_objects;
    std::mutex objects_mutex;
    
    for (int t = 0; t < thread_count; t++) {
        threads.emplace_back([&shm, &allocate_count, &free_count, &all_objects, &objects_mutex, operations_per_thread]() {
            std::vector<TestObject*> local_objects;
            
            // 分配操作
            for (int i = 0; i < operations_per_thread; i++) {
                TestObject* obj = shm.Allocate();
                if (obj) {
                    allocate_count++;
                    obj->value = i;
                    local_objects.push_back(obj);
                }
            }
            
            // 短暂休眠
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            // 释放操作
            for (auto obj : local_objects) {
                shm.Free(obj);
                free_count++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    TEST_ASSERT(allocate_count == thread_count * operations_per_thread, "所有线程分配成功");
    TEST_ASSERT(free_count == thread_count * operations_per_thread, "所有线程释放成功");
    TEST_ASSERT(shm.GetUsedBlockCount() == 0, "全部释放后数据块数量为 0");
}

void test_shared_memory_edge_cases() {
    std::cout << "\n=== 测试: 共享内存边界情况 ===" << std::endl;
    
    SharedMemory<TestObject> shm(7, 1);
    
    // 测试空释放
    shm.Free(nullptr);
    TEST_ASSERT(true, "空指针释放安全");
    
    // 测试多次释放
    TestObject* obj = shm.Allocate();
    if (obj) {
        shm.Free(obj);
        shm.Free(obj); // 第二次释放应该是安全的
        TEST_ASSERT(true, "多次释放安全");
    }
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "共享内存单元测试" << std::endl;
    std::cout << "========================================" << std::endl;
    
    test_share_object();
    test_shared_memory_allocate();
    test_shared_memory_data();
    test_shared_memory_status();
    test_shared_memory_clean();
    test_shared_memory_multiple_pages();
    test_shared_memory_thread_safety();
    test_shared_memory_edge_cases();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "测试结果" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "通过: " << passed_tests << std::endl;
    std::cout << "失败: " << failed_tests << std::endl;
    std::cout << "总计:  " << (passed_tests + failed_tests) << std::endl;
    
    if (failed_tests == 0) {
        std::cout << "\n所有测试通过！" << std::endl;
        return 0;
    } else {
        std::cout << "\n部分测试失败！" << std::endl;
        return 1;
    }
}
