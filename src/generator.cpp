#include "matching_engine/generator.hpp"
#include <unordered_set>
#include <algorithm>

namespace matching_engine {

std::vector<ReplayCommand> SyntheticOrderGenerator::generate(size_t count) {
    std::vector<ReplayCommand> commands;
    commands.reserve(count);

    std::uniform_real_distribution<double> action_dist(0.0, 1.0);
    std::uniform_real_distribution<double> side_dist(0.0, 1.0);
    std::uniform_real_distribution<double> policy_dist(0.0, 1.0);
    std::uniform_int_distribution<Price> price_offset_dist(-static_cast<Price>(config_.price_spread), static_cast<Price>(config_.price_spread));
    std::uniform_int_distribution<Quantity> qty_dist(config_.min_qty, config_.max_qty);

    std::vector<OrderId> active_order_ids;
    OrderId next_id = 1;
    uint64_t seq = 1;
    uint64_t ts = 1000000;

    for (size_t i = 0; i < count; ++i) {
        double r = action_dist(rng_);
        ReplayCommand cmd;
        cmd.sequence_id = seq++;
        cmd.timestamp = ts += 100;

        if (r < config_.limit_ratio || active_order_ids.empty()) {
            // NEW Limit Order
            cmd.action = ReplayAction::NEW;
            cmd.order_id = next_id++;
            cmd.side = (side_dist(rng_) < 0.5) ? Side::Buy : Side::Sell;
            cmd.type = OrderType::Limit;

            double p_rand = policy_dist(rng_);
            if (p_rand < config_.ioc_ratio) cmd.policy = ExecutionPolicy::IOC;
            else if (p_rand < config_.ioc_ratio + config_.fok_ratio) cmd.policy = ExecutionPolicy::FOK;
            else cmd.policy = ExecutionPolicy::GTC;

            Price offset = price_offset_dist(rng_);
            cmd.price = config_.mid_price + (cmd.side == Side::Buy ? -std::abs(offset) : std::abs(offset));
            if (cmd.price <= 0) cmd.price = 1;
            cmd.quantity = qty_dist(rng_);

            if (cmd.policy == ExecutionPolicy::GTC) {
                active_order_ids.push_back(cmd.order_id);
            }
        } else if (r < config_.limit_ratio + config_.market_ratio) {
            // NEW Market Order
            cmd.action = ReplayAction::NEW;
            cmd.order_id = next_id++;
            cmd.side = (side_dist(rng_) < 0.5) ? Side::Buy : Side::Sell;
            cmd.type = OrderType::Market;
            cmd.policy = ExecutionPolicy::IOC;
            cmd.price = 0;
            cmd.quantity = qty_dist(rng_);
        } else if (r < config_.limit_ratio + config_.market_ratio + config_.cancel_ratio) {
            // CANCEL Order
            cmd.action = ReplayAction::CANCEL;
            std::uniform_int_distribution<size_t> idx_dist(0, active_order_ids.size() - 1);
            size_t idx = idx_dist(rng_);
            cmd.order_id = active_order_ids[idx];

            active_order_ids[idx] = active_order_ids.back();
            active_order_ids.pop_back();
        } else {
            // MODIFY Order
            cmd.action = ReplayAction::MODIFY;
            std::uniform_int_distribution<size_t> idx_dist(0, active_order_ids.size() - 1);
            size_t idx = idx_dist(rng_);
            cmd.order_id = active_order_ids[idx];

            Price offset = price_offset_dist(rng_);
            cmd.price = config_.mid_price + offset;
            if (cmd.price <= 0) cmd.price = 1;
            cmd.quantity = qty_dist(rng_);
        }

        commands.push_back(cmd);
    }
    return commands;
}

} // namespace matching_engine
