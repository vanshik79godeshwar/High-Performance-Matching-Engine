#ifndef MATCHING_ENGINE_ORDER_BOOK_HPP
#define MATCHING_ENGINE_ORDER_BOOK_HPP

#include "order.hpp"
#include "price_level.hpp"
#include "book_snapshot.hpp"
#include <map>
#include <vector>
#include <algorithm>
#include <concepts>
#include <cassert>

namespace matching_engine {

// Strategy A: Standard Ordered Map Strategy
class MapOrderBook {
public:
    using BidMap = std::map<Price, PriceLevel, std::greater<Price>>;
    using AskMap = std::map<Price, PriceLevel, std::less<Price>>;

    MapOrderBook() = default;

    bool add_order(Order* order) {
        assert(order != nullptr);
        if (order->side == Side::Buy) {
            auto [it, _] = bids_.emplace(order->price, PriceLevel(order->price));
            it->second.push_back(order);
            total_bid_quantity_ += order->remaining_qty;
            ++total_bid_orders_;
        } else {
            auto [it, _] = asks_.emplace(order->price, PriceLevel(order->price));
            it->second.push_back(order);
            total_ask_quantity_ += order->remaining_qty;
            ++total_ask_orders_;
        }
        return true;
    }

    bool remove_order(Order* order) {
        assert(order != nullptr);
        if (order->side == Side::Buy) {
            auto it = bids_.find(order->price);
            if (it == bids_.end()) return false;

            Quantity qty = order->remaining_qty;
            it->second.remove(order);
            assert(total_bid_quantity_ >= qty);
            total_bid_quantity_ -= qty;
            if (total_bid_orders_ > 0) --total_bid_orders_;

            if (it->second.empty()) {
                bids_.erase(it);
            }
        } else {
            auto it = asks_.find(order->price);
            if (it == asks_.end()) return false;

            Quantity qty = order->remaining_qty;
            it->second.remove(order);
            assert(total_ask_quantity_ >= qty);
            total_ask_quantity_ -= qty;
            if (total_ask_orders_ > 0) --total_ask_orders_;

            if (it->second.empty()) {
                asks_.erase(it);
            }
        }
        return true;
    }

    void decrease_order_quantity(Order* order, Quantity delta) noexcept {
        assert(order != nullptr);
        if (order->side == Side::Buy) {
            auto it = bids_.find(order->price);
            if (it != bids_.end()) {
                it->second.decrease_quantity(order, delta);
                assert(total_bid_quantity_ >= delta);
                total_bid_quantity_ -= delta;
            }
        } else {
            auto it = asks_.find(order->price);
            if (it != asks_.end()) {
                it->second.decrease_quantity(order, delta);
                assert(total_ask_quantity_ >= delta);
                total_ask_quantity_ -= delta;
            }
        }
    }

    [[nodiscard]] PriceLevel* get_best_bid_level() noexcept {
        if (bids_.empty()) return nullptr;
        return &bids_.begin()->second;
    }

    [[nodiscard]] PriceLevel* get_best_ask_level() noexcept {
        if (asks_.empty()) return nullptr;
        return &asks_.begin()->second;
    }

    [[nodiscard]] const PriceLevel* get_best_bid_level() const noexcept {
        if (bids_.empty()) return nullptr;
        return &bids_.begin()->second;
    }

    [[nodiscard]] const PriceLevel* get_best_ask_level() const noexcept {
        if (asks_.empty()) return nullptr;
        return &asks_.begin()->second;
    }

    void remove_empty_best_bid() {
        if (!bids_.empty() && bids_.begin()->second.empty()) {
            bids_.erase(bids_.begin());
        }
    }

    void remove_empty_best_ask() {
        if (!asks_.empty() && asks_.begin()->second.empty()) {
            asks_.erase(asks_.begin());
        }
    }

    [[nodiscard]] BookSnapshot get_snapshot(size_t max_depth = 10) const {
        BookSnapshot snapshot;
        snapshot.total_bid_qty = total_bid_quantity_;
        snapshot.total_ask_qty = total_ask_quantity_;
        snapshot.total_bid_orders = total_bid_orders_;
        snapshot.total_ask_orders = total_ask_orders_;

        if (!bids_.empty()) {
            snapshot.best_bid = bids_.begin()->first;
            snapshot.best_bid_qty = bids_.begin()->second.total_quantity();
        }
        if (!asks_.empty()) {
            snapshot.best_ask = asks_.begin()->first;
            snapshot.best_ask_qty = asks_.begin()->second.total_quantity();
        }

        size_t count = 0;
        for (const auto& [price, level] : bids_) {
            if (count++ >= max_depth) break;
            snapshot.bids.push_back({price, level.total_quantity(), level.order_count()});
        }

        count = 0;
        for (const auto& [price, level] : asks_) {
            if (count++ >= max_depth) break;
            snapshot.asks.push_back({price, level.total_quantity(), level.order_count()});
        }

        return snapshot;
    }

    [[nodiscard]] size_t total_orders() const noexcept {
        return total_bid_orders_ + total_ask_orders_;
    }

    [[nodiscard]] Quantity total_quantity(Side side) const noexcept {
        return side == Side::Buy ? total_bid_quantity_ : total_ask_quantity_;
    }

    [[nodiscard]] Quantity total_quantity_at_price(Side side, Price price) const {
        if (side == Side::Buy) {
            auto it = bids_.find(price);
            return it != bids_.end() ? it->second.total_quantity() : 0;
        } else {
            auto it = asks_.find(price);
            return it != asks_.end() ? it->second.total_quantity() : 0;
        }
    }

    void clear() noexcept {
        bids_.clear();
        asks_.clear();
        total_bid_quantity_ = 0;
        total_ask_quantity_ = 0;
        total_bid_orders_ = 0;
        total_ask_orders_ = 0;
    }

    [[nodiscard]] const BidMap& bids() const noexcept { return bids_; }
    [[nodiscard]] const AskMap& asks() const noexcept { return asks_; }

private:
    BidMap bids_;
    AskMap asks_;
    Quantity total_bid_quantity_{0};
    Quantity total_ask_quantity_{0};
    size_t total_bid_orders_{0};
    size_t total_ask_orders_{0};
};

// Strategy B: Flat Array / Dense Pool Price Level Strategy
class FlatOrderBook {
public:
    FlatOrderBook() = default;

    bool add_order(Order* order) {
        assert(order != nullptr);
        if (order->side == Side::Buy) {
            PriceLevel* level = find_or_create_level(bids_, order->price, Side::Buy);
            level->push_back(order);
            total_bid_quantity_ += order->remaining_qty;
            ++total_bid_orders_;
        } else {
            PriceLevel* level = find_or_create_level(asks_, order->price, Side::Sell);
            level->push_back(order);
            total_ask_quantity_ += order->remaining_qty;
            ++total_ask_orders_;
        }
        return true;
    }

    bool remove_order(Order* order) {
        assert(order != nullptr);
        if (order->side == Side::Buy) {
            PriceLevel* level = find_level(bids_, order->price);
            if (!level) return false;

            Quantity qty = order->remaining_qty;
            level->remove(order);
            assert(total_bid_quantity_ >= qty);
            total_bid_quantity_ -= qty;
            if (total_bid_orders_ > 0) --total_bid_orders_;

            if (level->empty()) {
                remove_level(bids_, order->price);
            }
        } else {
            PriceLevel* level = find_level(asks_, order->price);
            if (!level) return false;

            Quantity qty = order->remaining_qty;
            level->remove(order);
            assert(total_ask_quantity_ >= qty);
            total_ask_quantity_ -= qty;
            if (total_ask_orders_ > 0) --total_ask_orders_;

            if (level->empty()) {
                remove_level(asks_, order->price);
            }
        }
        return true;
    }

    void decrease_order_quantity(Order* order, Quantity delta) noexcept {
        assert(order != nullptr);
        if (order->side == Side::Buy) {
            PriceLevel* level = find_level(bids_, order->price);
            if (level) {
                level->decrease_quantity(order, delta);
                assert(total_bid_quantity_ >= delta);
                total_bid_quantity_ -= delta;
            }
        } else {
            PriceLevel* level = find_level(asks_, order->price);
            if (level) {
                level->decrease_quantity(order, delta);
                assert(total_ask_quantity_ >= delta);
                total_ask_quantity_ -= delta;
            }
        }
    }

    [[nodiscard]] PriceLevel* get_best_bid_level() noexcept {
        return bids_.empty() ? nullptr : &bids_.front();
    }

    [[nodiscard]] PriceLevel* get_best_ask_level() noexcept {
        return asks_.empty() ? nullptr : &asks_.front();
    }

    [[nodiscard]] const PriceLevel* get_best_bid_level() const noexcept {
        return bids_.empty() ? nullptr : &bids_.front();
    }

    [[nodiscard]] const PriceLevel* get_best_ask_level() const noexcept {
        return asks_.empty() ? nullptr : &asks_.front();
    }

    void remove_empty_best_bid() {
        if (!bids_.empty() && bids_.front().empty()) {
            bids_.erase(bids_.begin());
        }
    }

    void remove_empty_best_ask() {
        if (!asks_.empty() && asks_.front().empty()) {
            asks_.erase(asks_.begin());
        }
    }

    [[nodiscard]] BookSnapshot get_snapshot(size_t max_depth = 10) const {
        BookSnapshot snapshot;
        snapshot.total_bid_qty = total_bid_quantity_;
        snapshot.total_ask_qty = total_ask_quantity_;
        snapshot.total_bid_orders = total_bid_orders_;
        snapshot.total_ask_orders = total_ask_orders_;

        if (!bids_.empty()) {
            snapshot.best_bid = bids_.front().price();
            snapshot.best_bid_qty = bids_.front().total_quantity();
        }
        if (!asks_.empty()) {
            snapshot.best_ask = asks_.front().price();
            snapshot.best_ask_qty = asks_.front().total_quantity();
        }

        size_t count = 0;
        for (const auto& level : bids_) {
            if (count++ >= max_depth) break;
            snapshot.bids.push_back({level.price(), level.total_quantity(), level.order_count()});
        }

        count = 0;
        for (const auto& level : asks_) {
            if (count++ >= max_depth) break;
            snapshot.asks.push_back({level.price(), level.total_quantity(), level.order_count()});
        }

        return snapshot;
    }

    [[nodiscard]] size_t total_orders() const noexcept {
        return total_bid_orders_ + total_ask_orders_;
    }

    [[nodiscard]] Quantity total_quantity(Side side) const noexcept {
        return side == Side::Buy ? total_bid_quantity_ : total_ask_quantity_;
    }

    [[nodiscard]] Quantity total_quantity_at_price(Side side, Price price) const {
        const auto* level = side == Side::Buy ? find_level(bids_, price) : find_level(asks_, price);
        return level ? level->total_quantity() : 0;
    }

    void clear() noexcept {
        bids_.clear();
        asks_.clear();
        total_bid_quantity_ = 0;
        total_ask_quantity_ = 0;
        total_bid_orders_ = 0;
        total_ask_orders_ = 0;
    }

private:
    std::vector<PriceLevel> bids_;
    std::vector<PriceLevel> asks_;
    Quantity total_bid_quantity_{0};
    Quantity total_ask_quantity_{0};
    size_t total_bid_orders_{0};
    size_t total_ask_orders_{0};

    PriceLevel* find_level(std::vector<PriceLevel>& levels, Price price) {
        for (auto& lvl : levels) {
            if (lvl.price() == price) return &lvl;
        }
        return nullptr;
    }

    const PriceLevel* find_level(const std::vector<PriceLevel>& levels, Price price) const {
        for (const auto& lvl : levels) {
            if (lvl.price() == price) return &lvl;
        }
        return nullptr;
    }

    PriceLevel* find_or_create_level(std::vector<PriceLevel>& levels, Price price, Side side) {
        for (auto& lvl : levels) {
            if (lvl.price() == price) return &lvl;
        }

        if (side == Side::Buy) {
            auto it = std::lower_bound(levels.begin(), levels.end(), price,
                [](const PriceLevel& lvl, Price p) { return lvl.price() > p; });
            auto inserted_it = levels.insert(it, PriceLevel(price));
            return &(*inserted_it);
        } else {
            auto it = std::lower_bound(levels.begin(), levels.end(), price,
                [](const PriceLevel& lvl, Price p) { return lvl.price() < p; });
            auto inserted_it = levels.insert(it, PriceLevel(price));
            return &(*inserted_it);
        }
    }

    void remove_level(std::vector<PriceLevel>& levels, Price price) {
        for (auto it = levels.begin(); it != levels.end(); ++it) {
            if (it->price() == price) {
                levels.erase(it);
                break;
            }
        }
    }
};

} // namespace matching_engine

#endif // MATCHING_ENGINE_ORDER_BOOK_HPP
