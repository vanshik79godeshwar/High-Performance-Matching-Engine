#ifndef MATCHING_ENGINE_EVENTS_HPP
#define MATCHING_ENGINE_EVENTS_HPP

#include "order.hpp"
#include <vector>
#include <memory>
#include <string>
#include <variant>
#include <iostream>

namespace matching_engine {

struct OrderAcceptedEvent {
    SequenceId sequence_id;
    Timestamp timestamp;
    OrderId order_id;
    Side side;
    OrderType type;
    ExecutionPolicy policy;
    Price price;
    Quantity quantity;
};

struct OrderRejectedEvent {
    SequenceId sequence_id;
    Timestamp timestamp;
    OrderId order_id;
    std::string reason;
};

struct OrderAddedEvent {
    SequenceId sequence_id;
    Timestamp timestamp;
    OrderId order_id;
    Side side;
    Price price;
    Quantity remaining_qty;
};

struct OrderCancelledEvent {
    SequenceId sequence_id;
    Timestamp timestamp;
    OrderId order_id;
    Side side;
    Price price;
    Quantity cancelled_qty;
};

struct OrderModifiedEvent {
    SequenceId sequence_id;
    Timestamp timestamp;
    OrderId order_id;
    Price new_price;
    Quantity new_qty;
    bool priority_retained;
};

struct TradeExecutedEvent {
    SequenceId sequence_id;
    Timestamp timestamp;
    uint64_t trade_id;
    OrderId aggressor_id;
    OrderId resting_id;
    Side aggressor_side;
    Price exec_price;
    Quantity exec_qty;
    Quantity aggressor_remaining;
    Quantity resting_remaining;
};

struct OrderFilledEvent {
    SequenceId sequence_id;
    Timestamp timestamp;
    OrderId order_id;
    Side side;
    Price price;
    Quantity total_qty;
};

struct OrderPartiallyFilledEvent {
    SequenceId sequence_id;
    Timestamp timestamp;
    OrderId order_id;
    Side side;
    Price exec_price;
    Quantity exec_qty;
    Quantity remaining_qty;
};

struct BookUpdatedEvent {
    SequenceId sequence_id;
    Timestamp timestamp;
    Price best_bid;
    Quantity best_bid_qty;
    Price best_ask;
    Quantity best_ask_qty;
};

using EngineEvent = std::variant<
    OrderAcceptedEvent,
    OrderRejectedEvent,
    OrderAddedEvent,
    OrderCancelledEvent,
    OrderModifiedEvent,
    TradeExecutedEvent,
    OrderFilledEvent,
    OrderPartiallyFilledEvent,
    BookUpdatedEvent
>;

class IEventSink {
public:
    virtual ~IEventSink() = default;

    virtual void on_order_accepted(const OrderAcceptedEvent& event) = 0;
    virtual void on_order_rejected(const OrderRejectedEvent& event) = 0;
    virtual void on_order_added(const OrderAddedEvent& event) = 0;
    virtual void on_order_cancelled(const OrderCancelledEvent& event) = 0;
    virtual void on_order_modified(const OrderModifiedEvent& event) = 0;
    virtual void on_trade_executed(const TradeExecutedEvent& event) = 0;
    virtual void on_order_filled(const OrderFilledEvent& event) = 0;
    virtual void on_order_partially_filled(const OrderPartiallyFilledEvent& event) = 0;
    virtual void on_book_updated(const BookUpdatedEvent& event) = 0;
};

class NullEventSink final : public IEventSink {
public:
    void on_order_accepted(const OrderAcceptedEvent&) override {}
    void on_order_rejected(const OrderRejectedEvent&) override {}
    void on_order_added(const OrderAddedEvent&) override {}
    void on_order_cancelled(const OrderCancelledEvent&) override {}
    void on_order_modified(const OrderModifiedEvent&) override {}
    void on_trade_executed(const TradeExecutedEvent&) override {}
    void on_order_filled(const OrderFilledEvent&) override {}
    void on_order_partially_filled(const OrderPartiallyFilledEvent&) override {}
    void on_book_updated(const BookUpdatedEvent&) override {}
};

class VectorEventSink final : public IEventSink {
public:
    void on_order_accepted(const OrderAcceptedEvent& event) override { events_.emplace_back(event); }
    void on_order_rejected(const OrderRejectedEvent& event) override { events_.emplace_back(event); }
    void on_order_added(const OrderAddedEvent& event) override { events_.emplace_back(event); }
    void on_order_cancelled(const OrderCancelledEvent& event) override { events_.emplace_back(event); }
    void on_order_modified(const OrderModifiedEvent& event) override { events_.emplace_back(event); }
    void on_trade_executed(const TradeExecutedEvent& event) override { events_.emplace_back(event); }
    void on_order_filled(const OrderFilledEvent& event) override { events_.emplace_back(event); }
    void on_order_partially_filled(const OrderPartiallyFilledEvent& event) override { events_.emplace_back(event); }
    void on_book_updated(const BookUpdatedEvent& event) override { events_.emplace_back(event); }

    [[nodiscard]] const std::vector<EngineEvent>& events() const noexcept { return events_; }
    void clear() noexcept { events_.clear(); }

private:
    std::vector<EngineEvent> events_;
};

class ConsoleEventSink final : public IEventSink {
public:
    void on_order_accepted(const OrderAcceptedEvent& e) override {
        std::cout << "[ACCEPTED] Seq=" << e.sequence_id << " OrderID=" << e.order_id
                  << " Side=" << e.side << " Type=" << e.type << " Policy=" << e.policy
                  << " Price=" << e.price << " Qty=" << e.quantity << "\n";
    }

    void on_order_rejected(const OrderRejectedEvent& e) override {
        std::cout << "[REJECTED] Seq=" << e.sequence_id << " OrderID=" << e.order_id
                  << " Reason: " << e.reason << "\n";
    }

    void on_order_added(const OrderAddedEvent& e) override {
        std::cout << "[ADDED] Seq=" << e.sequence_id << " OrderID=" << e.order_id
                  << " Side=" << e.side << " Price=" << e.price << " RemQty=" << e.remaining_qty << "\n";
    }

    void on_order_cancelled(const OrderCancelledEvent& e) override {
        std::cout << "[CANCELLED] Seq=" << e.sequence_id << " OrderID=" << e.order_id
                  << " Side=" << e.side << " Price=" << e.price << " CancelledQty=" << e.cancelled_qty << "\n";
    }

    void on_order_modified(const OrderModifiedEvent& e) override {
        std::cout << "[MODIFIED] Seq=" << e.sequence_id << " OrderID=" << e.order_id
                  << " NewPrice=" << e.new_price << " NewQty=" << e.new_qty
                  << " PriorityRetained=" << (e.priority_retained ? "YES" : "NO") << "\n";
    }

    void on_trade_executed(const TradeExecutedEvent& e) override {
        std::cout << "[TRADE] Seq=" << e.sequence_id << " TradeID=" << e.trade_id
                  << " Aggressor=" << e.aggressor_id << " (" << e.aggressor_side << ")"
                  << " Resting=" << e.resting_id << " Price=" << e.exec_price
                  << " Qty=" << e.exec_qty << "\n";
    }

    void on_order_filled(const OrderFilledEvent& e) override {
        std::cout << "[FILLED] Seq=" << e.sequence_id << " OrderID=" << e.order_id
                  << " Side=" << e.side << " Price=" << e.price << " TotalQty=" << e.total_qty << "\n";
    }

    void on_order_partially_filled(const OrderPartiallyFilledEvent& e) override {
        std::cout << "[PARTIAL_FILL] Seq=" << e.sequence_id << " OrderID=" << e.order_id
                  << " Side=" << e.side << " ExecPrice=" << e.exec_price
                  << " ExecQty=" << e.exec_qty << " RemQty=" << e.remaining_qty << "\n";
    }

    void on_book_updated(const BookUpdatedEvent& e) override {
        std::cout << "[BOOK_UPDATE] Seq=" << e.sequence_id
                  << " BestBid=" << e.best_bid << " (Qty=" << e.best_bid_qty << ")"
                  << " BestAsk=" << e.best_ask << " (Qty=" << e.best_ask_qty << ")\n";
    }
};

class CompositeEventSink final : public IEventSink {
public:
    void add_sink(IEventSink* sink) {
        if (sink) sinks_.push_back(sink);
    }

    void on_order_accepted(const OrderAcceptedEvent& e) override { for (auto* s : sinks_) s->on_order_accepted(e); }
    void on_order_rejected(const OrderRejectedEvent& e) override { for (auto* s : sinks_) s->on_order_rejected(e); }
    void on_order_added(const OrderAddedEvent& e) override { for (auto* s : sinks_) s->on_order_added(e); }
    void on_order_cancelled(const OrderCancelledEvent& e) override { for (auto* s : sinks_) s->on_order_cancelled(e); }
    void on_order_modified(const OrderModifiedEvent& e) override { for (auto* s : sinks_) s->on_order_modified(e); }
    void on_trade_executed(const TradeExecutedEvent& e) override { for (auto* s : sinks_) s->on_trade_executed(e); }
    void on_order_filled(const OrderFilledEvent& e) override { for (auto* s : sinks_) s->on_order_filled(e); }
    void on_order_partially_filled(const OrderPartiallyFilledEvent& e) override { for (auto* s : sinks_) s->on_order_partially_filled(e); }
    void on_book_updated(const BookUpdatedEvent& e) override { for (auto* s : sinks_) s->on_book_updated(e); }

private:
    std::vector<IEventSink*> sinks_;
};

} // namespace matching_engine

#endif // MATCHING_ENGINE_EVENTS_HPP
