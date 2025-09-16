#include "io/JsonImporter.h"

#include <nlohmann/json.hpp>
#include <fstream>

namespace olsr {

using nlohmann::json;

bool JsonImporter::loadTopology(const std::string& path, Graph& g, std::string* errorMsg) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        if (errorMsg) *errorMsg = "Failed to open topology file";
        return false;
    }
    json j;
    try {
        ifs >> j;
    } catch (const std::exception& e) {
        if (errorMsg) *errorMsg = std::string("Invalid JSON: ") + e.what();
        return false;
    }

    try {
        std::vector<NodeId> idMap; // 1-based index -> assigned id
        idMap.resize(1);
        if (j.contains("nodes")) {
            for (const auto& n : j.at("nodes")) {
                NodeId id = static_cast<NodeId>(n.at("id").get<int>());
                std::string label = n.value("label", std::to_string(id));
                float x = n.value("x", 0.0f);
                float y = n.value("y", 0.0f);
                while (idMap.size() <= id) idMap.push_back(0);
                NodeId assigned = g.addNode(label, x, y);
                idMap[id] = assigned;
            }
        }
        if (j.contains("links")) {
            for (const auto& l : j.at("links")) {
                NodeId u = static_cast<NodeId>(l.at("u").get<int>());
                NodeId v = static_cast<NodeId>(l.at("v").get<int>());
                double w = l.value("weight", 1.0);
                if (u < idMap.size() && v < idMap.size()) {
                    NodeId uu = idMap[u];
                    NodeId vv = idMap[v];
                    g.addLink(uu, vv, w);
                }
            }
        }
    } catch (const std::exception& e) {
        if (errorMsg) *errorMsg = std::string("Topology parse error: ") + e.what();
        return false;
    }
    return true;
}

} // namespace olsr


