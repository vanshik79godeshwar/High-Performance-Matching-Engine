#ifndef MATCHING_ENGINE_ORDER_POOL_HPP
#define MATCHING_ENGINE_ORDER_POOL_HPP

#include "order.hpp"
#include <vector>
#include <memory>
#include <cassert>
#include <cstddef>

namespace matching_engine {

class OrderPool {
public:
    explicit OrderPool(size_t initial_capacity = 65536) {
        reserve(initial_capacity);
    }

    ~OrderPool() = default;

    OrderPool(const OrderPool&) = delete;
    OrderPool& operator=(const OrderPool&) = delete;
    OrderPool(OrderPool&&) noexcept = default;
    OrderPool& operator=(OrderPool&&) noexcept = default;

    void reserve(size_t capacity) {
        if (capacity <= capacity_) return;
        size_t additional = capacity - capacity_;
        auto block = std::make_unique<Order[]>(additional);
        Order* raw_block = block.get();
        blocks_.push_back(std::move(block));

        for (size_t i = 0; i < additional; ++i) {
            raw_block[i].next = free_list_;
            free_list_ = &raw_block[i];
        }
        capacity_ = capacity;
    }

    [[nodiscard]] Order* allocate(OrderId id, Side side, OrderType type, ExecutionPolicy policy,
                                  Price price, Quantity qty, SequenceId seq_id, Timestamp ts = 0) {
        if (!free_list_) {
            reserve(capacity_ == 0 ? 1024 : capacity_ * 2);
        }
        Order* order = free_list_;
        free_list_ = free_list_->next;

        order->id = id;
        order->side = side;
        order->type = type;
        order->policy = policy;
        order->price = price;
        order->initial_qty = qty;
        order->remaining_qty = qty;
        order->sequence_id = seq_id;
        order->timestamp = ts;
        order->prev = nullptr;
        order->next = nullptr;

        ++active_count_;
        return order;
    }

    void deallocate(Order* order) noexcept {
        if (!order) return;
        order->prev = nullptr;
        order->next = free_list_;
        free_list_ = order;
        if (active_count_ > 0) {
            --active_count_;
        }
    }

    void clear() noexcept {
        free_list_ = nullptr;
        active_count_ = 0;
        size_t total = capacity_;
        capacity_ = 0;
        blocks_.clear();
        if (total > 0) {
            reserve(total);
        }
    }

    [[nodiscard]] size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] size_t active_count() const noexcept { return active_count_; }

private:
    std::vector<std::unique_ptr<Order[]>> blocks_;
    Order* free_list_{nullptr};
    size_t capacity_{0};
    size_t active_count_{0};
};

} // namespace matching_engine

#endif // MATCHING_ENGINE_ORDER_POOL_HPP
