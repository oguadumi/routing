#include "ui/UiOverlay.h"

#include <imgui.h>
#include <algorithm> 
#include <chrono>
#include "io/JsonImporter.h"

namespace olsr {

UiOverlay::UiOverlay(Graph& graph, Router& router)
    : graph_(graph), router_(router) {}

void UiOverlay::log(const std::string& msg) {
    events_.push_back(UiEvent{msg});
    if (events_.size() > 1000) events_.erase(events_.begin());
}

void UiOverlay::draw() {
    // If hysteresis enabled, apply on a working copy inside graph (weights/status overwritten with filtered)
    if (hystEnabled_) {
        double nowMs = ImGui::GetTime() * 1000.0;
        static double prevMs = nowMs;
        double dtMs = nowMs - prevMs;
        prevMs = nowMs;
        hyst_.apply(graph_, nowMs, dtMs);
    }
    drawMenuBar();
    if (showActions_) drawActions();
    if (showTopology_) drawTopologyCanvas();
    if (showInspector_) drawInspector();
    if (showRouting_) drawRoutingTable();
    if (showLog_) drawEventLog();
}

void UiOverlay::drawMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::InputText("Load topo", loadPathBuf_, sizeof(loadPathBuf_));
            if (ImGui::MenuItem("Load")) {
                JsonImporter imp; std::string err;
                Graph newG;
                if (imp.loadTopology(loadPathBuf_, newG, &err)) {
                    graph_ = newG;
                    router_.recomputeAll(graph_);
                    log(std::string("Loaded topo: ") + loadPathBuf_);
                } else {
                    log(std::string("Load failed: ") + err);
                }
            }
            ImGui::Separator();
            ImGui::InputText("Export to", exportPathBuf_, sizeof(exportPathBuf_));
            if (ImGui::MenuItem("Export routes")) {
                if (exporter_.exportRoutes(graph_, router_, exportPathBuf_)) {
                    log(std::string("Exported routes: ") + exportPathBuf_);
                } else {
                    log(std::string("Export failed: ") + exportPathBuf_);
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Topology", nullptr, &showTopology_);
            ImGui::MenuItem("Inspector", nullptr, &showInspector_);
            ImGui::MenuItem("Routing Table", nullptr, &showRouting_);
            ImGui::MenuItem("Actions", nullptr, &showActions_);
            ImGui::MenuItem("Event Log", nullptr, &showLog_);
            ImGui::Separator();
            ImGui::MenuItem("Enable Hysteresis", nullptr, &hystEnabled_);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void UiOverlay::drawActions() {
    ImGui::Begin("Actions");
    if (ImGui::Button("Recompute")) {
        auto start = std::chrono::high_resolution_clock::now();
        router_.recomputeAll(graph_);
        auto end = std::chrono::high_resolution_clock::now();
        auto dur_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        if (dur_us >= 1000) {
            log("Recomputed routes (" + std::to_string(dur_us / 1000) + " ms)");
        } else {
            log("Recomputed routes (" + std::to_string(dur_us) + " micro-s)");
        }
    }
    ImGui::SameLine();
    ImGui::InputText("Export path", exportPathBuf_, sizeof(exportPathBuf_));
    ImGui::SameLine();
    if (ImGui::Button("Export JSON")) {
        if (exporter_.exportRoutes(graph_, router_, exportPathBuf_)) {
            log(std::string("Exported routes: ") + exportPathBuf_);
        } else {
            log(std::string("Export failed: ") + exportPathBuf_);
        }
    }

    ImGui::Separator();
    if (ImGui::CollapsingHeader("Topology Management", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::InputText("New node label", newNodeLabel_, sizeof(newNodeLabel_));
        if (ImGui::Button("Add Node")) {
            float x = 100.0f + 20.0f * (float)graph_.nodes().size();
            float y = 100.0f;
            NodeId nid = graph_.addNode(newNodeLabel_, x, y);
            router_.recomputeAll(graph_);
            log("Added node id=" + std::to_string(nid));
        }
        ImGui::InputInt("Link u", &newLinkU_);
        ImGui::InputInt("Link v", &newLinkV_);
        ImGui::InputDouble("Link weight", &newLinkWeight_);
        if (ImGui::Button("Add Link")) {
            if (graph_.addLink((NodeId)newLinkU_, (NodeId)newLinkV_, newLinkWeight_)) {
                router_.recomputeAll(graph_);
                log("Added link");
            } else {
                log("Add link failed (invalid or duplicate)");
            }
        }
        if (selectedNode_) {
            if (ImGui::Button("Delete Selected Node")) {
                graph_.removeNode(selectedNode_);
                selectedNode_ = 0; selU_ = selV_ = 0;
                router_.recomputeAll(graph_);
                log("Deleted node");
            }
        }
        if (selU_ && selV_) {
            if (ImGui::Button("Delete Selected Link")) {
                // No explicit remove API; emulate by rebuilding links sans selected
                auto& links = graph_.links();
                links.erase(std::remove_if(links.begin(), links.end(), [&](const Link& l){ return (l.u == selU_ && l.v == selV_) || (l.u == selV_ && l.v == selU_); }), links.end());
                selU_ = selV_ = 0;
                router_.recomputeAll(graph_);
                log("Deleted link");
            }
        }
    }

    ImGui::Separator();
    if (ImGui::CollapsingHeader("Hysteresis", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enable Hysteresis", &hystEnabled_);
        ImGui::InputDouble("alpha", &hystAlpha_);
        ImGui::InputDouble("theta_up", &hystThetaUp_);
        ImGui::InputDouble("theta_down", &hystThetaDown_);
        ImGui::InputInt("hold_ms", &hystHoldMs_);
        if (ImGui::Button("Apply Hysteresis Params")) {
            HysteresisParams p; p.alpha = hystAlpha_; p.thetaUp = hystThetaUp_; p.thetaDown = hystThetaDown_; p.holdMs = (double)hystHoldMs_;
            hyst_.setParams(p);
            log("Applied hysteresis params");
        }
    }
    if (selU_ && selV_) {
        const Link* l = graph_.findLink(selU_, selV_);
        if (l) {
            if (l->status == LinkStatus::UP) {
                if (ImGui::Button("Jam Link")) {
                    auto start = std::chrono::high_resolution_clock::now();
                    graph_.setLinkStatus(selU_, selV_, LinkStatus::DOWN);
                    router_.recomputeAll(graph_);
                    auto end = std::chrono::high_resolution_clock::now();
                    auto dur_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                    if (dur_us >= 1000) {
                        log("Link jammed (recompute " + std::to_string(dur_us / 1000) + " ms)");
                    } else {
                        log("Link jammed (recompute " + std::to_string(dur_us) + " micro-s)");
                    }
                }
            } else {
                if (ImGui::Button("Unjam Link")) {
                    auto start = std::chrono::high_resolution_clock::now();
                    graph_.setLinkStatus(selU_, selV_, LinkStatus::UP);
                    router_.recomputeAll(graph_);
                    auto end = std::chrono::high_resolution_clock::now();
                    auto dur_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                    if (dur_us >= 1000) {
                        log("Link unjammed (recompute " + std::to_string(dur_us / 1000) + " ms)");
                    } else {
                        log("Link unjammed (recompute " + std::to_string(dur_us) + " micro-s)");
                    }
                }
            }
        }
    }
    ImGui::End();
}

void UiOverlay::drawTopologyCanvas() {
    ImGui::Begin("Topology");
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    drawList->AddRectFilled(origin, ImVec2(origin.x + canvasSize.x, origin.y + canvasSize.y), IM_COL32(30,30,30,255));
    ImGui::InvisibleButton("canvas", canvasSize);
    const bool isHovered = ImGui::IsItemHovered();

    // Draw links
    for (const auto& l : graph_.links()) {
        const Node* nu = nullptr;
        const Node* nv = nullptr;
        for (const auto& n : graph_.nodes()) {
            if (n.id == l.u) nu = &n;
            if (n.id == l.v) nv = &n;
        }
        if (!nu || !nv) continue;
        ImU32 col = (l.status == LinkStatus::UP) ? IM_COL32(0,200,0,255) : IM_COL32(200,0,0,255);
        drawList->AddLine(ImVec2(origin.x + nu->x, origin.y + nu->y), ImVec2(origin.x + nv->x, origin.y + nv->y), col, 2.0f);
    }

    // Draw nodes
    const float r = 12.0f;
    for (const auto& n : graph_.nodes()) {
        ImVec2 p(origin.x + n.x, origin.y + n.y);
        drawList->AddCircleFilled(p, r, IM_COL32(80, 140, 250, 255));
        drawList->AddText(ImVec2(p.x + r + 4, p.y - r), IM_COL32(255,255,255,255), n.label.c_str());
        // selection
        if (isHovered && ImGui::IsMouseClicked(0)) {
            ImVec2 mp = ImGui::GetIO().MousePos;
            float dx = mp.x - p.x, dy = mp.y - p.y;
            if (dx*dx + dy*dy <= r*r) {
                selectedNode_ = n.id;
                selU_ = selV_ = 0;
            }
        }
    }

    // Select links by proximity to segment
    if (isHovered && ImGui::IsMouseClicked(0)) {
        ImVec2 mp = ImGui::GetIO().MousePos;
        float bestDist2 = 25.0f; // px^2 threshold
        NodeId bu = 0, bv = 0;
        for (const auto& l : graph_.links()) {
            const Node* nu = nullptr; const Node* nv = nullptr;
            for (const auto& n : graph_.nodes()) {
                if (n.id == l.u) nu = &n; if (n.id == l.v) nv = &n;
            }
            if (!nu || !nv) continue;
            ImVec2 a(origin.x + nu->x, origin.y + nu->y);
            ImVec2 b(origin.x + nv->x, origin.y + nv->y);
            // point-to-segment distance^2
            ImVec2 ab(b.x - a.x, b.y - a.y);
            ImVec2 ap(mp.x - a.x, mp.y - a.y);
            float t = (ab.x*ap.x + ab.y*ap.y);
            float denom = (ab.x*ab.x + ab.y*ab.y);
            if (denom > 0.0f) t /= denom; else t = 0.0f;
            if (t < 0.0f) t = 0.0f; if (t > 1.0f) t = 1.0f;
            ImVec2 proj(a.x + ab.x*t, a.y + ab.y*t);
            float dx = mp.x - proj.x, dy = mp.y - proj.y;
            float d2 = dx*dx + dy*dy;
            if (d2 < bestDist2) { bestDist2 = d2; bu = l.u; bv = l.v; }
        }
        if (bu && bv) { selU_ = bu; selV_ = bv; selectedNode_ = 0; }
    }

    // Drag nodes
    if (selectedNode_ && isHovered && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        ImVec2 delta = ImGui::GetIO().MouseDelta;
        for (auto& n : const_cast<std::vector<Node>&>(graph_.nodes())) {
            if (n.id == selectedNode_) { n.x += delta.x; n.y += delta.y; }
        }
    }

    ImGui::End();
}

void UiOverlay::drawInspector() {
    ImGui::Begin("Inspector");
    if (selectedNode_) {
        const Node* sel = nullptr;
        for (const auto& n : graph_.nodes()) if (n.id == selectedNode_) sel = &n;
        if (sel) {
            ImGui::Text("Node %u", sel->id);
            ImGui::Text("Label: %s", sel->label.c_str());
            // degree
            int degree = 0;
            for (const auto& l : graph_.links()) if (l.u == sel->id || l.v == sel->id) ++degree;
            ImGui::Text("Degree: %d", degree);
            // routes count
            const RouteTable* tbl = router_.table(sel->id);
            int rc = tbl ? (int)tbl->size() : 0;
            ImGui::Text("Routes: %d", rc);
        }
    } else if (selU_ && selV_) {
        const Link* l = graph_.findLink(selU_, selV_);
        if (l) {
            ImGui::Text("Link (%u,%u)", l->u, l->v);
            double w = l->weight;
            if (ImGui::InputDouble("Weight", &w)) {
                graph_.setLinkWeight(l->u, l->v, w);
                auto start = std::chrono::high_resolution_clock::now();
                router_.recomputeAll(graph_);
                auto end = std::chrono::high_resolution_clock::now();
                auto dur_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
                if (dur_us >= 1000) {
                    log("Weight edited; recomputed (" + std::to_string(dur_us / 1000) + " ms)");
                } else {
                    log("Weight edited; recomputed (" + std::to_string(dur_us) + " micro-s)");
                }
            }
            ImGui::Text("Status: %s", l->status == LinkStatus::UP ? "UP" : "DOWN");
            if (hystEnabled_) {
                if (const HysteresisState* hs = hyst_.state(l->u, l->v)) {
                    ImGui::Text("Filtered: %.3f", hs->filtered);
                }
            }
        }
    } else {
        ImGui::Text("Select a node or link");
    }
    ImGui::End();
}

void UiOverlay::drawRoutingTable() {
    ImGui::Begin("Routing Table");
    static int srcIndex = 0;
    if ((int)graph_.nodes().size() > 0) {
        if (srcIndex >= (int)graph_.nodes().size()) srcIndex = 0;
        std::vector<const char*> labels;
        labels.reserve(graph_.nodes().size());
        for (const auto& n : graph_.nodes()) labels.push_back(n.label.c_str());
        ImGui::Combo("Source", &srcIndex, labels.data(), (int)labels.size());
        NodeId src = graph_.nodes()[srcIndex].id;
        const RouteTable* tbl = router_.table(src);
        if (tbl) {
            if (ImGui::BeginTable("rt", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Dest");
                ImGui::TableSetupColumn("Next Hop");
                ImGui::TableSetupColumn("Cost");
                ImGui::TableSetupColumn("Hops");
                ImGui::TableHeadersRow();
                for (const auto& e : *tbl) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%u", e.destination);
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%u", e.next_hop);
                    ImGui::TableSetColumnIndex(2); ImGui::Text("%.3f", e.total_cost);
                    ImGui::TableSetColumnIndex(3); ImGui::Text("%u", e.hop_count);
                }
                ImGui::EndTable();
            }
        }
    }
    ImGui::End();
}

void UiOverlay::drawEventLog() {
    ImGui::Begin("Event Log");
    for (const auto& e : events_) ImGui::TextUnformatted(e.message.c_str());
    ImGui::End();
}

void UiOverlay::toggleSelectedLinkJam() {
    if (!(selU_ && selV_)) return;
    const Link* l = graph_.findLink(selU_, selV_);
    if (!l) return;
    auto start = std::chrono::high_resolution_clock::now();
    if (l->status == LinkStatus::UP) {
        graph_.setLinkStatus(selU_, selV_, LinkStatus::DOWN);
        router_.recomputeAll(graph_);
        auto end = std::chrono::high_resolution_clock::now();
        auto dur_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        if (dur_us >= 1000) {
            log("Link jammed (recompute " + std::to_string(dur_us / 1000) + " ms)");
        } else {
            log("Link jammed (recompute " + std::to_string(dur_us) + " micro-s)");
        }
    } else {
        graph_.setLinkStatus(selU_, selV_, LinkStatus::UP);
        router_.recomputeAll(graph_);
        auto end = std::chrono::high_resolution_clock::now();
        auto dur_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        if (dur_us >= 1000) {
            log("Link unjammed (recompute " + std::to_string(dur_us / 1000) + " ms)");
        } else {
            log("Link unjammed (recompute " + std::to_string(dur_us) + " micro-s)");
        }
    }
}

} // namespace olsr


