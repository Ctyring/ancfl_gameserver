#ifndef __SHARED_MEMORY_H__
#define __SHARED_MEMORY_H__

#include <string>
#include <vector>
#include <memory>

namespace game_server {

// 共享内存状态枚举
enum class SharedMemoryStatus {
    NONE,       // 未使用空闲状态
    USE,        // 已经使用了，数据库服务器可以读取修改写入数据库
    LOCK,       // 锁住状态，逻辑服务器正在写入
    RELEASE,    // 逻辑服务器已经释放了。数据库服务器写入修改后可以置为NONE状态
    DELETE      // 删除标志
};

// 所有放到sharedMemory里的元素都必须是从ShareObject派生的
class ShareObject {
public:
    ShareObject();
    virtual ~ShareObject() = default;

    // 开始修改，标记为被占用
    void Lock();

    // 标记为修改完成
    void Unlock();

    // 标记为已经释放了
    void Release();

    // 标记为删除
    void Destroy();

    void UseIt();

    void Reset();

    bool IsLock() const;

    bool IsDestroy() const;

    bool IsRelease() const;

    bool IsUse() const;

    time_t getLastMotifyTime() const;

    SharedMemoryStatus GetStatus() const;

    int32_t GetCheckCode() const;

protected:
    static const int32_t BLOCK_CHECK_CODE = 0x5A;
    int32_t check_code_;
    SharedMemoryStatus status_;
    time_t update_time_;    // 最后一次修改时间
};

// 记录每个数据块的状态
struct SMBlock {
    int32_t index;      // 数据当前编号
    bool in_use;        // 是否在使用true是正在使用，false是没有使用
    bool new_block;     // 是否是刚刚新创建的区块
    time_t before_time; // DS服务器更新完成后回写的信息时间
    time_t after_time;

    SMBlock() : index(0), in_use(false), new_block(false), before_time(0), after_time(0) {}
};

// 共享内存页结构
struct ShareMemoryPage {
    char* data;         // 指定共享内存地址
    SMBlock* blocks;    // 数据块的头位置
    void* handle;       // 共享内存句柄
    int32_t size;       // 页面大小
    int32_t block_count; // 数据块数量
};

// 共享内存基类
class SharedMemoryBase {
public:
    SharedMemoryBase(int32_t module_id, int32_t block_size, int32_t count_per_page, bool no_create = false);
    virtual ~SharedMemoryBase();

    // 创建新页面
    bool NewPage();

    // 分配数据块
    void* Allocate();

    // 释放数据块
    void Free(void* data);

    // 获取数据块状态
    SharedMemoryStatus GetStatus(void* data) const;

    // 设置数据块状态
    void SetStatus(void* data, SharedMemoryStatus status);

    // 获取数据块数量
    int32_t GetBlockCount() const;

    // 获取已使用数据块数量
    int32_t GetUsedBlockCount() const;

    // 清理过期数据
    void CleanExpiredData(time_t expired_time);

protected:
    int32_t module_id_;      // 模块ID
    int32_t block_size_;     // 数据块大小
    int32_t count_per_page_; // 每页数据块数量
    int32_t page_count_;     // 页面数量
    int32_t used_blocks_;    // 已使用数据块数量

    using ShareMemoryPageMapping = std::vector<ShareMemoryPage>;
    ShareMemoryPageMapping pages_; // 共享内存页映射

    // 计算数据块在页面中的索引
    bool GetBlockIndex(void* data, int32_t& page_index, int32_t& block_index) const;

    // 创建共享内存
    void* CreateShareMemory(int32_t module_id, int32_t page_index, int32_t size);

    // 映射共享内存
    void* MapShareMemory(int32_t module_id, int32_t page_index, int32_t size);

    // 解除映射
    void UnmapShareMemory(void* data, int32_t size);

    // 销毁共享内存
    void DestroyShareMemory(void* handle);
};

// 模板类，用于具体类型的共享内存管理
template <typename T>
class SharedMemory : public SharedMemoryBase {
public:
    SharedMemory(int32_t module_id, int32_t count_per_page, bool no_create = false)
        : SharedMemoryBase(module_id, sizeof(T), count_per_page, no_create) {}

    // 分配数据块
    T* Allocate() {
        return static_cast<T*>(SharedMemoryBase::Allocate());
    }

    // 释放数据块
    void Free(T* data) {
        SharedMemoryBase::Free(data);
    }
};

} // namespace game_server

#endif // __SHARED_MEMORY_H__
