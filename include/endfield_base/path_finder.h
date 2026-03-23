#pragma once

#include <optional>
#include <vector>

#include "endfield_base/facility.h"
#include "endfield_base/simulation_state.h"

namespace endfield_base {
struct PathRequest {
    int startInstanceId = 0;
    int endInstanceId = 0;
};

struct PathStep {
    GridPoint position;
    bool bridgeLayer = false;
    std::optional<int> ownerInstanceId;
};

struct PathResult {
    bool found = false;
    int length = 0;
    double bottleneckThroughput = 0.0;
    std::vector<PathStep> steps;
    std::vector<int> traversedInstanceIds;
};

class PathFinder {
public:
    [[nodiscard]] static auto findPath(const SimulationState& state, const PathRequest& request)
        -> PathResult;
};
}  // namespace endfield_base
