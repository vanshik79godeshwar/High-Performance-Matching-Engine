#include <benchmark/benchmark.h>
#include "matching_engine/matching_engine.hpp"

using namespace matching_engine;

static void BM_StrategyA_MapOrderBook_MixedWorkload(benchmark::State& state) {
    NullEventSink sink;
    MatchingEngine<MapOrderBook> engine(&sink);
    OrderId id = 1;

    for (auto _ : state) {
        engine.submit_order(id, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10000 - (id % 50), 100);
        engine.submit_order(id + 1, Side::Sell, OrderType::Limit, ExecutionPolicy::GTC, 10000 + (id % 50), 100);
        id += 2;
    }
    state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_StrategyA_MapOrderBook_MixedWorkload);

static void BM_StrategyB_FlatOrderBook_MixedWorkload(benchmark::State& state) {
    NullEventSink sink;
    MatchingEngine<FlatOrderBook> engine(&sink);
    OrderId id = 1;

    for (auto _ : state) {
        engine.submit_order(id, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10000 - (id % 50), 100);
        engine.submit_order(id + 1, Side::Sell, OrderType::Limit, ExecutionPolicy::GTC, 10000 + (id % 50), 100);
        id += 2;
    }
    state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_StrategyB_FlatOrderBook_MixedWorkload);
