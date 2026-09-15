#include <gtest/gtest.h>
#include "matching_engine/matching_engine.hpp"
#include "matching_engine/reference_engine.hpp"
#include "matching_engine/generator.hpp"
#include "matching_engine/invariant_checker.hpp"

using namespace matching_engine;

TEST(RandomizedDifferentialTest, CompareEngineAgainstReference) {
    SyntheticOrderGenerator generator;
    auto commands = generator.generate(10000);

    NullEventSink null_sink;
    MatchingEngine<MapOrderBook> opt_engine(&null_sink);
    ReferenceEngine ref_engine;

    size_t step = 0;
    for (const auto& cmd : commands) {
        ++step;
        switch (cmd.action) {
            case ReplayAction::NEW:
                opt_engine.submit_order(cmd.order_id, cmd.side, cmd.type, cmd.policy, cmd.price, cmd.quantity, cmd.timestamp);
                ref_engine.submit_order(cmd.order_id, cmd.side, cmd.type, cmd.policy, cmd.price, cmd.quantity, cmd.sequence_id);
                break;
            case ReplayAction::CANCEL:
                opt_engine.cancel_order(cmd.order_id, cmd.timestamp);
                ref_engine.cancel_order(cmd.order_id);
                break;
            case ReplayAction::MODIFY:
                opt_engine.modify_order(cmd.order_id, cmd.price, cmd.quantity, cmd.timestamp);
                ref_engine.modify_order(cmd.order_id, cmd.price, cmd.quantity, cmd.sequence_id);
                break;
        }

        // Periodically check invariants
        if (step % 500 == 0) {
            std::vector<std::string> errors;
            ASSERT_TRUE(InvariantChecker::validate(opt_engine, errors))
                << "Invariant failed at step " << step << ": " << (errors.empty() ? "" : errors[0]);
        }
    }

    // Compare trade counts
    EXPECT_EQ(opt_engine.trade_count(), ref_engine.trades().size());

    // Compare remaining order count in book
    EXPECT_EQ(opt_engine.book().total_orders(), ref_engine.orders().size());
}
