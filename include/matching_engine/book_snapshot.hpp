#ifndef MATCHING_ENGINE_BOOK_SNAPSHOT_HPP
#define MATCHING_ENGINE_BOOK_SNAPSHOT_HPP

#include "order.hpp"
#include <vector>
#include <optional>
#include <ostream>
#include <iomanip>

namespace matching_engine {

struct LevelInfo {
    Price price{0};
    Quantity total_quantity{0};
    size_t order_count{0};
};

struct BookSnapshot {
    std::optional<Price> best_bid;
    Quantity best_bid_qty{0};
    std::optional<Price> best_ask;
    Quantity best_ask_qty{0};

    Quantity total_bid_qty{0};
    Quantity total_ask_qty{0};
    size_t total_bid_orders{0};
    size_t total_ask_orders{0};

    std::vector<LevelInfo> bids; // Sorted descending
    std::vector<LevelInfo> asks; // Sorted ascending

    [[nodiscard]] std::optional<Price> spread() const noexcept {
        if (best_bid && best_ask) {
            return *best_ask - *best_bid;
        }
        return std::nullopt;
    }
};

inline void print_snapshot(const BookSnapshot& snapshot, std::ostream& os) {
    os << "========================================\n";
    os << "          LIMIT ORDER BOOK              \n";
    os << "========================================\n";
    os << "ASK SIDE (Total Vol: " << snapshot.total_ask_qty
       << ", Orders: " << snapshot.total_ask_orders << ")\n";

    // Print asks in descending order so highest ask is at top, lowest ask near spread
    if (snapshot.asks.empty()) {
        os << "  <EMPTY>\n";
    } else {
        for (auto it = snapshot.asks.rbegin(); it != snapshot.asks.rend(); ++it) {
            os << "  " << std::setw(10) << it->price << " x "
               << std::setw(8) << it->total_quantity << " (" << it->order_count << " orders)\n";
        }
    }

    os << "----------------------------------------\n";
    if (auto s = snapshot.spread(); s.has_value()) {
        os << "SPREAD: " << *s << "\n";
    } else {
        os << "SPREAD: N/A\n";
    }
    os << "----------------------------------------\n";

    os << "BID SIDE (Total Vol: " << snapshot.total_bid_qty
       << ", Orders: " << snapshot.total_bid_orders << ")\n";
    if (snapshot.bids.empty()) {
        os << "  <EMPTY>\n";
    } else {
        for (const auto& level : snapshot.bids) {
            os << "  " << std::setw(10) << level.price << " x "
               << std::setw(8) << level.total_quantity << " (" << level.order_count << " orders)\n";
        }
    }
    os << "========================================\n";
}

} // namespace matching_engine

#endif // MATCHING_ENGINE_BOOK_SNAPSHOT_HPP
