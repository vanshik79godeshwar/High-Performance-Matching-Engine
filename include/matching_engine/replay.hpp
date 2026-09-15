#ifndef MATCHING_ENGINE_REPLAY_HPP
#define MATCHING_ENGINE_REPLAY_HPP

#include "order.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

namespace matching_engine {

enum class ReplayAction : uint8_t {
    NEW = 0,
    CANCEL = 1,
    MODIFY = 2
};

struct ReplayCommand {
    ReplayAction action{ReplayAction::NEW};
    SequenceId sequence_id{0};
    Timestamp timestamp{0};
    OrderId order_id{0};
    Side side{Side::Buy};
    OrderType type{OrderType::Limit};
    ExecutionPolicy policy{ExecutionPolicy::GTC};
    Price price{0};
    Quantity quantity{0};
};

class ReplayEngine {
public:
    static std::vector<ReplayCommand> load_csv(const std::string& filepath);
    static bool save_csv(const std::string& filepath, const std::vector<ReplayCommand>& commands);

    template <typename MatchingEngineType>
    static size_t run_replay(MatchingEngineType& engine, const std::vector<ReplayCommand>& commands) {
        size_t processed = 0;
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
            ++processed;
        }
        return processed;
    }
};

} // namespace matching_engine

#endif // MATCHING_ENGINE_REPLAY_HPP
