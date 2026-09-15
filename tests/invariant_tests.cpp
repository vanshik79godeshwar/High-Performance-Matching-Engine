#include <gtest/gtest.h>
#include "matching_engine/matching_engine.hpp"
#include "matching_engine/generator.hpp"
#include "matching_engine/invariant_checker.hpp"

using namespace matching_engine;

TEST(InvariantTest, ValidateInvariantsOnEmptyBook) {
    MatchingEngine<MapOrderBook> engine;
    std::vector<std::string> errors;
    EXPECT_TRUE(InvariantChecker::validate(engine, errors));
    EXPECT_TRUE(errors.empty());
}

TEST(InvariantTest, ValidateInvariantsOnComplexBook) {
    MatchingEngine<MapOrderBook> engine;
    SyntheticOrderGenerator generator;
    auto commands = generator.generate(2000);

    for (const auto& cmd : commands) {
        switch (cmd.action) {
            case ReplayAction::NEW:
                engine.submit_order(cmd.order_id, cmd.side, cmd.type, cmd.policy, cmd.price, cmd.quantity, cmd.timestamp);
                break;
            case ReplayAction::CANCEL:
                engine.cancel_order(cmd.order_id, cmd.timestamp);
                break;
            case ReplayAction::MODIFY:
                engine.modify_order(cmd.order_id, cmd.price, cmd.quantity, cmd.timestamp);
                break;
        }

        std::vector<std::string> errors;
        bool valid = InvariantChecker::validate(engine, errors);
        if (!valid) {
            FAIL() << "Invariant violation after command seq=" << cmd.sequence_id << ": " << errors[0];
        }
    }
}
