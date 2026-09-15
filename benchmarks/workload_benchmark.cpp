#include <benchmark/benchmark.h>
#include "matching_engine/matching_engine.hpp"
#include "matching_engine/generator.hpp"
#include "matching_engine/replay.hpp"

using namespace matching_engine;

static void BM_Workload_SmallBook(benchmark::State& state) {
    SyntheticOrderGenerator generator(GeneratorConfig{.seed = 42, .limit_ratio = 0.80, .cancel_ratio = 0.10});
    auto commands = generator.generate(1000);

    NullEventSink sink;
    for (auto _ : state) {
        MatchingEngine<MapOrderBook> engine(&sink);
        ReplayEngine::run_replay(engine, commands);
    }
    state.SetItemsProcessed(state.iterations() * 1000);
}
BENCHMARK(BM_Workload_SmallBook);

static void BM_Workload_MediumBook(benchmark::State& state) {
    SyntheticOrderGenerator generator(GeneratorConfig{.seed = 100, .limit_ratio = 0.80, .cancel_ratio = 0.10});
    auto commands = generator.generate(10000);

    NullEventSink sink;
    for (auto _ : state) {
        MatchingEngine<MapOrderBook> engine(&sink);
        ReplayEngine::run_replay(engine, commands);
    }
    state.SetItemsProcessed(state.iterations() * 10000);
}
BENCHMARK(BM_Workload_MediumBook);

static void BM_Workload_HeavyCancellation(benchmark::State& state) {
    SyntheticOrderGenerator generator(GeneratorConfig{.seed = 200, .limit_ratio = 0.40, .cancel_ratio = 0.55});
    auto commands = generator.generate(10000);

    NullEventSink sink;
    for (auto _ : state) {
        MatchingEngine<MapOrderBook> engine(&sink);
        ReplayEngine::run_replay(engine, commands);
    }
    state.SetItemsProcessed(state.iterations() * 10000);
}
BENCHMARK(BM_Workload_HeavyCancellation);

static void BM_Workload_HeavyMatching(benchmark::State& state) {
    SyntheticOrderGenerator generator(GeneratorConfig{.seed = 300, .limit_ratio = 0.20, .market_ratio = 0.70, .cancel_ratio = 0.05});
    auto commands = generator.generate(10000);

    NullEventSink sink;
    for (auto _ : state) {
        MatchingEngine<MapOrderBook> engine(&sink);
        ReplayEngine::run_replay(engine, commands);
    }
    state.SetItemsProcessed(state.iterations() * 10000);
}
BENCHMARK(BM_Workload_HeavyMatching);
