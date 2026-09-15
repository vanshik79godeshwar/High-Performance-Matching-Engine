#ifndef MATCHING_ENGINE_INVARIANT_CHECKER_HPP
#define MATCHING_ENGINE_INVARIANT_CHECKER_HPP

#include "matching_engine.hpp"
#include <vector>
#include <string>
#include <unordered_set>
#include <iostream>

namespace matching_engine {

class InvariantChecker {
public:
    template <typename BookImpl>
    static bool validate(const MatchingEngine<BookImpl>& engine, std::vector<std::string>& errors) {
        errors.clear();

        const auto& book = engine.book();
        const auto& index = engine.order_index();

        std::unordered_set<OrderId> book_order_ids;
        std::unordered_set<OrderId> bid_order_ids;
        std::unordered_set<OrderId> ask_order_ids;

        // 1. Validate Bid side price level ordering and order integrity
        if constexpr (std::is_same_v<BookImpl, MapOrderBook>) {
            Price prev_price = INT64_MAX;
            for (const auto& [price, level] : book.bids()) {
                if (price >= prev_price) {
                    errors.push_back("Bid prices not strictly descending: price " +
                                      std::to_string(price) + " >= prev " + std::to_string(prev_price));
                }
                prev_price = price;

                // Validate FIFO queue in level
                const Order* current = level.front();
                SequenceId prev_seq = 0;
                Quantity level_calculated_qty = 0;
                size_t level_calculated_count = 0;

                while (current) {
                    if (current->side != Side::Buy) {
                        errors.push_back("Order " + std::to_string(current->id) + " on Bid side has Side::Sell");
                    }
                    if (current->price != price) {
                        errors.push_back("Order " + std::to_string(current->id) + " price " +
                                          std::to_string(current->price) + " does not match level price " + std::to_string(price));
                    }
                    if (current->remaining_qty == 0) {
                        errors.push_back("Order " + std::to_string(current->id) + " has 0 remaining quantity in book");
                    }
                    if (current->type == OrderType::Market) {
                        errors.push_back("Market Order " + std::to_string(current->id) + " found resting in Bid book");
                    }
                    if (current->sequence_id <= prev_seq && prev_seq != 0) {
                        errors.push_back("FIFO ordering violated in Bid level " + std::to_string(price) +
                                          ": seq " + std::to_string(current->sequence_id) + " <= prev " + std::to_string(prev_seq));
                    }
                    prev_seq = current->sequence_id;

                    if (!bid_order_ids.insert(current->id).second) {
                        errors.push_back("Duplicate OrderId " + std::to_string(current->id) + " found in Bid book");
                    }
                    if (!book_order_ids.insert(current->id).second) {
                        errors.push_back("Order " + std::to_string(current->id) + " exists on both sides or multiple times");
                    }

                    level_calculated_qty += current->remaining_qty;
                    ++level_calculated_count;
                    current = current->next;
                }

                if (level_calculated_qty != level.total_quantity()) {
                    errors.push_back("Level total_quantity mismatch at price " + std::to_string(price));
                }
                if (level_calculated_count != level.order_count()) {
                    errors.push_back("Level order_count mismatch at price " + std::to_string(price));
                }
            }

            // 2. Validate Ask side price level ordering and order integrity
            Price prev_ask_price = 0;
            for (const auto& [price, level] : book.asks()) {
                if (price <= prev_ask_price && prev_ask_price != 0) {
                    errors.push_back("Ask prices not strictly ascending: price " +
                                      std::to_string(price) + " <= prev " + std::to_string(prev_ask_price));
                }
                prev_ask_price = price;

                const Order* current = level.front();
                SequenceId prev_seq = 0;
                Quantity level_calculated_qty = 0;
                size_t level_calculated_count = 0;

                while (current) {
                    if (current->side != Side::Sell) {
                        errors.push_back("Order " + std::to_string(current->id) + " on Ask side has Side::Buy");
                    }
                    if (current->price != price) {
                        errors.push_back("Order " + std::to_string(current->id) + " price " +
                                          std::to_string(current->price) + " does not match level price " + std::to_string(price));
                    }
                    if (current->remaining_qty == 0) {
                        errors.push_back("Order " + std::to_string(current->id) + " has 0 remaining quantity in book");
                    }
                    if (current->type == OrderType::Market) {
                        errors.push_back("Market Order " + std::to_string(current->id) + " found resting in Ask book");
                    }
                    if (current->sequence_id <= prev_seq && prev_seq != 0) {
                        errors.push_back("FIFO ordering violated in Ask level " + std::to_string(price) +
                                          ": seq " + std::to_string(current->sequence_id) + " <= prev " + std::to_string(prev_seq));
                    }
                    prev_seq = current->sequence_id;

                    if (!ask_order_ids.insert(current->id).second) {
                        errors.push_back("Duplicate OrderId " + std::to_string(current->id) + " found in Ask book");
                    }
                    if (!book_order_ids.insert(current->id).second) {
                        errors.push_back("Order " + std::to_string(current->id) + " exists on both sides or multiple times");
                    }

                    level_calculated_qty += current->remaining_qty;
                    ++level_calculated_count;
                    current = current->next;
                }

                if (level_calculated_qty != level.total_quantity()) {
                    errors.push_back("Level total_quantity mismatch at Ask price " + std::to_string(price));
                }
                if (level_calculated_count != level.order_count()) {
                    errors.push_back("Level order_count mismatch at Ask price " + std::to_string(price));
                }
            }
        }

        // 3. Best Bid < Best Ask spread invariant
        const PriceLevel* best_bid = book.get_best_bid_level();
        const PriceLevel* best_ask = book.get_best_ask_level();
        if (best_bid && best_ask) {
            if (best_bid->price() >= best_ask->price()) {
                errors.push_back("Spread crossed! Best Bid (" + std::to_string(best_bid->price()) +
                                  ") >= Best Ask (" + std::to_string(best_ask->price()) + ")");
            }
        }

        // 4. Synchronization between Order Index and Order Book
        if (index.size() != book_order_ids.size()) {
            errors.push_back("OrderIndex size (" + std::to_string(index.size()) +
                              ") does not match Book order count (" + std::to_string(book_order_ids.size()) + ")");
        }

        index.for_each([&](OrderId id, const Order* order_ptr) {
            if (book_order_ids.find(id) == book_order_ids.end()) {
                errors.push_back("Order " + std::to_string(id) + " in OrderIndex is missing from Order Book");
            }
            if (order_ptr && order_ptr->id != id) {
                errors.push_back("OrderIndex key " + std::to_string(id) + " points to order with mismatched id " + std::to_string(order_ptr->id));
            }
        });

        return errors.empty();
    }
};

} // namespace matching_engine

#endif // MATCHING_ENGINE_INVARIANT_CHECKER_HPP
