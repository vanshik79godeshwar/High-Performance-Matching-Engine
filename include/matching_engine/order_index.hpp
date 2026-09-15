#ifndef MATCHING_ENGINE_ORDER_INDEX_HPP
#define MATCHING_ENGINE_ORDER_INDEX_HPP

#include "order.hpp"
#include <unordered_map>
#include <cstddef>

namespace matching_engine {

class OrderIndex {
public:
    explicit OrderIndex(size_t initial_capacity = 65536) {
        index_.reserve(initial_capacity);
    }

    bool insert(OrderId id, Order* order) {
        auto [_, inserted] = index_.emplace(id, order);
        return inserted;
    }

    [[nodiscard]] Order* find(OrderId id) const noexcept {
        auto it = index_.find(id);
        if (it != index_.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool erase(OrderId id) noexcept {
        return index_.erase(id) > 0;
    }

    [[nodiscard]] bool contains(OrderId id) const noexcept {
        return index_.find(id) != index_.end();
    }

    void clear() noexcept {
        index_.clear();
    }

    void reserve(size_t capacity) {
        index_.reserve(capacity);
    }

    [[nodiscard]] size_t size() const noexcept {
        return index_.size();
    }

    [[nodiscard]] bool empty() const noexcept {
        return index_.empty();
    }

    template <typename Func>
    void for_each(Func&& func) const {
        for (const auto& [id, order_ptr] : index_) {
            func(id, order_ptr);
        }
    }

private:
    std::unordered_map<OrderId, Order*> index_;
};

} // namespace matching_engine

#endif // MATCHING_ENGINE_ORDER_INDEX_HPP
