#include "custAlloc/custAlloc.hpp"
#include <atomic>
#include <benchmark/benchmark.h>
#include <cstddef>

struct obj {
    long long data1;
    long long data2;
    int data3;
};

constexpr size_t POOL_SIZE = 10000;

class PoolAllocFixture : public benchmark::Fixture {
public:
    static inline PoolAllocator* alloc = nullptr;

    void SetUp(const ::benchmark::State& state) override
    {
        if (state.thread_index() == 0) {
            const size_t threads = state.threads();
            const size_t POOL_SIZE = 100'000 * threads;
            alloc = new PoolAllocator(sizeof(obj), POOL_SIZE);
        }
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }

    void TearDown(const ::benchmark::State& state) override
    {
        if (state.thread_index() == 0) {
            delete alloc;
            alloc = nullptr;
        }
    }
};

// SCENARIO 1: HIGH FREQUENCY ALLOCATION & DEALLOCATION (BEST CASE)

static void BM_PoolAllocator_Single(benchmark::State& state)
{
    static PoolAllocator* alloc = new PoolAllocator(sizeof(obj), POOL_SIZE);
    for (auto _ : state) {
        void* p = alloc->allocate_object();
        benchmark::DoNotOptimize(p);
        alloc->deallocate_object(p);
    }
}

BENCHMARK(BM_PoolAllocator_Single);

static void BM_NewDelete_Single(benchmark::State& state)
{
    for (auto _ : state) {
        obj* p = new obj();
        benchmark::DoNotOptimize(p);
        delete p;
    }
}
BENCHMARK(BM_NewDelete_Single);

// SCENARIO 2: BULK ALLOCATION & DEALLOCATION

static void BM_PoolAllocator_Bulk(benchmark::State& state)
{
    PoolAllocator allocator(sizeof(obj), POOL_SIZE);
    std::vector<void*> objects;
    objects.reserve(POOL_SIZE);

    for (auto _ : state) {
        for (size_t i = 0; i < POOL_SIZE; ++i) {
            objects.push_back(allocator.allocate_object());
        }

        for (void* p : objects) {
            allocator.deallocate_object(p);
        }

        // Ensure that cleanup work isn't included in the result.
        state.PauseTiming();
        objects.clear();
        state.ResumeTiming();
    }
}
BENCHMARK(BM_PoolAllocator_Bulk);

static void BM_NewDelete_Bulk(benchmark::State& state)
{
    std::vector<obj*> objects;
    objects.reserve(POOL_SIZE);

    for (auto _ : state) {
        for (size_t i = 0; i < POOL_SIZE; ++i) {
            objects.push_back(new obj());
        }

        for (obj* p : objects) {
            delete p;
        }

        state.PauseTiming();
        objects.clear();
        state.ResumeTiming();
    }
}
BENCHMARK(BM_NewDelete_Bulk);

// SCENARIO 3: MEMORY CHURN (REALISTIC WORKLOAD)

static void BM_PoolAllocator_Churn(benchmark::State& state)
{
    PoolAllocator allocator(sizeof(obj), POOL_SIZE);
    std::vector<void*> objects;
    objects.reserve(POOL_SIZE);

    // Pre-fill the allocator to about 50% capacity to simulate a steady state.
    for (size_t i = 0; i < POOL_SIZE / 2; ++i) {
        objects.push_back(allocator.allocate_object());
    }

    for (auto _ : state) {
        for (size_t i = 0; i < 100; ++i) {
            allocator.deallocate_object(objects[i]);
        }
        for (size_t i = 0; i < 100; ++i) {
            objects[i] = allocator.allocate_object();
        }
    }
}
BENCHMARK(BM_PoolAllocator_Churn);

static void BM_NewDelete_Churn(benchmark::State& state)
{
    std::vector<obj*> objects;
    objects.reserve(POOL_SIZE);

    for (size_t i = 0; i < POOL_SIZE / 2; ++i) {
        objects.push_back(new obj());
    }

    for (auto _ : state) {
        for (size_t i = 0; i < 100; ++i) {
            delete objects[i];
            objects[i] = nullptr;
        }
        for (size_t i = 0; i < 100; ++i) {
            objects[i] = new obj();
        }
    }

    state.PauseTiming();
    for (auto* p : objects) {
        delete p;
    }
    objects.clear();
}
BENCHMARK(BM_NewDelete_Churn);

BENCHMARK_MAIN();
