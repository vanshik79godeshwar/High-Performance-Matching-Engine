#ifndef MATCHING_ENGINE_GENERATOR_HPP
#define MATCHING_ENGINE_GENERATOR_HPP

#include "replay.hpp"
#include <vector>
#include <random>

namespace matching_engine {

struct GeneratorConfig {
    uint64_t seed{42};
    Price mid_price{10000};
    uint32_t price_spread{50}; // Tick spread range
    Quantity min_qty{10};
    Quantity max_qty{1000};

    double limit_ratio{0.70};
    double market_ratio{0.10};
    double cancel_ratio{0.15};
    double modify_ratio{0.05};

    double ioc_ratio{0.05};
    double fok_ratio{0.05};
};

class SyntheticOrderGenerator {
public:
    explicit SyntheticOrderGenerator(GeneratorConfig config = {})
        : config_(config), rng_(config.seed) {}

    std::vector<ReplayCommand> generate(size_t count);

private:
    GeneratorConfig config_;
    std::mt19937_64 rng_;
};

} // namespace matching_engine

#endif // MATCHING_ENGINE_GENERATOR_HPP
