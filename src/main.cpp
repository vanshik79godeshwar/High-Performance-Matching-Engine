#include "matching_engine/matching_engine.hpp"
#include "matching_engine/replay.hpp"
#include "matching_engine/generator.hpp"
#include "matching_engine/invariant_checker.hpp"
#include <iostream>
#include <string>
#include <chrono>

using namespace matching_engine;

void run_demo() {
    std::cout << "==================================================\n";
    std::cout << "  HIGH-PERFORMANCE MATCHING ENGINE DEMO           \n";
    std::cout << "==================================================\n\n";

    ConsoleEventSink console_sink;
    MatchingEngine<MapOrderBook> engine(&console_sink);

    std::cout << "--- Submitting Initial Bids ---\n";
    engine.submit_order(1, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10050, 100, 1000);
    engine.submit_order(2, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10050, 200, 1001);
    engine.submit_order(3, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10025, 300, 1002);

    std::cout << "\n--- Submitting Initial Asks ---\n";
    engine.submit_order(4, Side::Sell, OrderType::Limit, ExecutionPolicy::GTC, 10100, 150, 1003);
    engine.submit_order(5, Side::Sell, OrderType::Limit, ExecutionPolicy::GTC, 10125, 250, 1004);

    std::cout << "\n--- Current Book Snapshot ---\n";
    print_snapshot(engine.get_snapshot(), std::cout);

    std::cout << "\n--- Submitting Aggressive Limit Buy (Crosses Spread) ---\n";
    engine.submit_order(6, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10100, 100, 1005);

    std::cout << "\n--- Current Book Snapshot After Match ---\n";
    print_snapshot(engine.get_snapshot(), std::cout);

    std::cout << "\n--- Modifying Order 3 (Price 10025 -> 10050, Qty 300) ---\n";
    engine.modify_order(3, 10050, 300, 1006);

    std::cout << "\n--- Cancelling Order 2 ---\n";
    engine.cancel_order(2, 1007);

    std::cout << "\n--- Final Book Snapshot ---\n";
    print_snapshot(engine.get_snapshot(), std::cout);

    std::vector<std::string> errors;
    if (InvariantChecker::validate(engine, errors)) {
        std::cout << "\n[PASS] All 13 Engine Invariants Validated Successfully!\n";
    } else {
        std::cout << "\n[FAIL] Invariant Violations Detected:\n";
        for (const auto& err : errors) std::cout << "  - " << err << "\n";
    }
}

void run_replay(const std::string& csv_file) {
    std::cout << "Loading order stream from: " << csv_file << "...\n";
    auto commands = ReplayEngine::load_csv(csv_file);
    if (commands.empty()) {
        std::cerr << "No commands loaded from CSV file.\n";
        return;
    }
    std::cout << "Loaded " << commands.size() << " order commands.\n";

    VectorEventSink sink;
    MatchingEngine<MapOrderBook> engine(&sink);

    auto start = std::chrono::high_resolution_clock::now();
    size_t processed = ReplayEngine::run_replay(engine, commands);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double duration_ms = duration_ns / 1e6;
    double mops = (processed / (duration_ns / 1e9)) / 1e6;

    std::cout << "\nReplay Completed Successfully!\n";
    std::cout << "Processed Commands : " << processed << "\n";
    std::cout << "Total Trades        : " << engine.trade_count() << "\n";
    std::cout << "Total Elapsed Time  : " << duration_ms << " ms\n";
    std::cout << "Throughput          : " << mops << " Million Ops/sec\n";
    std::cout << "Avg Latency         : " << (duration_ns / static_cast<double>(processed)) << " ns/op\n\n";

    print_snapshot(engine.get_snapshot(5), std::cout);
}

void run_generate(const std::string& csv_file, size_t count) {
    std::cout << "Generating " << count << " synthetic order events to " << csv_file << "...\n";
    SyntheticOrderGenerator generator;
    auto commands = generator.generate(count);
    if (ReplayEngine::save_csv(csv_file, commands)) {
        std::cout << "Successfully generated and saved " << commands.size() << " orders.\n";
    } else {
        std::cerr << "Failed to save CSV file: " << csv_file << "\n";
    }
}

void run_cli_benchmark() {
    std::cout << "Running built-in benchmark workload (1,000,000 orders)...\n";
    SyntheticOrderGenerator generator;
    auto commands = generator.generate(1000000);

    NullEventSink null_sink;
    MatchingEngine<MapOrderBook> map_engine(&null_sink);
    MatchingEngine<FlatOrderBook> flat_engine(&null_sink);

    // Benchmark MapOrderBook (Strategy A)
    auto start_a = std::chrono::high_resolution_clock::now();
    ReplayEngine::run_replay(map_engine, commands);
    auto end_a = std::chrono::high_resolution_clock::now();
    auto ns_a = std::chrono::duration_cast<std::chrono::nanoseconds>(end_a - start_a).count();

    // Benchmark FlatOrderBook (Strategy B)
    auto start_b = std::chrono::high_resolution_clock::now();
    ReplayEngine::run_replay(flat_engine, commands);
    auto end_b = std::chrono::high_resolution_clock::now();
    auto ns_b = std::chrono::duration_cast<std::chrono::nanoseconds>(end_b - start_b).count();

    std::cout << "\n==================================================\n";
    std::cout << "  BENCHMARK RESULTS (1M Orders Workload)          \n";
    std::cout << "==================================================\n";
    std::cout << "Strategy A (MapOrderBook)  : " << (1000000.0 / (ns_a / 1e9) / 1e6)
              << " MOps/sec (" << (ns_a / 1000000.0) << " ns/op)\n";
    std::cout << "Strategy B (FlatOrderBook) : " << (1000000.0 / (ns_b / 1e9) / 1e6)
              << " MOps/sec (" << (ns_b / 1000000.0) << " ns/op)\n";
    std::cout << "==================================================\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        run_demo();
        return 0;
    }

    std::string arg1 = argv[1];
    if (arg1 == "--demo") {
        run_demo();
    } else if (arg1 == "--replay" && argc >= 3) {
        run_replay(argv[2]);
    } else if (arg1 == "--generate" && argc >= 3) {
        size_t count = 10000;
        if (argc >= 5 && std::string(argv[3]) == "--count") {
            count = std::stoull(argv[4]);
        }
        run_generate(argv[2], count);
    } else if (arg1 == "--benchmark") {
        run_cli_benchmark();
    } else {
        std::cout << "Usage:\n";
        std::cout << "  matching_engine --demo\n";
        std::cout << "  matching_engine --replay <orders.csv>\n";
        std::cout << "  matching_engine --generate <orders.csv> [--count 10000]\n";
        std::cout << "  matching_engine --benchmark\n";
    }

    return 0;
}
