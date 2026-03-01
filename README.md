# High Performance C++ Pool Allocator

A minimal but highly efficient fixed-size pool allocator written in modern C++. This project was created to explore and demonstrate the significant performance advantages of custom memory management strategies over standard library allocators (`new`/`delete`) in performance-critical applications.

The core focus is pure speed, achieved by eliminating operating system overhead and leveraging a simple, cache-friendly free-list design. The allocator's performance is validated using the Google Benchmark library.

### Performance Benchmark

[Read about the benchmark analysis and design choices](docs/BENCHMARK.md).

### Key Features

*   **Fast:** **3x–22x** faster than standard `new/delete`, depending on workload; with a peak of ~22× in [high‑churn](docs/BENCHMARK.md), multithreaded scenarios
*   **Lock-Free Concurrency:** Replaces expensive OS-level mutex locks (`std::mutex`) with a highly optimized `std::atomic` Compare-And-Swap (CAS) global free-list. Uses packed tagged pointers to simultaneously prevent ABA problem and enforce lock-free single word CAS.
*   **Thread-Local Caching (TLS):** Mitigates hardware cache-line contention by providing each thread with a private, lock-free local cache, amortizing the cost of global atomic operations via batching.
*   **Modern C++:** Built with C++17/20 features for compile time optimisations.
*   **Professionally Benchmarked:** Performance and concurrency scaling is proven with a comprehensive suite of tests using Google Benchmark.
*   **Standard Build System:** Uses CMake for convenient, cross platform building and integration.

## Building and Running

### Prerequisites

*   **CMake** (3.15+)
*   **C++23 compliant compiler**
*   **Google Benchmark Library**

### Build

1.  Clone the repository:
    ```bash
    git clone https://github.com/kmyria/poolAllocator.git
    cd poolAllocator/customAllocator/
    ```

2.  Compile and build:
    ```bash
    cmake --fresh -B build && cmake --build build
    ```


### Running the executable

The build process generates executables inside the `build/` directory:

*   **`benchmark`**: The performance test suite.

To run the benchmark, execute the following command:
```bash
./build/benchmark
```
Remember to disable CPU frequency scaling for the most acccurate results.

