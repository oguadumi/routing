#pragma once

#include "core/Graph.h"
#include "route/Dijkstra.h"
#include <unordered_map>

namespace olsr {

class Router {
public:
    void recomputeAll(const Graph& g);
    const RouteTable* table(NodeId src) const;

private:
    std::unordered_map<NodeId, RouteTable> tables_;
    DijkstraEngine engine_;
};

} // namespace olsr


