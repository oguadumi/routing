#include "route/Router.h"

namespace olsr {

void Router::recomputeAll(const Graph& g) {
    tables_.clear();
    for (const auto& n : g.nodes()) {
        tables_.emplace(n.id, engine_.compute(g, n.id));
    }
}

const RouteTable* Router::table(NodeId src) const {
    auto it = tables_.find(src);
    if (it == tables_.end()) return nullptr;
    return &it->second;
}

} // namespace olsr


