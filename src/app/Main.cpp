#include "core/Graph.h"
#include "route/Router.h"
#include "io/JsonImporter.h"
#include "io/JsonExporter.h"

#include <iostream>
#include <string>

using namespace olsr;

extern int run_gui(int argc, char** argv);

int main(int argc, char** argv) {
    std::string topoPath;
    std::string exportPath;
    bool noGui = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--topo" && i + 1 < argc) {
            topoPath = argv[++i];
        } else if (arg == "--export" && i + 1 < argc) {
            exportPath = argv[++i];
        } else if (arg == "--no-gui") {
            noGui = true;
        }
    }

    if (!noGui) {
        return run_gui(argc, argv);
    }

    Graph g;
    if (!topoPath.empty()) {
        JsonImporter imp;
        std::string err;
        if (!imp.loadTopology(topoPath, g, &err)) {
            std::cerr << "Error loading topology: " << err << "\n";
            return 1;
        }
    } else {
        // Minimal default graph: 2 nodes, 1 link
        auto n1 = g.addNode("R1", 100, 100);
        auto n2 = g.addNode("R2", 200, 100);
        g.addLink(n1, n2, 1.0);
    }

    Router router;
    router.recomputeAll(g);

    if (!exportPath.empty()) {
        JsonExporter exp;
        if (!exp.exportRoutes(g, router, exportPath)) {
            std::cerr << "Export failed: " << exportPath << "\n";
            return 2;
        }
        std::cout << "Exported routes to " << exportPath << "\n";
    } else {
        // Print routing table for node 1 if exists
        if (!g.nodes().empty()) {
            NodeId src = g.nodes().front().id;
            auto tbl = router.table(src);
            if (tbl) {
                std::cout << "Routes from node " << src << ":\n";
                for (const auto& e : *tbl) {
                    std::cout << "  dest=" << e.destination << " next=" << e.next_hop
                              << " cost=" << e.total_cost << " hops=" << e.hop_count << "\n";
                }
            }
        }
    }

    return 0;
}


