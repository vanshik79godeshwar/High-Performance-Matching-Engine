#include <gtest/gtest.h>
#include "matching_engine/reference_engine.hpp"

using namespace matching_engine;

TEST(ReferenceEngineTest, BasicMatch) {
    ReferenceEngine ref;
    EXPECT_TRUE(ref.submit_order(1, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10000, 100, 1));
    EXPECT_TRUE(ref.submit_order(2, Side::Sell, OrderType::Limit, ExecutionPolicy::GTC, 10000, 100, 2));

    EXPECT_EQ(ref.trades().size(), 1);
    EXPECT_EQ(ref.orders().size(), 0);
}

TEST(ReferenceEngineTest, CancelOrder) {
    ReferenceEngine ref;
    ref.submit_order(1, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10000, 100, 1);
    EXPECT_TRUE(ref.cancel_order(1));
    EXPECT_EQ(ref.orders().size(), 0);
}
