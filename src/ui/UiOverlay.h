#pragma once

#include "core/Graph.h"
#include "route/Router.h"
#include "io/JsonExporter.h"
#include "hyst/Hysteresis.h"

#include <string>
#include <vector>

struct ImGuiContext;

namespace olsr {

struct UiEvent {
    std::string message;
};

class UiOverlay {
public:
    UiOverlay(Graph& graph, Router& router);

    void draw();

    // Keyboard actions
    void toggleSelectedLinkJam();

    // State access
    const std::vector<UiEvent>& events() const { return events_; }

private:
    void drawMenuBar();
    void drawTopologyCanvas();
    void drawInspector();
    void drawRoutingTable();
    void drawActions();
    void drawEventLog();

    void log(const std::string& msg);

    Graph& graph_;
    Router& router_;
    JsonExporter exporter_;
    HysteresisController hyst_{HysteresisParams{}};
    bool hystEnabled_ = false;

    // Selection
    NodeId selectedNode_ = 0;
    NodeId selU_ = 0, selV_ = 0; // selected link endpoints

    // Panel toggles
    bool showTopology_ = true;
    bool showInspector_ = true;
    bool showRouting_ = true;
    bool showActions_ = true;
    bool showLog_ = true;

    // Menu state
    char loadPathBuf_[256] = "assets/topologies/sample_small.json";
    char exportPathBuf_[256] = "build/routes_gui.json";

    // Actions state
    char newNodeLabel_[64] = "R";
    double newLinkWeight_ = 1.0;
    int newLinkU_ = 0;
    int newLinkV_ = 0;
    double hystAlpha_ = 0.3;
    double hystThetaUp_ = 1.6;
    double hystThetaDown_ = 1.3;
    int hystHoldMs_ = 1000;

    std::vector<UiEvent> events_;
};

} // namespace olsr


