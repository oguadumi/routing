#include "io/JsonExporter.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <chrono>

namespace olsr {

using nlohmann::json;

static const char* statusToString(LinkStatus st) {
    return st == LinkStatus::UP ? "UP" : "DOWN";
}

bool JsonExporter::exportRoutes(const Graph& g, const Router& r, const std::string& path) {
    json j;
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    j["meta"] = {
        {"version", "1.0.0"},
        {"timestamp_ms", now_ms}
    };

    j["nodes"] = json::array();
    for (const auto& n : g.nodes()) {
        j["nodes"].push_back({{"id", n.id}, {"label", n.label}});
    }

    j["links"] = json::array();
    for (const auto& l : g.links()) {
        j["links"].push_back({
            {"u", l.u}, {"v", l.v}, {"weight", l.weight}, {"status", statusToString(l.status)}
        });
    }

    json routesObj = json::object();
    for (const auto& n : g.nodes()) {
        const RouteTable* tbl = r.table(n.id);
        if (!tbl) continue;
        json arr = json::array();
        for (const auto& e : *tbl) {
            arr.push_back({
                {"destination", e.destination},
                {"next_hop", e.next_hop},
                {"total_cost", e.total_cost},
                {"hop_count", e.hop_count}
            });
        }
        routesObj[std::to_string(n.id)] = arr;
    }
    j["routes"] = routesObj;

    std::ofstream ofs(path, std::ios::binary);
    if (!ofs.is_open()) return false;
    ofs << j.dump(2) << '\n';
    return true;
}

} // namespace olsr


