cmake_minimum_required(VERSION 3.20)
project(HighPerformanceMatchingEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler optimization and warning flags
if(MSVC)
    add_compile_options(/O2 /W4 /WX /permissive-)
else()
    add_compile_options(-O3 -Wall -Wextra -Werror -Wpedantic -march=native)
endif()

# Include directories
include_directories(include)

# Core Library Target
add_library(matching_engine_lib STATIC
    src/order_book.cpp
    src/matching_engine.cpp
    src/replay.cpp
    src/generator.cpp
)

# CLI Main Executable Target
add_executable(matching_engine src/main.cpp)
target_link_libraries(matching_engine PRIVATE matching_engine_lib)

# Examples
add_executable(basic_exchange examples/basic_exchange.cpp)
target_link_libraries(basic_exchange PRIVATE matching_engine_lib)

add_executable(replay_example examples/replay_example.cpp)
target_link_libraries(replay_example PRIVATE matching_engine_lib)

# Testing (GoogleTest)
enable_testing()
find_package(GTest REQUIRED)

set(TEST_SOURCES
    tests/order_book_tests.cpp
    tests/matching_engine_tests.cpp
    tests/order_management_tests.cpp
    tests/invariant_tests.cpp
    tests/randomized_tests.cpp
    tests/reference_engine_tests.cpp
)

foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE matching_engine_lib GTest::gtest_main)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# Benchmarking (Google Benchmark)
find_package(benchmark REQUIRED)

set(BENCHMARK_SOURCES
    benchmarks/matching_benchmark.cpp
    benchmarks/order_book_benchmark.cpp
    benchmarks/workload_benchmark.cpp
)

foreach(bench_src ${BENCHMARK_SOURCES})
    get_filename_component(bench_name ${bench_src} NAME_WE)
    add_executable(${bench_name} ${bench_src})
    if(TARGET benchmark::benchmark_main)
        target_link_libraries(${bench_name} PRIVATE matching_engine_lib benchmark::benchmark_main)
    else()
        target_link_libraries(${bench_name} PRIVATE matching_engine_lib benchmark::benchmark)
    endif()
endforeach()
