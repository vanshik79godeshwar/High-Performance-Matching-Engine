#include <gtest/gtest.h>
#include "matching_engine/matching_engine.hpp"

using namespace matching_engine;

class OrderManagementTest : public ::testing::Test {
protected:
    VectorEventSink sink;
    MatchingEngine<MapOrderBook> engine{&sink};

    void SetUp() override {
        engine.clear();
        sink.clear();
    }
};

TEST_F(OrderManagementTest, CancelExistingOrder) {
    engine.submit_order(1, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10000, 100);
    EXPECT_EQ(engine.book().total_orders(), 1);

    EXPECT_TRUE(engine.cancel_order(1));
    EXPECT_EQ(engine.book().total_orders(), 0);
    EXPECT_FALSE(engine.order_index().contains(1));
}

TEST_F(OrderManagementTest, CancelNonExistentOrder) {
    EXPECT_FALSE(engine.cancel_order(999));
}

TEST_F(OrderManagementTest, ModifyQuantityReductionPreservesPriority) {
    engine.submit_order(1, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10000, 100);
    engine.submit_order(2, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10000, 200);

    // Reduce qty of order 1 from 100 to 50
    EXPECT_TRUE(engine.modify_order(1, 10000, 50));

    EXPECT_EQ(engine.book().get_best_bid_level()->total_quantity(), 250);
    EXPECT_EQ(engine.book().get_best_bid_level()->front()->id, 1); // Order 1 must still be first!
}

TEST_F(OrderManagementTest, ModifyQuantityIncreaseLosesPriority) {
    engine.submit_order(1, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10000, 100);
    engine.submit_order(2, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10000, 200);

    // Increase qty of order 1 from 100 to 150
    EXPECT_TRUE(engine.modify_order(1, 10000, 150));

    EXPECT_EQ(engine.book().get_best_bid_level()->total_quantity(), 350);
    EXPECT_EQ(engine.book().get_best_bid_level()->front()->id, 2); // Order 2 should now be first!
}

TEST_F(OrderManagementTest, ModifyPriceLosesPriority) {
    engine.submit_order(1, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10000, 100);
    engine.submit_order(2, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10050, 200);

    // Change price of order 1 to 10100 (becomes best bid)
    EXPECT_TRUE(engine.modify_order(1, 10100, 100));

    EXPECT_EQ(engine.book().get_best_bid_level()->price(), 10100);
    EXPECT_EQ(engine.book().get_best_bid_level()->front()->id, 1);
}
