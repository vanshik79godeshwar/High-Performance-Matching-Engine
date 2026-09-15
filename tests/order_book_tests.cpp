#include <gtest/gtest.h>
#include "matching_engine/order_book.hpp"
#include "matching_engine/order_pool.hpp"

using namespace matching_engine;

TEST(OrderBookTest, MapOrderBookInsertAndBestPrice) {
    OrderPool pool;
    MapOrderBook book;

    Order* b1 = pool.allocate(1, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10000, 100, 1);
    Order* b2 = pool.allocate(2, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10050, 200, 2);
    Order* a1 = pool.allocate(3, Side::Sell, OrderType::Limit, ExecutionPolicy::GTC, 10100, 150, 3);
    Order* a2 = pool.allocate(4, Side::Sell, OrderType::Limit, ExecutionPolicy::GTC, 10075, 50, 4);

    book.add_order(b1);
    book.add_order(b2);
    book.add_order(a1);
    book.add_order(a2);

    ASSERT_NE(book.get_best_bid_level(), nullptr);
    EXPECT_EQ(book.get_best_bid_level()->price(), 10050);
    EXPECT_EQ(book.get_best_bid_level()->total_quantity(), 200);

    ASSERT_NE(book.get_best_ask_level(), nullptr);
    EXPECT_EQ(book.get_best_ask_level()->price(), 10075);
    EXPECT_EQ(book.get_best_ask_level()->total_quantity(), 50);

    EXPECT_EQ(book.total_orders(), 4);
    EXPECT_EQ(book.total_quantity(Side::Buy), 300);
    EXPECT_EQ(book.total_quantity(Side::Sell), 200);
}

TEST(OrderBookTest, FlatOrderBookInsertAndBestPrice) {
    OrderPool pool;
    FlatOrderBook book;

    Order* b1 = pool.allocate(1, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10000, 100, 1);
    Order* b2 = pool.allocate(2, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10050, 200, 2);
    Order* a1 = pool.allocate(3, Side::Sell, OrderType::Limit, ExecutionPolicy::GTC, 10100, 150, 3);

    book.add_order(b1);
    book.add_order(b2);
    book.add_order(a1);

    ASSERT_NE(book.get_best_bid_level(), nullptr);
    EXPECT_EQ(book.get_best_bid_level()->price(), 10050);

    ASSERT_NE(book.get_best_ask_level(), nullptr);
    EXPECT_EQ(book.get_best_ask_level()->price(), 10100);

    book.remove_order(b2);
    EXPECT_EQ(book.get_best_bid_level()->price(), 10000);
}
