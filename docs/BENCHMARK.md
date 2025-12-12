### Performance Benchmark

General purpose allocators are slow, because: well because, they are designed for general purposes. Their ability to handle every size, every thread, and every edge case is not always a necessity.
Thus, the idea behind this project is to demonstrate the progression behind allocator designs that allow for blazingly fast performance without needing to forfeit safety.

# Phase 1 : Maximum speed

We first focus solely on pure single-threaded performance without regard to thread safety. By pre-allocating a memory pool and a stack based free list, the overhead from OS system calls is almost entirely removed.

```
| Benchmark                 |      Time |       CPU | Iterations |   Speedup |
| --------------------------|-----------|-----------|----------- |---------- |
| BM_PoolAllocator_Single   |  0.772 ns |  0.771 ns |  888035416 |    ~13.3x |
| BM_NewDelete_Single       |   10.3 ns |   10.3 ns |   67890432 |      1.0x |
| --------------------------|-----------|-----------|----------- |---------- |
| BM_PoolAllocator_Bulk     |  19364 ns |  19416 ns |      35814 |     ~9.0x |
| BM_NewDelete_Bulk         | 175021 ns | 174821 ns |       4029 |      1.0x |
| --------------------------|-----------|-----------|----------- |---------- |
| BM_PoolAllocator_Churn    |    165 ns |    165 ns |    4231547 |     ~9.2x |
| BM_NewDelete_Churn        |   1526 ns |   1523 ns |     465199 |      1.0x |
```

It can be seen that in the Single benchmark, which measures the overhead of obtaining a pointer, the pool allocator is effectively instant (0.8 ns). The pointer never leaves L1 cache (less than 1 ns to access) and is likely still inside a CPU register. It avoids the overhead of dynamic memory management including syscalls, thread synchronisation and constructor calls.
The Bulk and Churn benchamrks also demonstrate a 9x increase in throughput due to the stack's high memory locality.

# Phase 2 : The cost of safety

To develop the allocator towards a useable state for a real application, thread safety was implemented using `std::mutex` and `std::scoped_lock`, protecting the free list from race conditions.

```
| Benchmark                 |      Time |       CPU | Iterations |   Speedup |
| --------------------------|-----------|-----------|----------- |---------- |
| BM_PoolAllocator_Single   |   14.6 ns |   14.6 ns |   46963039 |     ~0.6x |
| BM_NewDelete_Single       |   9.08 ns |   9.06 ns |   78425634 |      1.0x |
| --------------------------|-----------|-----------|----------- |---------- |
| BM_PoolAllocator_Bulk     | 141040 ns | 140844 ns |       4949 |     ~1.1x |
| BM_NewDelete_Bulk         | 157514 ns | 157283 ns |       4403 |      1.0x |
| --------------------------|-----------|-----------|----------- |---------- |
| BM_PoolAllocator_Churn    |   1306 ns |   1304 ns |     534877 |     ~1.2x |
| BM_NewDelete_Churn        |   1545 ns |   1542 ns |     456269 |      1.0x |
```                                    

Safety is expensive, we can see that from phase 1, the cost of single allocations increased by (14.6 - 0.8 = 13.8) ns. Almost 95% of the time is spent waiting for the lock. `new` is faster here since standard malloc implementations use thread local storage to maintain caches of recently used memory blocks.
On the other hand, Bulk and Churn remain competitive and were slightly faster, proving that the cache locality is enough to completely offset the cost of a slow mutex.
Therefore to reclaim the performance from phase 1, implementing thread local pools will be the focus in the next phase.

