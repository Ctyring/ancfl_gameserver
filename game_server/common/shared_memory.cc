#include "shared_memory.h"
#include <cstring>
#include <iostream>
#include <ctime>
#include <errno.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace game_server {

ShareObject::ShareObject() : 
    check_code_(BLOCK_CHECK_CODE),
    status_(SharedMemoryStatus::USE),
    update_time_(time(nullptr)) {
}

void ShareObject::Lock() {
    std::lock_guard<std::mutex> lock(mutex_);
    status_ = SharedMemoryStatus::LOCK;
    update_time_ = time(nullptr);
}

bool ShareObject::IsLock() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return status_ == SharedMemoryStatus::LOCK;
}

void ShareObject::Unlock() {
    std::lock_guard<std::mutex> lock(mutex_);
    update_time_ = time(nullptr);
    status_ = SharedMemoryStatus::USE;
}

void ShareObject::UseIt() {
    std::lock_guard<std::mutex> lock(mutex_);
    status_ = SharedMemoryStatus::USE;
    update_time_ = time(nullptr);
}

void ShareObject::Release() {
    std::lock_guard<std::mutex> lock(mutex_);
    status_ = SharedMemoryStatus::RELEASE;
    update_time_ = time(nullptr);
}

void ShareObject::Destroy() {
    std::lock_guard<std::mutex> lock(mutex_);
    status_ = SharedMemoryStatus::DELETE;
    update_time_ = time(nullptr);
}

bool ShareObject::IsDestroy() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return status_ == SharedMemoryStatus::DELETE;
}

bool ShareObject::IsRelease() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return status_ == SharedMemoryStatus::RELEASE;
}

time_t ShareObject::getLastMotifyTime() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return update_time_;
}

SharedMemoryStatus ShareObject::GetStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return status_;
}

int32_t ShareObject::GetCheckCode() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return check_code_;
}

bool ShareObject::IsUse() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return status_ != SharedMemoryStatus::NONE;
}

void ShareObject::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    status_ = SharedMemoryStatus::NONE;
    update_time_ = time(nullptr);
}

SharedMemoryBase::SharedMemoryBase(int32_t module_id, int32_t block_size, int32_t count_per_page, bool no_create) 
    : module_id_(module_id),
      block_size_(block_size),
      count_per_page_(count_per_page),
      page_count_(0),
      used_blocks_(0) {
    // 确保block_size至少包含ShareObject的大小
    if (block_size_ < static_cast<int32_t>(sizeof(ShareObject))) {
        block_size_ = static_cast<int32_t>(sizeof(ShareObject));
    }
}

SharedMemoryBase::~SharedMemoryBase() {
    // 清理所有页面
    for (auto& page : pages_) {
        if (page.data) {
            UnmapShareMemory(page.data, page.size);
        }
        if (page.handle) {
            DestroyShareMemory(page.handle);
        }
    }
    pages_.clear();
}

bool SharedMemoryBase::NewPage() {
    int32_t block_size = block_size_;
    int32_t page_size = count_per_page_ * (block_size + sizeof(SMBlock));
    int32_t current_page_index = page_count_;

    ShareMemoryPage new_page;
    new_page.handle = CreateShareMemory(module_id_, current_page_index, page_size);
    if (new_page.handle == nullptr) {
        return false;
    }

    new_page.data = static_cast<char*>(MapShareMemoryByHandle(new_page.handle, page_size));
    if (!new_page.data) {
        DestroyShareMemory(new_page.handle);
        return false;
    }

    new_page.size = page_size;
    new_page.block_count = count_per_page_;
    new_page.blocks = reinterpret_cast<SMBlock*>(new_page.data + count_per_page_ * block_size);

    // 初始化数据块
    for (int32_t i = 0; i < count_per_page_; ++i) {
        new_page.blocks[i].index = current_page_index * count_per_page_ + i;
        new_page.blocks[i].in_use = false;
        new_page.blocks[i].new_block = false;
        new_page.blocks[i].before_time = 0;
        new_page.blocks[i].after_time = 0;
    }

    pages_.push_back(new_page);
    page_count_++;
    return true;
}

void* SharedMemoryBase::Allocate() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 查找可用数据块
    for (auto& page : pages_) {
        for (int32_t i = 0; i < page.block_count; ++i) {
            if (!page.blocks[i].in_use) {
                page.blocks[i].in_use = true;
                page.blocks[i].new_block = true;
                page.blocks[i].before_time = time(nullptr);
                used_blocks_++;
                
                // 初始化ShareObject对象
                void* data = page.data + i * block_size_;
                new (data) ShareObject();
                
                return data;
            }
        }
    }

    // 没有可用数据块，创建新页面
    if (!NewPage()) {
        return nullptr;
    }

    // 在新页面中分配第一个数据块
    auto& page = pages_.back();
    page.blocks[0].in_use = true;
    page.blocks[0].new_block = true;
    page.blocks[0].before_time = time(nullptr);
    used_blocks_++;
    
    // 初始化ShareObject对象
    void* data = page.data + 0 * block_size_;
    new (data) ShareObject();
    
    return data;
}

void SharedMemoryBase::Free(void* data) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    int32_t page_index = 0;
    int32_t block_index = 0;
    if (!GetBlockIndex(data, page_index, block_index)) {
        return;
    }

    if (page_index < 0 || page_index >= static_cast<int32_t>(pages_.size())) {
        return;
    }

    auto& page = pages_[page_index];
    if (block_index < 0 || block_index >= page.block_count) {
        return;
    }

    if (page.blocks[block_index].in_use) {
        page.blocks[block_index].in_use = false;
        page.blocks[block_index].new_block = false;
        used_blocks_--;
    }
}

SharedMemoryStatus SharedMemoryBase::GetStatus(void* data) const {
    ShareObject* obj = static_cast<ShareObject*>(data);
    return obj->GetStatus();
}

void SharedMemoryBase::SetStatus(void* data, SharedMemoryStatus status) {
    ShareObject* obj = static_cast<ShareObject*>(data);
    switch (status) {
    case SharedMemoryStatus::LOCK:
        obj->Lock();
        break;
    case SharedMemoryStatus::USE:
        obj->UseIt();
        break;
    case SharedMemoryStatus::RELEASE:
        obj->Release();
        break;
    case SharedMemoryStatus::DELETE:
        obj->Destroy();
        break;
    case SharedMemoryStatus::NONE:
        obj->Reset();
        break;
    }
}

int32_t SharedMemoryBase::GetBlockCount() const {
    return page_count_ * count_per_page_;
}

int32_t SharedMemoryBase::GetUsedBlockCount() const {
    return used_blocks_;
}

void SharedMemoryBase::CleanExpiredData(time_t expired_time) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& page : pages_) {
        for (int32_t i = 0; i < page.block_count; ++i) {
            if (page.blocks[i].in_use) {
                void* data = page.data + i * block_size_;
                ShareObject* obj = static_cast<ShareObject*>(data);
                if (obj->getLastMotifyTime() < expired_time) {
                    // 注意：这里不直接调用 Free()，因为 Free() 也会加锁，会导致死锁
                    // 直接修改状态，避免死锁
                    page.blocks[i].in_use = false;
                    page.blocks[i].new_block = false;
                    used_blocks_--;
                }
            }
        }
    }
}

bool SharedMemoryBase::GetBlockIndex(void* data, int32_t& page_index, int32_t& block_index) const {
    for (int32_t i = 0; i < static_cast<int32_t>(pages_.size()); ++i) {
        const auto& page = pages_[i];
        // 只检查数据块区域，不包括块信息区域
        size_t data_area_size = page.block_count * block_size_;
        if (data >= page.data && data < page.data + data_area_size) {
            page_index = i;
            block_index = (static_cast<char*>(data) - page.data) / block_size_;
            return true;
        }
    }
    return false;
}

void* SharedMemoryBase::CreateShareMemory(int32_t module_id, int32_t page_index, int32_t size) {
#ifdef _WIN32
    std::string name = "Global\\GameServer_" + std::to_string(module_id) + "_" + std::to_string(page_index);
    HANDLE hMap = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        size,
        name.c_str()
    );
    return hMap;
#else
    // 使用模块ID和页索引生成唯一的键值
    // 为了避免键值冲突，我们使用一个固定的文件路径，但是使用不同的项目ID
    int proj_id = (module_id * 10 + page_index) % 255;
    if (proj_id == 0) proj_id = 1; // 避免proj_id为0
    key_t key = ftok("/etc/passwd", proj_id);
    if (key == -1) {
        std::cerr << "ftok failed for module_id=" << module_id << ", page_index=" << page_index << ", proj_id=" << proj_id << std::endl;
        return nullptr;
    }
    
    // 先尝试删除现有的共享内存
    int shmid = shmget(key, 0, 0666);
    if (shmid != -1) {
        shmctl(shmid, IPC_RMID, NULL);
        // 短暂延迟，确保删除操作生效
        struct timespec ts = {0, 100000000}; // 100毫秒
        nanosleep(&ts, NULL);
    }
    
    // 创建新的共享内存，最多重试3次
    for (int retry = 0; retry < 3; retry++) {
        shmid = shmget(key, size, IPC_CREAT | IPC_EXCL | 0666);
        if (shmid != -1) {
            // 使用 intptr_t 来存储 shmid，确保 shmid=0 也能正确返回
            // 加1是为了避免返回nullptr
            return reinterpret_cast<void*>(static_cast<intptr_t>(shmid + 1));
        }
        
        // 如果创建失败，再次尝试删除现有的共享内存
        shmid = shmget(key, 0, 0666);
        if (shmid != -1) {
            shmctl(shmid, IPC_RMID, NULL);
            // 短暂延迟，确保删除操作生效
            struct timespec ts = {0, 50000000}; // 50毫秒
            nanosleep(&ts, NULL);
        }
    }
    
    return nullptr;
#endif
}

void* SharedMemoryBase::MapShareMemory(int32_t module_id, int32_t page_index, int32_t size) {
#ifdef _WIN32
    std::string name = "Global\\GameServer_" + std::to_string(module_id) + "_" + std::to_string(page_index);
    HANDLE hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name.c_str());
    if (!hMap) {
        return nullptr;
    }
    void* addr = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, size);
    CloseHandle(hMap);
    return addr;
#else
    key_t key = ftok("/etc/passwd", module_id * 256 + page_index);
    int shmid = shmget(key, size, 0666);
    if (shmid == -1) {
        return nullptr;
    }
    void* addr = shmat(shmid, NULL, 0);
    if (addr == (void*)-1) {
        return nullptr;
    }
    return addr;
#endif
}

void* SharedMemoryBase::MapShareMemoryByHandle(void* handle, int32_t size) {
#ifdef _WIN32
    void* addr = MapViewOfFile(reinterpret_cast<HANDLE>(handle), FILE_MAP_ALL_ACCESS, 0, 0, size);
    return addr;
#else
    // 减1是因为CreateShareMemory中加了1
    long shmid = reinterpret_cast<long>(handle) - 1;
    void* addr = shmat(shmid, NULL, 0);
    if (addr == (void*)-1) {
        return nullptr;
    }
    return addr;
#endif
}

void SharedMemoryBase::UnmapShareMemory(void* data, int32_t size) {
#ifdef _WIN32
    UnmapViewOfFile(data);
#else
    shmdt(data);
#endif
}

void SharedMemoryBase::DestroyShareMemory(void* handle) {
#ifdef _WIN32
    CloseHandle(static_cast<HANDLE>(handle));
#else
    // 减1是因为CreateShareMemory中加了1
    int shmid = static_cast<int>(reinterpret_cast<intptr_t>(handle)) - 1;
    shmctl(shmid, IPC_RMID, NULL);
#endif
}

} // namespace game_server
