#pragma once

#include "core/Graph.h"
#include "route/Router.h"
#include <string>

namespace olsr {

class JsonExporter {
public:
    bool exportRoutes(const Graph& g, const Router& r, const std::string& path);
};

} // namespace olsr


