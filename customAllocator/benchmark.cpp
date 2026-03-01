#include "custAlloc/custAlloc.hpp"
#include <benchmark/benchmark.h>
#include <vector>

struct obj {
    long long data1;
    long long data2;
    int data3;
};

constexpr size_t POOL_SIZE = 10000;

class AllocatorFixture : public benchmark::Fixture {
public:
    static PoolAllocator* shared_alloc;
    static int setup_count;
    static std::mutex init_mutex;

    void SetUp(const benchmark::State& state) override {
        std::lock_guard<std::mutex> lock(init_mutex);
        
        if (setup_count == 0) {
            size_t total_capacity = (POOL_SIZE * state.threads()) + (state.threads() * 1000);
            shared_alloc = new PoolAllocator(sizeof(obj), total_capacity);
        }
        setup_count++;
    }

    void TearDown(const benchmark::State& state) override {
        std::lock_guard<std::mutex> lock(init_mutex);
        setup_count--;
        
        if (setup_count == 0) {
            delete shared_alloc;
            shared_alloc = nullptr;
        }
    }
};

PoolAllocator* AllocatorFixture::shared_alloc = nullptr;
int AllocatorFixture::setup_count = 0;
std::mutex AllocatorFixture::init_mutex;


// SCENARIO 1: HIGH FREQUENCY ALLOCATION & DEALLOCATION (BEST CASE)

BENCHMARK_DEFINE_F(AllocatorFixture, Pool_Single)(benchmark::State& state) {
    for (auto _ : state) {
        void* p = shared_alloc->allocate_object();
        benchmark::DoNotOptimize(p);
        shared_alloc->deallocate_object(p);
    }
}
BENCHMARK_REGISTER_F(AllocatorFixture, Pool_Single)->ThreadRange(1, 8);

static void BM_NewDelete_Single(benchmark::State& state) {
    for (auto _ : state) {
        obj* p = new obj();
        benchmark::DoNotOptimize(p);
        delete p;
    }
}
BENCHMARK(BM_NewDelete_Single)->ThreadRange(1, 8);

// SCENARIO 2: BULK ALLOCATION & DEALLOCATION

BENCHMARK_DEFINE_F(AllocatorFixture, Pool_Bulk)(benchmark::State& state) {
    std::vector<void*> objects;
    objects.reserve(POOL_SIZE);

    for (auto _ : state) {
        for (size_t i = 0; i < POOL_SIZE; ++i) {
            objects.push_back(shared_alloc->allocate_object());
        }

        for (void* p : objects) {
            shared_alloc->deallocate_object(p);
        }

        state.PauseTiming();
        objects.clear();
        state.ResumeTiming();
    }
}
BENCHMARK_REGISTER_F(AllocatorFixture, Pool_Bulk)->ThreadRange(1, 8);

static void BM_NewDelete_Bulk(benchmark::State& state) {
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
BENCHMARK(BM_NewDelete_Bulk)->ThreadRange(1, 8);

// SCENARIO 3: MEMORY CHURN (REALISTIC WORKLOAD)

BENCHMARK_DEFINE_F(AllocatorFixture, Pool_Churn)(benchmark::State& state) {
    std::vector<void*> objects;
    objects.reserve(POOL_SIZE);

    for (size_t i = 0; i < POOL_SIZE / 2; ++i) {
        objects.push_back(shared_alloc->allocate_object());
    }

    for (auto _ : state) {
        for (size_t i = 0; i < 100; ++i) {
            shared_alloc->deallocate_object(objects[i]);
        }
        for (size_t i = 0; i < 100; ++i) {
            objects[i] = shared_alloc->allocate_object();
        }
    }
}
BENCHMARK_REGISTER_F(AllocatorFixture, Pool_Churn)->ThreadRange(1, 8);

static void BM_NewDelete_Churn(benchmark::State& state) {
    std::vector<obj*> objects;
    objects.reserve(POOL_SIZE);

    for (size_t i = 0; i < POOL_SIZE / 2; ++i) {
        objects.push_back(new obj());
    }

    for (auto _ : state) {
        for (size_t i = 0; i < 100; ++i) {
            delete objects[i];
        }
        for (size_t i = 0; i < 100; ++i) {
            objects[i] = new obj();
        }
    }
}
BENCHMARK(BM_NewDelete_Churn)->ThreadRange(1, 8);

BENCHMARK_MAIN();
