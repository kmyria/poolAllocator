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

Read about the benchmark analysis and design choices [here](docs/BENCHMARK.md).

## Building and Running

### Prerequisites

*   **CMake** (version 3.15 or newer)
*   A **C++23 compliant compiler** (e.g. gcc, clang, msvc)
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

Remember to disable CPU frequency scaling for the most acccurate results.

### Future direction

The next phase of development will focus on adding support for concurrent environments.

*   Implementing thread safety
*   Benchmark concurrency
*   More advanced designs
