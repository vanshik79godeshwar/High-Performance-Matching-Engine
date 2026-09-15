#ifndef MATCHING_ENGINE_MATCHING_ENGINE_HPP
#define MATCHING_ENGINE_MATCHING_ENGINE_HPP

#include "order.hpp"
#include "order_pool.hpp"
#include "order_index.hpp"
#include "order_book.hpp"
#include "events.hpp"
#include "book_snapshot.hpp"
#include <memory>
#include <string>

namespace matching_engine {

template <typename BookImpl = MapOrderBook>
class MatchingEngine {
public:
    explicit MatchingEngine(IEventSink* event_sink = nullptr, size_t pool_capacity = 65536)
        : event_sink_(event_sink ? event_sink : &default_null_sink_),
          order_pool_(pool_capacity),
          order_index_(pool_capacity) {}

    ~MatchingEngine() = default;

    MatchingEngine(const MatchingEngine&) = delete;
    MatchingEngine& operator=(const MatchingEngine&) = delete;

    void set_event_sink(IEventSink* sink) noexcept {
        event_sink_ = sink ? sink : &default_null_sink_;
    }

    [[nodiscard]] IEventSink* event_sink() const noexcept {
        return event_sink_;
    }

    // Submit a new order
    bool submit_order(OrderId id, Side side, OrderType type, ExecutionPolicy policy,
                      Price price, Quantity qty, Timestamp ts = 0);

    // Cancel an existing order by ID
    bool cancel_order(OrderId id, Timestamp ts = 0);

    // Modify price and/or quantity of an existing order
    bool modify_order(OrderId id, Price new_price, Quantity new_qty, Timestamp ts = 0);

    [[nodiscard]] BookSnapshot get_snapshot(size_t max_depth = 10) const {
        return book_.get_snapshot(max_depth);
    }

    [[nodiscard]] const BookImpl& book() const noexcept { return book_; }
    [[nodiscard]] BookImpl& book() noexcept { return book_; }
    [[nodiscard]] const OrderIndex& order_index() const noexcept { return order_index_; }
    [[nodiscard]] const OrderPool& order_pool() const noexcept { return order_pool_; }

    [[nodiscard]] SequenceId current_sequence_id() const noexcept { return next_sequence_id_ - 1; }
    [[nodiscard]] uint64_t trade_count() const noexcept { return trade_count_; }

    void clear() noexcept {
        order_index_.clear();
        order_pool_.clear();
        book_.clear();
        next_sequence_id_ = 1;
        trade_count_ = 0;
    }

private:
    IEventSink* event_sink_{nullptr};
    NullEventSink default_null_sink_;
    OrderPool order_pool_;
    OrderIndex order_index_;
    BookImpl book_;
    SequenceId next_sequence_id_{1};
    uint64_t trade_count_{0};

    SequenceId next_sequence() noexcept { return next_sequence_id_++; }

    bool match_buy_order(Order* order, Timestamp ts);
    bool match_sell_order(Order* order, Timestamp ts);
    bool check_fok_liquidity(Side side, Price price, Quantity qty) const;
    void notify_book_update(Timestamp ts);
};

template <typename BookImpl>
bool MatchingEngine<BookImpl>::check_fok_liquidity(Side side, Price price, Quantity qty) const {
    Quantity available = 0;
    if (side == Side::Buy) {
        if constexpr (std::is_same_v<BookImpl, MapOrderBook>) {
            for (const auto& [ask_price, level] : book_.asks()) {
                if (price != 0 && ask_price > price) break;
                available += level.total_quantity();
                if (available >= qty) return true;
            }
        } else {
            const PriceLevel* level = book_.get_best_ask_level();
            if (level && (price == 0 || level->price() <= price)) {
                available += level->total_quantity();
            }
        }
    } else {
        if constexpr (std::is_same_v<BookImpl, MapOrderBook>) {
            for (const auto& [bid_price, level] : book_.bids()) {
                if (price != 0 && bid_price < price) break;
                available += level.total_quantity();
                if (available >= qty) return true;
            }
        } else {
            const PriceLevel* level = book_.get_best_bid_level();
            if (level && (price == 0 || level->price() >= price)) {
                available += level->total_quantity();
            }
        }
    }
    return available >= qty;
}

template <typename BookImpl>
bool MatchingEngine<BookImpl>::submit_order(OrderId id, Side side, OrderType type, ExecutionPolicy policy,
                                            Price price, Quantity qty, Timestamp ts) {
    SequenceId seq = next_sequence();

    if (id == 0) {
        event_sink_->on_order_rejected({seq, ts, id, "Invalid OrderId 0"});
        return false;
    }
    if (qty == 0) {
        event_sink_->on_order_rejected({seq, ts, id, "Quantity must be greater than 0"});
        return false;
    }
    if (type == OrderType::Limit && price <= 0) {
        event_sink_->on_order_rejected({seq, ts, id, "Limit order price must be positive"});
        return false;
    }
    if (order_index_.contains(id)) {
        event_sink_->on_order_rejected({seq, ts, id, "Duplicate OrderId"});
        return false;
    }

    event_sink_->on_order_accepted({seq, ts, id, side, type, policy, price, qty});

    if (policy == ExecutionPolicy::FOK) {
        if (!check_fok_liquidity(side, price, qty)) {
            event_sink_->on_order_cancelled({seq, ts, id, side, price, qty});
            return false;
        }
    }

    Order* order = order_pool_.allocate(id, side, type, policy, price, qty, seq, ts);

    if (side == Side::Buy) {
        match_buy_order(order, ts);
    } else {
        match_sell_order(order, ts);
    }

    notify_book_update(ts);
    return true;
}

template <typename BookImpl>
bool MatchingEngine<BookImpl>::match_buy_order(Order* order, Timestamp ts) {
    while (order->remaining_qty > 0) {
        PriceLevel* best_ask = book_.get_best_ask_level();
        if (!best_ask) break;

        if (order->type == OrderType::Limit && best_ask->price() > order->price) {
            break;
        }

        Order* resting = best_ask->front();
        assert(resting != nullptr);

        Quantity trade_qty = std::min(order->remaining_qty, resting->remaining_qty);
        Price exec_price = resting->price;
        uint64_t trade_id = ++trade_count_;
        SequenceId trade_seq = next_sequence();

        order->remaining_qty -= trade_qty;
        book_.decrease_order_quantity(resting, trade_qty);

        event_sink_->on_trade_executed({
            trade_seq, ts, trade_id, order->id, resting->id, Side::Buy,
            exec_price, trade_qty, order->remaining_qty, resting->remaining_qty
        });

        if (resting->is_filled()) {
            event_sink_->on_order_filled({trade_seq, ts, resting->id, Side::Sell, exec_price, resting->initial_qty});
            book_.remove_order(resting);
            order_index_.erase(resting->id);
            order_pool_.deallocate(resting);
        } else {
            event_sink_->on_order_partially_filled({trade_seq, ts, resting->id, Side::Sell, exec_price, trade_qty, resting->remaining_qty});
        }

        if (order->remaining_qty > 0) {
            event_sink_->on_order_partially_filled({trade_seq, ts, order->id, Side::Buy, exec_price, trade_qty, order->remaining_qty});
        } else {
            event_sink_->on_order_filled({trade_seq, ts, order->id, Side::Buy, exec_price, order->initial_qty});
        }
    }

    if (order->remaining_qty > 0) {
        if (order->type == OrderType::Market || order->policy == ExecutionPolicy::IOC || order->policy == ExecutionPolicy::FOK) {
            SequenceId cancel_seq = next_sequence();
            event_sink_->on_order_cancelled({cancel_seq, ts, order->id, Side::Buy, order->price, order->remaining_qty});
            order_pool_.deallocate(order);
        } else {
            SequenceId add_seq = next_sequence();
            book_.add_order(order);
            order_index_.insert(order->id, order);
            event_sink_->on_order_added({add_seq, ts, order->id, Side::Buy, order->price, order->remaining_qty});
        }
    } else {
        if (order->initial_qty == 0) {
            order_pool_.deallocate(order);
        }
    }
    return true;
}

template <typename BookImpl>
bool MatchingEngine<BookImpl>::match_sell_order(Order* order, Timestamp ts) {
    while (order->remaining_qty > 0) {
        PriceLevel* best_bid = book_.get_best_bid_level();
        if (!best_bid) break;

        if (order->type == OrderType::Limit && best_bid->price() < order->price) {
            break;
        }

        Order* resting = best_bid->front();
        assert(resting != nullptr);

        Quantity trade_qty = std::min(order->remaining_qty, resting->remaining_qty);
        Price exec_price = resting->price;
        uint64_t trade_id = ++trade_count_;
        SequenceId trade_seq = next_sequence();

        order->remaining_qty -= trade_qty;
        book_.decrease_order_quantity(resting, trade_qty);

        event_sink_->on_trade_executed({
            trade_seq, ts, trade_id, order->id, resting->id, Side::Sell,
            exec_price, trade_qty, order->remaining_qty, resting->remaining_qty
        });

        if (resting->is_filled()) {
            event_sink_->on_order_filled({trade_seq, ts, resting->id, Side::Buy, exec_price, resting->initial_qty});
            book_.remove_order(resting);
            order_index_.erase(resting->id);
            order_pool_.deallocate(resting);
        } else {
            event_sink_->on_order_partially_filled({trade_seq, ts, resting->id, Side::Buy, exec_price, trade_qty, resting->remaining_qty});
        }

        if (order->remaining_qty > 0) {
            event_sink_->on_order_partially_filled({trade_seq, ts, order->id, Side::Sell, exec_price, trade_qty, order->remaining_qty});
        } else {
            event_sink_->on_order_filled({trade_seq, ts, order->id, Side::Sell, exec_price, order->initial_qty});
        }
    }

    if (order->remaining_qty > 0) {
        if (order->type == OrderType::Market || order->policy == ExecutionPolicy::IOC || order->policy == ExecutionPolicy::FOK) {
            SequenceId cancel_seq = next_sequence();
            event_sink_->on_order_cancelled({cancel_seq, ts, order->id, Side::Sell, order->price, order->remaining_qty});
            order_pool_.deallocate(order);
        } else {
            SequenceId add_seq = next_sequence();
            book_.add_order(order);
            order_index_.insert(order->id, order);
            event_sink_->on_order_added({add_seq, ts, order->id, Side::Sell, order->price, order->remaining_qty});
        }
    } else {
        if (order->initial_qty == 0) {
            order_pool_.deallocate(order);
        }
    }
    return true;
}

template <typename BookImpl>
bool MatchingEngine<BookImpl>::cancel_order(OrderId id, Timestamp ts) {
    SequenceId seq = next_sequence();

    Order* order = order_index_.find(id);
    if (!order) {
        event_sink_->on_order_rejected({seq, ts, id, "Order not found for cancellation"});
        return false;
    }

    Quantity cancelled_qty = order->remaining_qty;
    Price price = order->price;
    Side side = order->side;

    book_.remove_order(order);
    order_index_.erase(id);
    order_pool_.deallocate(order);

    event_sink_->on_order_cancelled({seq, ts, id, side, price, cancelled_qty});
    notify_book_update(ts);
    return true;
}

template <typename BookImpl>
bool MatchingEngine<BookImpl>::modify_order(OrderId id, Price new_price, Quantity new_qty, Timestamp ts) {
    SequenceId seq = next_sequence();

    Order* order = order_index_.find(id);
    if (!order) {
        event_sink_->on_order_rejected({seq, ts, id, "Order not found for modification"});
        return false;
    }

    if (new_qty == 0) {
        return cancel_order(id, ts);
    }

    if (new_price <= 0 && order->type == OrderType::Limit) {
        event_sink_->on_order_rejected({seq, ts, id, "Invalid modification price <= 0"});
        return false;
    }

    Side side = order->side;
    OrderType type = order->type;
    ExecutionPolicy policy = order->policy;

    if (new_price != order->price) {
        book_.remove_order(order);
        order_index_.erase(id);
        order_pool_.deallocate(order);

        event_sink_->on_order_modified({seq, ts, id, new_price, new_qty, false});
        return submit_order(id, side, type, policy, new_price, new_qty, ts);
    } else {
        if (new_qty <= order->remaining_qty) {
            Quantity delta = order->remaining_qty - new_qty;
            book_.decrease_order_quantity(order, delta);
            event_sink_->on_order_modified({seq, ts, id, new_price, new_qty, true});
            notify_book_update(ts);
            return true;
        } else {
            book_.remove_order(order);
            order_index_.erase(id);
            order_pool_.deallocate(order);

            event_sink_->on_order_modified({seq, ts, id, new_price, new_qty, false});
            return submit_order(id, side, type, policy, new_price, new_qty, ts);
        }
    }
}

template <typename BookImpl>
void MatchingEngine<BookImpl>::notify_book_update(Timestamp ts) {
    Price best_bid = 0;
    Quantity best_bid_qty = 0;
    if (const PriceLevel* bid_lvl = book_.get_best_bid_level()) {
        best_bid = bid_lvl->price();
        best_bid_qty = bid_lvl->total_quantity();
    }

    Price best_ask = 0;
    Quantity best_ask_qty = 0;
    if (const PriceLevel* ask_lvl = book_.get_best_ask_level()) {
        best_ask = ask_lvl->price();
        best_ask_qty = ask_lvl->total_quantity();
    }

    event_sink_->on_book_updated({
        current_sequence_id(), ts, best_bid, best_bid_qty, best_ask, best_ask_qty
    });
}

} // namespace matching_engine

#endif // MATCHING_ENGINE_MATCHING_ENGINE_HPP
