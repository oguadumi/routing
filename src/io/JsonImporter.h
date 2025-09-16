#pragma once

#include "core/Graph.h"
#include <string>

namespace olsr {

class JsonImporter {
public:
    // Returns true on success
    bool loadTopology(const std::string& path, Graph& g, std::string* errorMsg = nullptr);
};

} // namespace olsr


