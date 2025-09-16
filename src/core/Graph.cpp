#include "core/Graph.h"

#include <algorithm>

namespace olsr {

static bool sameUndirected(NodeId a, NodeId b, NodeId c, NodeId d) {
    return (a == c && b == d) || (a == d && b == c);
}

NodeId Graph::addNode(std::string label, float x, float y) {
    NodeId newId = static_cast<NodeId>(nodes_.size() + 1);
    nodes_.push_back(Node{newId, std::move(label), x, y, true});
    return newId;
}

bool Graph::removeNode(NodeId id) {
    auto it = std::remove_if(links_.begin(), links_.end(), [id](const Link& l){
        return l.u == id || l.v == id;
    });
    links_.erase(it, links_.end());

    auto nit = std::remove_if(nodes_.begin(), nodes_.end(), [id](const Node& n){ return n.id == id; });
    if (nit == nodes_.end()) return false;
    nodes_.erase(nit, nodes_.end());
    return true;
}

bool Graph::addLink(NodeId u, NodeId v, double weight) {
    if (u == v) return false;
    if (!nodeExists(u) || !nodeExists(v)) return false;
    for (const auto& l : links_) {
        if (sameUndirected(l.u, l.v, u, v)) return false; // prevent duplicates
    }
    links_.push_back(Link{u, v, weight, weight, LinkStatus::UP, false});
    return true;
}

bool Graph::setLinkStatus(NodeId u, NodeId v, LinkStatus st) {
    for (auto& l : links_) {
        if (sameUndirected(l.u, l.v, u, v)) {
            l.status = st;
            l.manually_jammed = (st == LinkStatus::DOWN);
            return true;
        }
    }
    return false;
}

bool Graph::setLinkWeight(NodeId u, NodeId v, double w) {
    for (auto& l : links_) {
        if (sameUndirected(l.u, l.v, u, v)) {
            l.weight = w;
            if (!l.jammed) l.orig_weight = w;
            return true;
        }
    }
    return false;
}

const Link* Graph::findLink(NodeId u, NodeId v) const {
    for (const auto& l : links_) {
        if (sameUndirected(l.u, l.v, u, v)) return &l;
    }
    return nullptr;
}

bool Graph::nodeExists(NodeId id) const {
    for (const auto& n : nodes_) if (n.id == id) return true;
    return false;
}

} // namespace olsr


