#include "route/Dijkstra.h"

#include <limits>
#include <unordered_map>
#include <algorithm>

namespace olsr {

struct PQNode {
    NodeId node;
    double cost;
    bool operator>(const PQNode& other) const { return cost > other.cost; }
};

RouteTable DijkstraEngine::compute(const Graph& g, NodeId source) const {
    const double INF = std::numeric_limits<double>::infinity();
    std::unordered_map<NodeId, double> dist;
    std::unordered_map<NodeId, NodeId> parent;
    std::unordered_map<NodeId, NodeId> firstHop;
    std::unordered_map<NodeId, uint32_t> hops;

    for (const auto& n : g.nodes()) {
        dist[n.id] = INF;
        hops[n.id] = 0;
    }
    dist[source] = 0.0;

    std::priority_queue<PQNode, std::vector<PQNode>, std::greater<PQNode>> pq;
    pq.push(PQNode{source, 0.0});

    auto relax = [&](NodeId u, NodeId v, double w){
        if (dist[u] + w < dist[v]) {
            dist[v] = dist[u] + w;
            parent[v] = u;
            hops[v] = hops[u] + 1;
            // establish first hop from source to v
            if (u == source) firstHop[v] = v;
            else firstHop[v] = firstHop[u];
            pq.push(PQNode{v, dist[v]});
        }
    };

    // Build adjacency on the fly from links
    while (!pq.empty()) {
        auto [u, cost] = pq.top();
        pq.pop();
        if (cost != dist[u]) continue;

        for (const auto& l : g.links()) {
            if (l.status != LinkStatus::UP) continue;
            if (l.u == u) relax(u, l.v, l.weight);
            else if (l.v == u) relax(u, l.u, l.weight);
        }
    }

    RouteTable table;
    table.reserve(dist.size());
    for (const auto& [nid, d] : dist) {
        if (nid == source) continue;
        if (d == INF) continue;
        auto itHop = firstHop.find(nid);
        if (itHop == firstHop.end()) continue;
        table.push_back(RouteEntry{nid, itHop->second, d, hops[nid]});
    }
    // Stable ordering by destination id
    std::sort(table.begin(), table.end(), [](const RouteEntry& a, const RouteEntry& b){ return a.destination < b.destination; });
    return table;
}

} // namespace olsr


