#ifndef MATCHING_ENGINE_ORDER_HPP
#define MATCHING_ENGINE_ORDER_HPP

#include <cstdint>
#include <string_view>
#include <ostream>

namespace matching_engine {

using Price = int64_t;
using Quantity = uint64_t;
using OrderId = uint64_t;
using SequenceId = uint64_t;
using Timestamp = uint64_t;

enum class Side : uint8_t {
    Buy = 0,
    Sell = 1
};

enum class OrderType : uint8_t {
    Limit = 0,
    Market = 1
};

enum class ExecutionPolicy : uint8_t {
    GTC = 0, // Good-Til-Cancelled
    IOC = 1, // Immediate-Or-Cancel
    FOK = 2  // Fill-Or-Kill
};

struct Order {
    OrderId id{0};
    Side side{Side::Buy};
    OrderType type{OrderType::Limit};
    ExecutionPolicy policy{ExecutionPolicy::GTC};
    Price price{0};
    Quantity initial_qty{0};
    Quantity remaining_qty{0};
    SequenceId sequence_id{0};
    Timestamp timestamp{0};

    // Intrusive pointers for zero-allocation FIFO price level queues
    Order* prev{nullptr};
    Order* next{nullptr};

    [[nodiscard]] constexpr bool is_filled() const noexcept {
        return remaining_qty == 0;
    }

    [[nodiscard]] constexpr bool is_active() const noexcept {
        return remaining_qty > 0;
    }

    [[nodiscard]] constexpr Quantity filled_qty() const noexcept {
        return initial_qty - remaining_qty;
    }
};

inline constexpr std::string_view to_string(Side side) noexcept {
    switch (side) {
        case Side::Buy: return "BUY";
        case Side::Sell: return "SELL";
    }
    return "UNKNOWN";
}

inline constexpr std::string_view to_string(OrderType type) noexcept {
    switch (type) {
        case OrderType::Limit: return "LIMIT";
        case OrderType::Market: return "MARKET";
    }
    return "UNKNOWN";
}

inline constexpr std::string_view to_string(ExecutionPolicy policy) noexcept {
    switch (policy) {
        case ExecutionPolicy::GTC: return "GTC";
        case ExecutionPolicy::IOC: return "IOC";
        case ExecutionPolicy::FOK: return "FOK";
    }
    return "UNKNOWN";
}

inline std::ostream& operator<<(std::ostream& os, Side side) {
    return os << to_string(side);
}

inline std::ostream& operator<<(std::ostream& os, OrderType type) {
    return os << to_string(type);
}

inline std::ostream& operator<<(std::ostream& os, ExecutionPolicy policy) {
    return os << to_string(policy);
}

} // namespace matching_engine

#endif // MATCHING_ENGINE_ORDER_HPP
