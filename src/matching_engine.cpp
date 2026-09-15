#include "matching_engine/matching_engine.hpp"

namespace matching_engine {

// Explicit template instantiations for MapOrderBook and FlatOrderBook
template class MatchingEngine<MapOrderBook>;
template class MatchingEngine<FlatOrderBook>;

} // namespace matching_engine
