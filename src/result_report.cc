#include "endfield_base/result_report.h"

#include <algorithm>
#include <fstream>

#include <nlohmann/json.hpp>

#include "endfield_base/simulation_state.h"

namespace endfield_base {
namespace {
    nlohmann::json toJson(const FacilityPowerState& state) {
        return {
                {"instanceId", state.instanceId},
                {"covered", state.covered},
                {"powered", state.powered},
                {"requestedPower", state.requestedPower},
                {"servedPower", state.servedPower},
            };
    }

    nlohmann::json toJson(const FacilityThroughputResult& result) {
        return {
                {"instanceId", result.instanceId},
                {"definitionId", result.definitionId},
                {"powered", result.powered},
                {"throughputPerSecond", result.throughputPerSecond},
                {"productionPerSecond", result.productionPerSecond},
                {"consumptionPerSecond", result.consumptionPerSecond},
                {"utilization", result.utilization},
                {"pathLengthFromInput", result.pathLengthFromInput},
                {"pathLengthToOutput", result.pathLengthToOutput},
                {"bottleneckInstanceId", result.bottleneckInstanceId},
                {"bottleneckReason", result.bottleneckReason},
            };
    }

    nlohmann::json toJson(const NetworkPathResult& result) {
        return {
                {"fromInstanceId", result.fromInstanceId},
                {"toInstanceId", result.toInstanceId},
                {"pathLength", result.pathLength},
                {"bottleneckThroughput", result.bottleneckThroughput},
                {"pathRole", result.pathRole},
            };
    }
} // namespace

ResultReport buildResultReport(const SimulationState& state) {
    const PowerReport powerReport = PowerSystem::evaluatePower(state);
    const ThroughputReport throughputReport = ThroughputEvaluator::evaluateThroughput(state);

    ResultReport report;
    report.summary.totalThroughput = throughputReport.totalThroughput;
    report.summary.totalProduction = throughputReport.totalProduction;
    report.summary.totalConsumption = throughputReport.totalConsumption;
    report.summary.totalInput = throughputReport.totalInput;
    report.summary.totalOutput = throughputReport.totalOutput;
    report.summary.totalGeneration = powerReport.totalGeneration;
    report.summary.totalRequestedPower = powerReport.totalRequested;
    report.summary.totalServedPower = powerReport.totalServed;
    report.powerStates = powerReport.facilityStates;
    report.facilityResults = throughputReport.facilityResults;
    report.networkResults = throughputReport.networkResults;

    int usedCells = 0;
    for (int y = 0; y < state.grid.getHeight(); ++y) {
        for (int x = 0; x < state.grid.getWidth(); ++x) {
            if (const CellOccupancy* occupancy = state.grid.getCell(x, y); occupancy != nullptr
                                                                           && (occupancy->baseInstanceId.has_value() ||
                                                                               occupancy->bridgeInstanceId.
                                                                               has_value())) {
                ++usedCells;
            }
        }
    }

    report.layoutScore = evaluateLayout(
        report,
        state.grid.getWidth() * state.grid.getHeight(),
        usedCells
    );
    return report;
}

void exportResultReport(const std::filesystem::path& path, const ResultReport& report) {
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }

    nlohmann::json json;
    json["summary"] = {
            {"totalThroughput", report.summary.totalThroughput},
            {"totalProduction", report.summary.totalProduction},
            {"totalConsumption", report.summary.totalConsumption},
            {"totalInput", report.summary.totalInput},
            {"totalOutput", report.summary.totalOutput},
            {"totalGeneration", report.summary.totalGeneration},
            {"totalRequestedPower", report.summary.totalRequestedPower},
            {"totalServedPower", report.summary.totalServedPower},
        };

    json["facilityResults"] = nlohmann::json::array();
    for (const FacilityThroughputResult& facilityResult : report.facilityResults) {
        json["facilityResults"].push_back(toJson(facilityResult));
    }

    json["powerStates"] = nlohmann::json::array();
    for (const FacilityPowerState& powerState : report.powerStates) {
        json["powerStates"].push_back(toJson(powerState));
    }

    json["networkResults"] = nlohmann::json::array();
    for (const NetworkPathResult& networkResult : report.networkResults) {
        json["networkResults"].push_back(toJson(networkResult));
    }

    json["layoutScore"] = {
            {"throughputScore", report.layoutScore.throughputScore},
            {"spaceEfficiency", report.layoutScore.spaceEfficiency},
            {"powerEfficiency", report.layoutScore.powerEfficiency},
            {"totalScore", report.layoutScore.totalScore},
        };

    std::ofstream output(path);
    if (!output.is_open()) {
        throw std::runtime_error("Failed to open result report output: " + path.string());
    }
    output << json.dump(2) << '\n';
}

LayoutScore evaluateLayout(const ResultReport& report, int mapArea, int usedCells) {
    LayoutScore score;
    score.throughputScore = report.summary.totalThroughput;
    score.spaceEfficiency = mapArea > 0
                                ? 1.0 - static_cast<double>(usedCells) / static_cast<double>(mapArea)
                                : 0.0;
    score.powerEfficiency = report.summary.totalRequestedPower > 0.0
                                ? report.summary.totalServedPower / report.summary.totalRequestedPower
                                : 1.0;
    score.totalScore = score.throughputScore + score.spaceEfficiency + score.powerEfficiency;
    return score;
}
} // namespace endfield_base
