#include "hyst/Hysteresis.h"

// #include <imgui.h>

namespace olsr {

HysteresisController::HysteresisController(const HysteresisParams& params)
    : params_(params) {}

const HysteresisState* HysteresisController::state(NodeId u, NodeId v) const {
    auto it = linkState_.find(norm(u,v));
    if (it == linkState_.end()) return nullptr;
    return &it->second;
}

void HysteresisController::apply(Graph& g, double nowMs, double dtMs) {
    for (auto& l : g.links()) {
        auto key = norm(l.u, l.v);
        auto& st = linkState_[key];
        if (st.filtered == 0.0) st.filtered = l.weight; // initialize lazily
        
        // Skip hysteresis for manually jammed links
        if (l.manually_jammed) {
            l.weight = st.filtered; // still apply filtered weight for display
            continue;
        }
        
        // EMA filter
        st.filtered = params_.alpha * l.weight + (1.0 - params_.alpha) * st.filtered;

        // Status transitions with thresholds and hold-down
        bool canFlip = (nowMs - st.lastChangeMs) >= params_.holdMs;
        if (st.status == LinkStatus::UP) {
            if (st.filtered >= params_.thetaUp && canFlip) {
                st.status = LinkStatus::DOWN;
                st.lastChangeMs = nowMs;
            }
        } else {
            if (st.filtered <= params_.thetaDown && canFlip) {
                st.status = LinkStatus::UP;
                st.lastChangeMs = nowMs;
            }
        }

        // Apply to graph copy (do not overwrite original weight except for status)
        l.status = st.status;
        l.weight = st.filtered;
    }
}

} // namespace olsr



