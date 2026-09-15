#include "matching_engine/matching_engine.hpp"
#include "matching_engine/generator.hpp"
#include "matching_engine/replay.hpp"
#include <iostream>

using namespace matching_engine;

int main() {
    std::cout << "--- Synthetic Order Stream Generator & Replay Example ---\n";

    SyntheticOrderGenerator generator;
    auto commands = generator.generate(100);

    std::string sample_file = "sample_stream.csv";
    ReplayEngine::save_csv(sample_file, commands);
    std::cout << "Saved 100 sample orders to " << sample_file << "\n";

    MatchingEngine<MapOrderBook> engine;
    auto loaded = ReplayEngine::load_csv(sample_file);
    size_t processed = ReplayEngine::run_replay(engine, loaded);

    std::cout << "Replayed " << processed << " commands. Trades executed: " << engine.trade_count() << "\n";
    print_snapshot(engine.get_snapshot(), std::cout);

    return 0;
}
