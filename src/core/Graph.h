#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace olsr {

using NodeId = uint32_t;

struct Node {
    NodeId id;
    std::string label;
    float x;
    float y;
    bool up = true;
};

enum class LinkStatus { UP, DOWN };

struct Link {
    NodeId u;
    NodeId v;
    double weight;
    double orig_weight;
    LinkStatus status = LinkStatus::UP;
    bool jammed = false;
    bool manually_jammed = false;  // true if user manually jammed this link
};

class Graph {
public:
    const std::vector<Node>& nodes() const { return nodes_; }
    const std::vector<Link>& links() const { return links_; }
    std::vector<Node>& nodes() { return nodes_; }
    std::vector<Link>& links() { return links_; }

    NodeId addNode(std::string label, float x, float y);
    bool removeNode(NodeId id);
    bool addLink(NodeId u, NodeId v, double weight);
    bool setLinkStatus(NodeId u, NodeId v, LinkStatus st);
    bool setLinkWeight(NodeId u, NodeId v, double w);
    const Link* findLink(NodeId u, NodeId v) const;

    // Utility
    bool nodeExists(NodeId id) const;

private:
    std::vector<Node> nodes_;
    std::vector<Link> links_;
};

} // namespace olsr


