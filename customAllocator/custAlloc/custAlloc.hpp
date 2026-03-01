// customAlloc/customAlloc.hpp

#ifndef CUSTOM_ALLOC_HPP
#define CUSTOM_ALLOC_HPP

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>

class PoolAllocator {
public:
    PoolAllocator(size_t object_size, size_t pool_size)
    {
        size_t req_size = std::max(sizeof(FreeNode*), object_size);
        const size_t alignment = 64;
        block_size = (req_size + alignment - 1) & ~(alignment - 1);

        total_size = block_size * pool_size;
        memory_pool = ::operator new(total_size, std::align_val_t(64));
        assert(reinterpret_cast<std::uintptr_t>(memory_pool) % 64 == 0);
        FreeNode* first = static_cast<FreeNode*>(memory_pool);
        FreeNode* current = first;
        for (size_t i = 0; i < pool_size - 1; ++i) {
            current->next = (FreeNode*)((char*)current + block_size);
            current = current->next;
        }
        current->next = nullptr;

        free_list_head_.store(pack(first, 0), std::memory_order_release);
    }

    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;
    PoolAllocator(PoolAllocator&&) = delete;
    PoolAllocator& operator=(PoolAllocator&&) = delete;

    ~PoolAllocator()
    {
        if (TL.owner == this) {
            TL.cache_head = nullptr;
            TL.cache_count = 0;
            TL.owner = nullptr;
        }
        ::operator delete(memory_pool, total_size, std::align_val_t(64));
    }

    inline void* allocate_object()
    {
        if (TL.owner != this) {
            TL.cache_head = nullptr;
            TL.cache_count = 0;
            TL.owner = this;
        }

        if (TL.cache_head) {
            return TL_pop();
        }

        refill_local();

        if (TL.cache_head) {
            return TL_pop();
        }

        return nullptr;
    }

    inline void deallocate_object(void* ptr)
    {
        if (TL.owner != this) {
            TL.cache_head = nullptr;
            TL.cache_count = 0;
            TL.owner = this;
        }

        if (!ptr)
            return;

        TL_push(ptr);

        if (TL.cache_count >= TL_CAP)
            spill_local(TL_SPILL);
    }

private:
    struct FreeNode {
        FreeNode* next;
    };

    struct HeadView {
        uintptr_t packed;
        FreeNode* ptr() const
        {
            return reinterpret_cast<FreeNode*>(packed & PTR_MASK);
        }
        std::uint64_t tag() const { return (packed & TAG_MASK); }

        // FreeNode* ptr;
        // std::uint64_t tag;
        // packed = (ptr & PTR_MASK) | (tag & TAG_MASK)
    };

    uintptr_t pack(FreeNode* ptr, std::uint64_t tag)
    {
        return (reinterpret_cast<uintptr_t>(ptr) & PTR_MASK) | (tag & TAG_MASK);
    }

    static constexpr uintptr_t TAG_BITS = 6;
    static constexpr uintptr_t TAG_MASK
        = (std::uintptr_t { 1 } << TAG_BITS) - 1;
    static constexpr uintptr_t PTR_MASK = ~TAG_MASK;

    alignas(std::hardware_constructive_interference_size)
        std::atomic<uintptr_t> free_list_head_;

    struct ThreadLocalState {
        FreeNode* cache_head = nullptr;
        size_t cache_count = 0;
        PoolAllocator* owner = nullptr;
    };

    static thread_local ThreadLocalState TL;

    static constexpr size_t TL_CAP = 96;
    static constexpr size_t TL_REFILL = 32;
    static constexpr size_t TL_SPILL = 32;
    static constexpr size_t TL_TARGET = 64;

    FreeNode* TL_pop()
    {
        FreeNode* n = TL.cache_head;
        TL.cache_head = n->next;
        --TL.cache_count;
        return n;
    }

    bool TL_push(void* ptr)
    {
        if (!ptr)
            return false;

        FreeNode* n = static_cast<FreeNode*>(ptr);
        n->next = TL.cache_head;
        TL.cache_head = n;
        ++TL.cache_count;

        return true;
    }

    FreeNode* global_pop_single()
    {
        uintptr_t old_packed = free_list_head_.load(std::memory_order_acquire);

        HeadView old_head;
        uintptr_t new_packed;
        do {
            old_head = { old_packed };
            if (!old_head.ptr())
                return nullptr;
            new_packed = pack(old_head.ptr()->next, old_head.tag() + 1);

        } while (!free_list_head_.compare_exchange_weak(old_packed, new_packed,
            std::memory_order_acquire, std::memory_order_acquire));

        return old_head.ptr();
    }

    void global_push_batch(void* start, void* end)
    {
        if (!start || !end)
            return;

        FreeNode* batch_head = static_cast<FreeNode*>(start);
        FreeNode* batch_tail = static_cast<FreeNode*>(end);

        uintptr_t old_packed = free_list_head_.load(std::memory_order_relaxed);
        uintptr_t new_packed;

        HeadView old_head;

        do {
            old_head = { old_packed };
            batch_tail->next = old_head.ptr();
            new_packed = pack(batch_head, old_head.tag() + 1);

        } while (!free_list_head_.compare_exchange_weak(old_packed, new_packed,
            std::memory_order_release, std::memory_order_relaxed));
    }

    void refill_local()
    {
        while (TL.cache_count < TL_TARGET && TL_push(global_pop_single()))
            ;
    }

    void spill_local(size_t count)
    {
        count = std::min(count, TL.cache_count);

        if (count == 0)
            return;

        FreeNode* head = TL.cache_head;
        FreeNode* tail = head;
        for (size_t i = 1; i < count; i++) {
            tail = tail->next;
        }

        TL.cache_head = tail->next;
        TL.cache_count -= count;

        global_push_batch(head, tail);
    }

    void* memory_pool;

    size_t block_size;
    size_t total_size;
};

inline thread_local PoolAllocator::ThreadLocalState PoolAllocator::TL;

#endif // CUSTOM_ALLOC_HPP
