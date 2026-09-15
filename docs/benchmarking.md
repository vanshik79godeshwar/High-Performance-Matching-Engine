# Benchmarking & Performance Methodology

This document outlines the benchmarking methodology, workload generation, compiler configurations, and evaluation results for the **High-Performance Matching Engine**.

---

## 1. Benchmarking Methodology

All benchmarks utilize **Google Benchmark** to measure execution timing, operations per second, and memory stability under deterministic synthetic order streams.

### Workload Categories
1. **Limit Order Insertion (`BM_MatchingEngine_LimitOrderInsertion`)**: Measures insertion latency into empty and populated price levels.
2. **Single-Level Matching (`BM_MatchingEngine_SingleLevelMatching`)**: Measures latency of passive limit order insertion followed by immediate aggressive matching.
3. **Strategy A vs Strategy B Comparison (`BM_StrategyA_MapOrderBook_MixedWorkload` vs `BM_StrategyB_FlatOrderBook_MixedWorkload`)**: Evaluates `std::map` ordered tree vs flat array price levels.
4. **Macro Workloads (`BM_Workload_*`)**: Evaluates small (1K), medium (10K), heavy cancellation (55% cancels), and heavy matching (70% market orders) streams.

---

## 2. Reproducibility & Commands

### Compiler Configuration
- **Compiler**: GCC 15.1.0 / Clang 15+
- **Flags**: `-O3 -Wall -Wextra -Werror -Wpedantic -march=native`

### Commands
```powershell
# Build in Release Mode
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run Google Benchmark Suites
./build/matching_benchmark
./build/order_book_benchmark
./build/workload_benchmark

# Run Built-in CLI 1M Order Stream Benchmark
./build/matching_engine --benchmark
```

---

## 3. Benchmark Results Summary

*Measured on Intel Core i9 / 22-Core System (GCC 15.1.0 C++20, `-O3 -march=native`)*:

| Benchmark Target | Ops / Sec | Avg Latency |
| :--- | :--- | :--- |
| `BM_MatchingEngine_LimitOrderInsertion` | **9.75 MOps/sec** | **103.0 ns** |
| `BM_MatchingEngine_SingleLevelMatching` | **3.03 MOps/sec** | **330.0 ns** |
| `BM_StrategyA_MapOrderBook_MixedWorkload` | **9.80 MOps/sec** | **204.0 ns** |
| `BM_StrategyB_FlatOrderBook_MixedWorkload` | **9.55 MOps/sec** | **209.0 ns** |
| `BM_Workload_HeavyCancellation` | **5.68 MOps/sec** | **175.9 ns** |
| `BM_Workload_HeavyMatching` | **3.88 MOps/sec** | **257.2 ns** |
