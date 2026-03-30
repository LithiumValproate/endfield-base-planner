#pragma once

#include <string>
#include <vector>

#include "endfield_base/path_finder.h"
#include "endfield_base/power_system.h"
#include "endfield_base/simulation_state.h"

namespace endfield_base {
struct FacilityThroughputResult {
    int instanceId = 0;
    std::string definitionId;
    bool powered = false;
    double throughputPerSecond = 0.0;
    double productionPerSecond = 0.0;
    double consumptionPerSecond = 0.0;
    double utilization = 0.0;
    int pathLengthFromInput = -1;
    int pathLengthToOutput = -1;
    int bottleneckInstanceId = 0;
    std::string bottleneckReason;
};

struct NetworkPathResult {
    int fromInstanceId = 0;
    int toInstanceId = 0;
    int pathLength = 0;
    double bottleneckThroughput = 0.0;
    std::string pathRole;
};

struct ResolvedPath {
    int targetInstanceId = 0;
    PathResult path;
};

struct ThroughputReport {
    double totalThroughput = 0.0;
    double totalProduction = 0.0;
    double totalConsumption = 0.0;
    double totalInput = 0.0;
    double totalOutput = 0.0;
    std::vector<FacilityThroughputResult> facilityResults;
    std::vector<NetworkPathResult> networkResults;
};

class ThroughputEvaluator {
public:
    [[nodiscard]] static auto evaluateThroughput(const SimulationState& state)
        -> ThroughputReport;
};
}  // namespace endfield_base
