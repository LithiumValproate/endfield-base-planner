#pragma once

#include <vector>

#include "endfield_base/simulation_state.h"

namespace endfield_base {
struct StorageEndpoints {
    std::vector<int> inputInstanceIds;
    std::vector<int> outputInstanceIds;
};

class StorageSystem {
public:
    [[nodiscard]] static auto collectEndpoints(const SimulationState& state)
        -> StorageEndpoints;
};
}  // namespace endfield_base
