cmake_minimum_required(VERSION 3.15)
project(CustomAllocator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(benchmark CONFIG REQUIRED)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -O3")
endif()

# Create executable
add_executable(testAlloc
    custAlloc/custAlloc.cpp
    testAlloc.cpp
)

add_executable(benchmark
    custAlloc/custAlloc.cpp
    benchmark.cpp
)

# Include directories
target_include_directories(testAlloc PRIVATE .)
target_include_directories(benchmark PRIVATE .)

target_link_libraries(benchmark PRIVATE benchmark::benchmark)

# Optional: Set C++ standard per target
target_compile_features(testAlloc PRIVATE cxx_std_23)
target_compile_features(benchmark PRIVATE cxx_std_23)

# Optional: Debug info
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(testAlloc PRIVATE DEBUG)
    target_compile_definitions(benchmark PRIVATE DEBUG)
endif()
