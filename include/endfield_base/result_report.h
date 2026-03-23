#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "endfield_base/power_system.h"
#include "endfield_base/throughput_evaluator.h"

namespace endfield_base {
struct LayoutScore {
    double throughputScore = 0.0;
    double spaceEfficiency = 0.0;
    double powerEfficiency = 0.0;
    double totalScore = 0.0;
};

struct ResultSummary {
    double totalThroughput = 0.0;
    double totalProduction = 0.0;
    double totalConsumption = 0.0;
    double totalInput = 0.0;
    double totalOutput = 0.0;
    double totalGeneration = 0.0;
    double totalRequestedPower = 0.0;
    double totalServedPower = 0.0;
};

struct ResultReport {
    ResultSummary summary;
    std::vector<FacilityPowerState> powerStates;
    std::vector<FacilityThroughputResult> facilityResults;
    std::vector<NetworkPathResult> networkResults;
    LayoutScore layoutScore;
};

class SimulationState;

[[nodiscard]] auto buildResultReport(const SimulationState& state)
    -> ResultReport;
void exportResultReport(const std::filesystem::path& path, const ResultReport& report);
[[nodiscard]] auto evaluateLayout(const ResultReport& report, int mapArea, int usedCells)
    -> LayoutScore;
}  // namespace endfield_base
