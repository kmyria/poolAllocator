# High Performance C++ Pool Allocator

A minimal but highly efficient fixed-size pool allocator written in modern C++. This project was created to explore and demonstrate the significant performance advantages of custom memory management strategies over standard library allocators (`new`/`delete`) in performance-critical applications.

The core focus is pure speed, achieved by eliminating operating system overhead and leveraging a simple, cache-friendly free-list design. The allocator's performance is validated using the Google Benchmark library.

### Key Features

*   **Fast:** Up to **13x faster** than standard `new`/`delete` in high-frequency allocation scenarios.
*   **Modern C++:** Built with C++17/20 features for compile time optimisations.
*   **Performance-Aware Design:** Optional, compile time statistics tracking to ensure zero overhead in release builds.
*   **Professionally Benchmarked:** Performance is proven with a comprehensive suite of tests using Google Benchmark.
*   **Standard Build System:** Uses CMake for convenient, cross platform building and integration.

## Performance Benchmark

The primary goal of this allocator is speed. The following benchmarks were run on my Intel Core i7-9700K. It compares the performance of `PoolAllocator` against standard `new`/`delete` across several realistic usage patterns.

```
Run on (8 X 3991.96 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 0.58, 1.15, 1.37

| Benchmark                 |      Time |       CPU | Iterations |   Speedup |
| --------------------------|-----------|-----------|----------- |---------- |
| BM_PoolAllocator_Single   |  0.772 ns |  0.771 ns |  888035416 |    ~13.3x |
| BM_NewDelete_Single       |   10.3 ns |   10.3 ns |   67890432 |      1.0x |
| BM_PoolAllocator_Bulk     |  19364 ns |  19416 ns |      35814 |     ~9.0x |
| BM_NewDelete_Bulk         | 175021 ns | 174821 ns |       4029 |      1.0x |
| BM_PoolAllocator_Churn    |    165 ns |    165 ns |    4231547 |     ~9.2x |
| BM_NewDelete_Churn        |   1526 ns |   1523 ns |     465199 |      1.0x |
```


The significant performance gain is achieved by avoiding expensive system calls and instead using simple pointer manipulation on a pre-allocated memory pool.

## Building and Running

### Prerequisites

*   **CMake** (version 3.15 or newer)
*   A **C++17 compliant compiler** (e.g., GCC, Clang, MSVC)
*   **Google Benchmark Library** (must be installed in a location findable by CMake)

### Build Steps

1.  Clone the repository:
    ```bash
    git clone https://github.com/kmyria/poolAllocator.git
    cd poolAllocator/customAllocator/
    ```

2.  Configure the project with CMake, building in `Release` mode to enable optimisations.
    ```bash
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    ```

3.  Compile the code:
    ```bash
    cmake --build build
    ```

### Running the executable

The build process generates executables inside the `build/` directory:

*   **`benchmark`**: The performance test suite.

To run the benchmark, execute the following command:
```bash
./build/benchmark
```

### Future direction

The next phase of development will focus on adding support for concurrent environments.

*   Implementing thread safety
*   Benchmark concurrency
*   More advanced designs
