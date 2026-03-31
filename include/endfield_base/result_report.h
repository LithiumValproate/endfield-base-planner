#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "endfield_base/power_system.h"
#include "endfield_base/throughput_evaluator.h"

namespace endfield_base {
// Stores the high-level layout scoring dimensions derived from a report.
struct LayoutScore {
    double throughputScore = 0.0;
    double spaceEfficiency = 0.0;
    double powerEfficiency = 0.0;
    double totalScore = 0.0;
};

// Summarizes top-level throughput and power totals for one layout.
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

// Combines all analysis outputs exported by the CLI and viewer.
struct ResultReport {
    ResultSummary summary;
    std::vector<FacilityPowerState> powerStates;
    std::vector<FacilityThroughputResult> facilityResults;
    std::vector<NetworkPathResult> networkResults;
    LayoutScore layoutScore;
};

class SimulationState;

// Builds the full analysis report for the current simulation state.
[[nodiscard]] auto buildResultReport(const SimulationState& state)
    -> ResultReport;
// Serializes a result report as JSON to disk.
void exportResultReport(const std::filesystem::path& path, const ResultReport& report);
// Derives layout scoring values from a completed result report.
[[nodiscard]] auto evaluateLayout(const ResultReport& report, int mapArea, int usedCells)
    -> LayoutScore;
}  // namespace endfield_base
