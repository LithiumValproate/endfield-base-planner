#pragma once

#include <optional>
#include <vector>

#include "endfield_base/facility.h"
#include "endfield_base/simulation_state.h"

namespace endfield_base {
// Describes one path search request between two facility instances.
struct PathRequest {
    int startInstanceId = 0;
    int endInstanceId = 0;
};

// Records one visited step in a resolved logistics path.
struct PathStep {
    GridPoint position;
    bool bridgeLayer = false;
    std::optional<int> ownerInstanceId;
};

// Stores the outcome of a path search through the logistics graph.
struct PathResult {
    bool found = false;
    int length = 0;
    double bottleneckThroughput = 0.0;
    std::vector<PathStep> steps;
    std::vector<int> traversedInstanceIds;
};

// Finds logistics paths between facility instances.
class PathFinder {
public:
    // Resolves the best available path for the given request.
    [[nodiscard]] static auto findPath(const SimulationState& state, const PathRequest& request)
        -> PathResult;
};
}  // namespace endfield_base
