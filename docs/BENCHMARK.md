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

# Phase 3 : Cheap safety, big performance, what a bargain!

To be added when I get bored during a lecture.

```
| Benchmark                                       |      Time |       CPU | Iterations |   Speedup |
| ----------------------------------------------- | ----------| ----------| -----------| ----------|
| AllocatorFixture/Pool_Single/threads:1          |    2.63 ns|    2.63 ns|   268580275|     ~3.7x |
| AllocatorFixture/Pool_Single/threads:2          |    2.82 ns|    2.80 ns|   231646778|     ~3.7x |
| AllocatorFixture/Pool_Single/threads:4          |    2.87 ns|    2.87 ns|   226159916|     ~3.7x |
| AllocatorFixture/Pool_Single/threads:8          |    6.11 ns|    6.04 ns|   106462888|     ~4.0x |
| BM_NewDelete_Single/threads:1                   |    9.71 ns|    9.68 ns|    71105082|      1.0x |
| BM_NewDelete_Single/threads:2                   |   10.40 ns|   10.40 ns|    71208272|      1.0x |
| BM_NewDelete_Single/threads:4                   |   10.50 ns|   10.50 ns|    66216444|      1.0x |
| BM_NewDelete_Single/threads:8                   |   24.30 ns|   24.10 ns|    29299672|      1.0x |
| AllocatorFixture/Pool_Bulk/threads:1            |   64560 ns|   64408 ns|       10701|     ~7.5x |
| AllocatorFixture/Pool_Bulk/threads:2            |   84748 ns|   84582 ns|        7690|     ~6.0x |
| AllocatorFixture/Pool_Bulk/threads:4            |  169712 ns|  169122 ns|        4876|     ~3.1x |
| AllocatorFixture/Pool_Bulk/threads:8            |  191593 ns|  179959 ns|        3224|     ~5.4x |
| BM_NewDelete_Bulk/threads:1                     |  487530 ns|  485991 ns|        1396|      1.0x |
| BM_NewDelete_Bulk/threads:2                     |  508157 ns|  506963 ns|        1386|      1.0x |
| BM_NewDelete_Bulk/threads:4                     |  525271 ns|  524092 ns|        1336|      1.0x |
| BM_NewDelete_Bulk/threads:8                     |  972713 ns|  968613 ns|         696|      1.0x |
| AllocatorFixture/Pool_Churn/threads:1           |     396 ns|     395 ns|     1785487|    ~10.8x |
| AllocatorFixture/Pool_Churn/threads:2           |     403 ns|     401 ns|     1719528|    ~12.6x |
| AllocatorFixture/Pool_Churn/threads:4           |     419 ns|     418 ns|     1629304|    ~15.0x |
| AllocatorFixture/Pool_Churn/threads:8           |     548 ns|     546 ns|     1275800|    ~22.8x |
| BM_NewDelete_Churn/threads:1                    |    4276 ns|    4264 ns|      166600|      1.0x |
| BM_NewDelete_Churn/threads:2                    |    5073 ns|    5050 ns|      153432|      1.0x |
| BM_NewDelete_Churn/threads:4                    |    6281 ns|    6263 ns|      113372|      1.0x |
| BM_NewDelete_Churn/threads:8                    |   12500 ns|   12432 ns|       56888|      1.0x |
```


