#pragma once

#include "core/Graph.h"
#include <unordered_map>
#include <utility>

namespace olsr {

struct HysteresisParams {
    double alpha = 0.3;        // EMA factor
    double thetaUp = 1.6;      // DOWN threshold
    double thetaDown = 1.3;    // UP threshold
    double holdMs = 1000.0;    // minimum time between flips
};

struct HysteresisState {
    double filtered = 1.0;
    LinkStatus status = LinkStatus::UP;
    double lastChangeMs = 0.0; // ImGui time (ms)
};

class HysteresisController {
public:
    explicit HysteresisController(const HysteresisParams& params);

    // Update internal state for all links and optionally write filtered values back to graph
    void apply(Graph& g, double nowMs, double dtMs);

    void setParams(const HysteresisParams& p) { params_ = p; }
    const HysteresisParams& params() const { return params_; }

    const HysteresisState* state(NodeId u, NodeId v) const;

private:
    struct KeyHash {
        size_t operator()(const std::pair<NodeId,NodeId>& k) const noexcept {
            return (static_cast<size_t>(k.first) << 32) ^ static_cast<size_t>(k.second);
        }
    };

    std::pair<NodeId,NodeId> norm(NodeId a, NodeId b) const {
        return (a < b) ? std::make_pair(a,b) : std::make_pair(b,a);
    }

    HysteresisParams params_;
    std::unordered_map<std::pair<NodeId,NodeId>, HysteresisState, KeyHash> linkState_;
};

} // namespace olsr



