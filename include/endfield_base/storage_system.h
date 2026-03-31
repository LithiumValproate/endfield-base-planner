#pragma once

#include <vector>

#include "endfield_base/simulation_state.h"

namespace endfield_base {
// Collects the storage-related facility instances exposed by a layout.
struct StorageEndpoints {
    std::vector<int> inputInstanceIds;
    std::vector<int> outputInstanceIds;
};

// Extracts storage endpoint information from the simulation state.
class StorageSystem {
public:
    // Scans the layout and returns all discovered storage inputs and outputs.
    [[nodiscard]] static auto collectEndpoints(const SimulationState& state)
        -> StorageEndpoints;
};
}  // namespace endfield_base
