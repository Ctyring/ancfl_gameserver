#include "shared_memory.h"
#include <cstring>
#include <iostream>

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
    update_time_(0) {
}

void ShareObject::Lock() {
    status_ = SharedMemoryStatus::LOCK;
}

bool ShareObject::IsLock() const {
    return status_ == SharedMemoryStatus::LOCK;
}

void ShareObject::Unlock() {
    update_time_ = time(nullptr);
    status_ = SharedMemoryStatus::USE;
}

void ShareObject::UseIt() {
    status_ = SharedMemoryStatus::USE;
}

void ShareObject::Release() {
    status_ = SharedMemoryStatus::RELEASE;
}

void ShareObject::Destroy() {
    status_ = SharedMemoryStatus::DELETE;
}

bool ShareObject::IsDestroy() const {
    return status_ == SharedMemoryStatus::DELETE;
}

bool ShareObject::IsRelease() const {
    return status_ == SharedMemoryStatus::RELEASE;
}

time_t ShareObject::getLastMotifyTime() const {
    return update_time_;
}

SharedMemoryStatus ShareObject::GetStatus() const {
    return status_;
}

int32_t ShareObject::GetCheckCode() const {
    return check_code_;
}

bool ShareObject::IsUse() const {
    return status_ != SharedMemoryStatus::NONE;
}

void ShareObject::Reset() {
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
    if (block_size_ < sizeof(ShareObject)) {
        block_size_ = sizeof(ShareObject);
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

    ShareMemoryPage new_page;
    new_page.handle = CreateShareMemory(module_id_, page_count_, page_size);
    if (!new_page.handle) {
        return false;
    }

    new_page.data = static_cast<char*>(MapShareMemory(module_id_, page_count_, page_size));
    if (!new_page.data) {
        DestroyShareMemory(new_page.handle);
        return false;
    }

    new_page.size = page_size;
    new_page.block_count = count_per_page_;
    new_page.blocks = reinterpret_cast<SMBlock*>(new_page.data + count_per_page_ * block_size);

    // 初始化数据块
    for (int32_t i = 0; i < count_per_page_; ++i) {
        new_page.blocks[i].index = page_count_ * count_per_page_ + i;
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
    // 查找可用数据块
    for (auto& page : pages_) {
        for (int32_t i = 0; i < page.block_count; ++i) {
            if (!page.blocks[i].in_use) {
                page.blocks[i].in_use = true;
                page.blocks[i].new_block = true;
                page.blocks[i].before_time = time(nullptr);
                used_blocks_++;
                return page.data + i * block_size_;
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
    return page.data;
}

void SharedMemoryBase::Free(void* data) {
    int32_t page_index = 0;
    int32_t block_index = 0;
    if (!GetBlockIndex(data, page_index, block_index)) {
        return;
    }

    if (page_index < 0 || page_index >= pages_.size()) {
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
    for (auto& page : pages_) {
        for (int32_t i = 0; i < page.block_count; ++i) {
            if (page.blocks[i].in_use) {
                void* data = page.data + i * block_size_;
                ShareObject* obj = static_cast<ShareObject*>(data);
                if (obj->getLastMotifyTime() < expired_time) {
                    Free(data);
                }
            }
        }
    }
}

bool SharedMemoryBase::GetBlockIndex(void* data, int32_t& page_index, int32_t& block_index) const {
    for (int32_t i = 0; i < pages_.size(); ++i) {
        const auto& page = pages_[i];
        if (data >= page.data && data < page.data + page.size) {
            page_index = i;
            block_index = static_cast<char*>(data) - page.data / block_size_;
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
    key_t key = ftok(".", module_id * 1000 + page_index);
    int shmid = shmget(key, size, IPC_CREAT | 0666);
    if (shmid == -1) {
        return nullptr;
    }
    return reinterpret_cast<void*>(shmid);
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
    key_t key = ftok(".", module_id * 1000 + page_index);
    int shmid = shmget(key, size, 0666);
    if (shmid == -1) {
        return nullptr;
    }
    void* addr = shmat(shmid, NULL, 0);
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
    int shmid = reinterpret_cast<int>(handle);
    shmctl(shmid, IPC_RMID, NULL);
#endif
}

} // namespace game_server
