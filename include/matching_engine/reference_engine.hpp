#ifndef MATCHING_ENGINE_REFERENCE_ENGINE_HPP
#define MATCHING_ENGINE_REFERENCE_ENGINE_HPP

#include "order.hpp"
#include <vector>
#include <algorithm>
#include <cassert>

namespace matching_engine {

struct RefTrade {
    OrderId aggressor_id;
    OrderId resting_id;
    Side aggressor_side;
    Price price;
    Quantity quantity;
};

class ReferenceEngine {
public:
    ReferenceEngine() = default;

    struct RefOrder {
        OrderId id;
        Side side;
        OrderType type;
        ExecutionPolicy policy;
        Price price;
        Quantity initial_qty;
        Quantity remaining_qty;
        uint64_t sequence_id;
    };

    bool submit_order(OrderId id, Side side, OrderType type, ExecutionPolicy policy,
                      Price price, Quantity qty, uint64_t seq_id = 0) {
        if (id == 0 || qty == 0) return false;
        if (type == OrderType::Limit && price <= 0) return false;
        for (const auto& o : orders_) {
            if (o.id == id) return false;
        }

        if (policy == ExecutionPolicy::FOK) {
            Quantity avail = 0;
            if (side == Side::Buy) {
                for (const auto& o : orders_) {
                    if (o.side == Side::Sell && (price == 0 || o.price <= price)) {
                        avail += o.remaining_qty;
                    }
                }
            } else {
                for (const auto& o : orders_) {
                    if (o.side == Side::Buy && (price == 0 || o.price >= price)) {
                        avail += o.remaining_qty;
                    }
                }
            }
            if (avail < qty) return false;
        }

        RefOrder order{id, side, type, policy, price, qty, qty, seq_id};

        if (side == Side::Buy) {
            match_buy(order);
        } else {
            match_sell(order);
        }

        return true;
    }

    bool cancel_order(OrderId id) {
        auto it = std::find_if(orders_.begin(), orders_.end(), [id](const RefOrder& o) { return o.id == id; });
        if (it != orders_.end()) {
            orders_.erase(it);
            return true;
        }
        return false;
    }

    bool modify_order(OrderId id, Price new_price, Quantity new_qty, uint64_t seq_id = 0) {
        auto it = std::find_if(orders_.begin(), orders_.end(), [id](const RefOrder& o) { return o.id == id; });
        if (it == orders_.end()) return false;

        if (new_qty == 0) {
            orders_.erase(it);
            return true;
        }

        RefOrder old_order = *it;
        orders_.erase(it);

        if (new_price == old_order.price && new_qty <= old_order.remaining_qty) {
            // Keep priority, modify quantity
            RefOrder mod_order = old_order;
            mod_order.remaining_qty = new_qty;
            mod_order.initial_qty -= (old_order.remaining_qty - new_qty);
            orders_.insert(it, mod_order);
            return true;
        } else {
            return submit_order(id, old_order.side, old_order.type, old_order.policy, new_price, new_qty, seq_id);
        }
    }

    [[nodiscard]] const std::vector<RefTrade>& trades() const noexcept { return trades_; }
    [[nodiscard]] const std::vector<RefOrder>& orders() const noexcept { return orders_; }
    void clear_trades() noexcept { trades_.clear(); }
    void clear() noexcept { orders_.clear(); trades_.clear(); }

private:
    std::vector<RefOrder> orders_;
    std::vector<RefTrade> trades_;

    void match_buy(RefOrder& order) {
        while (order.remaining_qty > 0) {
            // Find best ask (lowest price, earliest seq)
            auto best_ask_it = orders_.end();
            for (auto it = orders_.begin(); it != orders_.end(); ++it) {
                if (it->side == Side::Sell && (order.type == OrderType::Market || it->price <= order.price)) {
                    if (best_ask_it == orders_.end() || it->price < best_ask_it->price ||
                        (it->price == best_ask_it->price && it->sequence_id < best_ask_it->sequence_id)) {
                        best_ask_it = it;
                    }
                }
            }

            if (best_ask_it == orders_.end()) break;

            Quantity trade_qty = std::min(order.remaining_qty, best_ask_it->remaining_qty);
            Price exec_price = best_ask_it->price;

            order.remaining_qty -= trade_qty;
            best_ask_it->remaining_qty -= trade_qty;

            trades_.push_back({order.id, best_ask_it->id, Side::Buy, exec_price, trade_qty});

            if (best_ask_it->remaining_qty == 0) {
                orders_.erase(best_ask_it);
            }
        }

        if (order.remaining_qty > 0 && order.type == OrderType::Limit && order.policy == ExecutionPolicy::GTC) {
            orders_.push_back(order);
        }
    }

    void match_sell(RefOrder& order) {
        while (order.remaining_qty > 0) {
            // Find best bid (highest price, earliest seq)
            auto best_bid_it = orders_.end();
            for (auto it = orders_.begin(); it != orders_.end(); ++it) {
                if (it->side == Side::Buy && (order.type == OrderType::Market || it->price >= order.price)) {
                    if (best_bid_it == orders_.end() || it->price > best_bid_it->price ||
                        (it->price == best_bid_it->price && it->sequence_id < best_bid_it->sequence_id)) {
                        best_bid_it = it;
                    }
                }
            }

            if (best_bid_it == orders_.end()) break;

            Quantity trade_qty = std::min(order.remaining_qty, best_bid_it->remaining_qty);
            Price exec_price = best_bid_it->price;

            order.remaining_qty -= trade_qty;
            best_bid_it->remaining_qty -= trade_qty;

            trades_.push_back({order.id, best_bid_it->id, Side::Sell, exec_price, trade_qty});

            if (best_bid_it->remaining_qty == 0) {
                orders_.erase(best_bid_it);
            }
        }

        if (order.remaining_qty > 0 && order.type == OrderType::Limit && order.policy == ExecutionPolicy::GTC) {
            orders_.push_back(order);
        }
    }
};

} // namespace matching_engine

#endif // MATCHING_ENGINE_REFERENCE_ENGINE_HPP
