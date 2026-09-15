#include <benchmark/benchmark.h>
#include "matching_engine/matching_engine.hpp"

using namespace matching_engine;

static void BM_MatchingEngine_LimitOrderInsertion(benchmark::State& state) {
    NullEventSink sink;
    MatchingEngine<MapOrderBook> engine(&sink);
    OrderId id = 1;

    for (auto _ : state) {
        engine.submit_order(id, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10000 - (id % 100), 100);
        ++id;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MatchingEngine_LimitOrderInsertion);

static void BM_MatchingEngine_SingleLevelMatching(benchmark::State& state) {
    NullEventSink sink;
    MatchingEngine<MapOrderBook> engine(&sink);
    OrderId id = 1;

    for (auto _ : state) {
        state.PauseTiming();
        engine.submit_order(id++, Side::Sell, OrderType::Limit, ExecutionPolicy::GTC, 10000, 100);
        state.ResumeTiming();

        engine.submit_order(id++, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10000, 100);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MatchingEngine_SingleLevelMatching);

BENCHMARK_MAIN();
