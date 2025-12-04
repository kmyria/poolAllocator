// customAlloc/customAlloc.hpp

#ifndef CUSTOM_ALLOC_HPP
#define CUSTOM_ALLOC_HPP

#include <cassert>
#include <cstddef>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <new>


// Tracking statistics incurred too much overhead, so 
// it's turned off right now.
template<bool StatTrak = false>

class PoolAllocator {
public:
    struct AllocatorStats {
        size_t total_blocks;
        size_t allocated_blocks;
        size_t peak_usage;
        int fragmentation_metric;
    };

    PoolAllocator(size_t object_size, size_t pool_size) {
        this->object_size = object_size;
        this->pool_size = pool_size; // number of blocks
        this->block_size = std::max(sizeof(FreeNode*), object_size);
        this->total_size = block_size * pool_size;

        this->allocated_blocks_count = 0;
        this->peak_usage_count = 0;

        memory_pool = ::operator new(total_size);
        create_free_list();
    }

    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    PoolAllocator(PoolAllocator&& other) noexcept :
        object_size(other.object_size),
        pool_size(other.pool_size),
        block_size(other.block_size),
        total_size(other.total_size),
        memory_pool(other.memory_pool),
        free_list_head(other.free_list_head),
        allocated_blocks_count(other.allocated_blocks_count),
        peak_usage_count(other.peak_usage_count)

    {
        other.memory_pool = nullptr;
        other.free_list_head = nullptr;
        other.allocated_blocks_count = 0;
    }

    PoolAllocator& operator=(PoolAllocator&& other) noexcept {
        if (this != &other) {
            ::operator delete(memory_pool);

            object_size = other.object_size;
            pool_size = other.pool_size;
            block_size = other.block_size;
            total_size = other.total_size;
            memory_pool = other.memory_pool;
            free_list_head = other.free_list_head;
            allocated_blocks_count = other.allocated_blocks_count;
            peak_usage_count = other.peak_usage_count;

            other.memory_pool = nullptr;
            other.free_list_head = nullptr;
            other.allocated_blocks_count = 0;
        }
        return *this;
    }

    ~PoolAllocator() {
        assert(allocated_blocks_count == 0);
        ::operator delete(memory_pool);
    }

    inline void *allocate_object() {
        if (!free_list_head) [[unlikely]] {
            return nullptr;
        }

        if constexpr (StatTrak) {
            allocated_blocks_count++;
            if (allocated_blocks_count > peak_usage_count) [[unlikely]] {
                peak_usage_count = allocated_blocks_count;
            }
        }

        FreeNode *head = free_list_head;
        free_list_head = free_list_head->next;
        return (void*)head;
    }

    inline void deallocate_object(void *ptr) {
        if (ptr) {

            if constexpr (StatTrak) {
                allocated_blocks_count--;
            }

            FreeNode *newHead = (FreeNode*)ptr;
            newHead->next = free_list_head;
            free_list_head = newHead;
        }
    }

    AllocatorStats get_allocator_stats() {
        return AllocatorStats {
            .total_blocks = pool_size,
            .allocated_blocks = allocated_blocks_count,
            .peak_usage = peak_usage_count,
            .fragmentation_metric = 0
        };
    }

private:
    struct FreeNode {
        FreeNode *next;
    };

    void create_free_list() {
        free_list_head = (FreeNode*)memory_pool;
        FreeNode *current = free_list_head;
        for (size_t i = 0; i < pool_size - 1; ++i) {
            current->next = (FreeNode*)((char*)current + block_size);
            current = current->next;
        }
        current->next = nullptr;
    }

    // utility (currently unused)
    bool validate_block_pointer(void *block) {
        if (block == nullptr) return false;
        if (block < memory_pool || block >= (char*)memory_pool + total_size) return false;
        if ((static_cast<char*>(block) - static_cast<char*>(memory_pool)) % block_size != 0) return false;
    
        return true;
    }

    void *memory_pool;
    FreeNode *free_list_head;

    size_t object_size;
    size_t pool_size;

    size_t block_size;
    size_t total_size;

    size_t allocated_blocks_count;
    size_t peak_usage_count;
};
#endif // CUSTOM_ALLOC_HPP
