#include <gtest/gtest.h>
#include "matching_engine/matching_engine.hpp"

using namespace matching_engine;

class MatchingEngineTest : public ::testing::Test {
protected:
    VectorEventSink sink;
    MatchingEngine<MapOrderBook> engine{&sink};

    void SetUp() override {
        engine.clear();
        sink.clear();
    }
};

TEST_F(MatchingEngineTest, SingleLimitOrderNoMatch) {
    EXPECT_TRUE(engine.submit_order(1, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10000, 100));

    EXPECT_EQ(engine.book().total_orders(), 1);
    EXPECT_EQ(engine.book().get_best_bid_level()->price(), 10000);
    EXPECT_EQ(engine.book().get_best_ask_level(), nullptr);
}

TEST_F(MatchingEngineTest, ExactMatchLimitOrders) {
    engine.submit_order(1, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10000, 100);
    engine.submit_order(2, Side::Sell, OrderType::Limit, ExecutionPolicy::GTC, 10000, 100);

    EXPECT_EQ(engine.trade_count(), 1);
    EXPECT_EQ(engine.book().total_orders(), 0);
    EXPECT_EQ(engine.book().get_best_bid_level(), nullptr);
    EXPECT_EQ(engine.book().get_best_ask_level(), nullptr);
}

TEST_F(MatchingEngineTest, PartialFillLimitOrder) {
    engine.submit_order(1, Side::Buy, OrderType::Limit, ExecutionPolicy::GTC, 10000, 100);
    engine.submit_order(2, Side::Sell, OrderType::Limit, ExecutionPolicy::GTC, 10000, 40);

    EXPECT_EQ(engine.trade_count(), 1);
    EXPECT_EQ(engine.book().total_orders(), 1);
    ASSERT_NE(engine.book().get_best_bid_level(), nullptr);
    EXPECT_EQ(engine.book().get_best_bid_level()->total_quantity(), 60);
}

TEST_F(MatchingEngineTest, MarketOrderFillsAvailableLiquidity) {
    engine.submit_order(1, Side::Sell, OrderType::Limit, ExecutionPolicy::GTC, 10000, 50);
    engine.submit_order(2, Side::Sell, OrderType::Limit, ExecutionPolicy::GTC, 10050, 50);

    // Aggressive Market Buy for 80
    engine.submit_order(3, Side::Buy, OrderType::Market, ExecutionPolicy::IOC, 0, 80);

    EXPECT_EQ(engine.trade_count(), 2);
    EXPECT_EQ(engine.book().total_orders(), 1);
    ASSERT_NE(engine.book().get_best_ask_level(), nullptr);
    EXPECT_EQ(engine.book().get_best_ask_level()->price(), 10050);
    EXPECT_EQ(engine.book().get_best_ask_level()->total_quantity(), 20);
}

TEST_F(MatchingEngineTest, IOCOrderDoesNotRest) {
    engine.submit_order(1, Side::Sell, OrderType::Limit, ExecutionPolicy::GTC, 10000, 30);

    // IOC Limit Buy for 100 @ 10000
    engine.submit_order(2, Side::Buy, OrderType::Limit, ExecutionPolicy::IOC, 10000, 100);

    EXPECT_EQ(engine.trade_count(), 1);
    EXPECT_EQ(engine.book().total_orders(), 0);
}

TEST_F(MatchingEngineTest, FOKOrderZeroPartialExecution) {
    engine.submit_order(1, Side::Sell, OrderType::Limit, ExecutionPolicy::GTC, 10000, 30);

    // FOK Limit Buy for 100 @ 10000 (Insufficient liquidity, must fail completely)
    bool res = engine.submit_order(2, Side::Buy, OrderType::Limit, ExecutionPolicy::FOK, 10000, 100);
    EXPECT_FALSE(res);

    EXPECT_EQ(engine.trade_count(), 0);
    EXPECT_EQ(engine.book().total_orders(), 1);
    EXPECT_EQ(engine.book().get_best_ask_level()->total_quantity(), 30);
}

TEST_F(MatchingEngineTest, FOKOrderFullExecutionSuccess) {
    engine.submit_order(1, Side::Sell, OrderType::Limit, ExecutionPolicy::GTC, 10000, 60);
    engine.submit_order(2, Side::Sell, OrderType::Limit, ExecutionPolicy::GTC, 10000, 40);

    // FOK Limit Buy for 100 @ 10000 (Sufficient liquidity)
    bool res = engine.submit_order(3, Side::Buy, OrderType::Limit, ExecutionPolicy::FOK, 10000, 100);
    EXPECT_TRUE(res);

    EXPECT_EQ(engine.trade_count(), 2);
    EXPECT_EQ(engine.book().total_orders(), 0);
}
