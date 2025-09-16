#pragma once

#include "core/Graph.h"
#include <queue>
#include <unordered_map>

namespace olsr {

struct RouteEntry {
    NodeId destination;
    NodeId next_hop;
    double total_cost;
    uint32_t hop_count;
};

using RouteTable = std::vector<RouteEntry>;

class DijkstraEngine {
public:
    RouteTable compute(const Graph& g, NodeId source) const;
};

} // namespace olsr


