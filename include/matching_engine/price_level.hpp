#ifndef MATCHING_ENGINE_PRICE_LEVEL_HPP
#define MATCHING_ENGINE_PRICE_LEVEL_HPP

#include "order.hpp"
#include <cassert>
#include <cstddef>

namespace matching_engine {

class PriceLevel {
public:
    PriceLevel() = default;
    explicit PriceLevel(Price p) : price_(p) {}

    [[nodiscard]] Price price() const noexcept { return price_; }
    [[nodiscard]] Quantity total_quantity() const noexcept { return total_quantity_; }
    [[nodiscard]] size_t order_count() const noexcept { return count_; }
    [[nodiscard]] bool empty() const noexcept { return count_ == 0; }

    [[nodiscard]] Order* front() const noexcept { return head_; }
    [[nodiscard]] Order* back() const noexcept { return tail_; }

    void push_back(Order* order) noexcept {
        assert(order != nullptr);
        order->prev = tail_;
        order->next = nullptr;

        if (tail_) {
            tail_->next = order;
        } else {
            head_ = order;
        }
        tail_ = order;

        total_quantity_ += order->remaining_qty;
        ++count_;
    }

    void remove(Order* order) noexcept {
        assert(order != nullptr);
        if (order->prev) {
            order->prev->next = order->next;
        } else {
            head_ = order->next;
        }

        if (order->next) {
            order->next->prev = order->prev;
        } else {
            tail_ = order->prev;
        }

        order->prev = nullptr;
        order->next = nullptr;

        assert(total_quantity_ >= order->remaining_qty);
        total_quantity_ -= order->remaining_qty;
        if (count_ > 0) {
            --count_;
        }
    }

    Order* pop_front() noexcept {
        if (!head_) return nullptr;
        Order* order = head_;
        remove(order);
        return order;
    }

    void decrease_quantity(Order* order, Quantity delta) noexcept {
        assert(order != nullptr);
        assert(delta <= order->remaining_qty);
        order->remaining_qty -= delta;
        assert(total_quantity_ >= delta);
        total_quantity_ -= delta;
    }

private:
    Price price_{0};
    Quantity total_quantity_{0};
    size_t count_{0};
    Order* head_{nullptr};
    Order* tail_{nullptr};
};

} // namespace matching_engine

#endif // MATCHING_ENGINE_PRICE_LEVEL_HPP
