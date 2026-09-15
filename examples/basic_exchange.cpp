#include "matching_engine/matching_engine.hpp"
#include <iostream>

using namespace matching_engine;

int main() {
    std::cout << "--- Basic Exchange Example ---\n";

    ConsoleEventSink sink;
    MatchingEngine<MapOrderBook> engine(&sink);

    std::cout << "\nSubmitting Bids and Asks:\n";
    engine.submit_order(101, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 5000, 100);
    engine.submit_order(102, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 5010, 200);
    engine.submit_order(103, Side::Sell, OrderType::Limit, ExecutionPolicy::GTC, 5020, 150);

    std::cout << "\nMatching Market Order:\n";
    engine.submit_order(104, Side::Buy, OrderType::Market, ExecutionPolicy::IOC, 0, 100);

    std::cout << "\nFinal Book:\n";
    print_snapshot(engine.get_snapshot(), std::cout);

    return 0;
}
