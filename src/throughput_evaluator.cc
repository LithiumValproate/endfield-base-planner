#include "endfield_base/throughput_evaluator.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

#include "endfield_base/storage_system.h"

namespace endfield_base {
namespace {
    FacilityPowerState findPowerState(const PowerReport& powerReport, int instanceId) {
        const auto it = std::find_if(
            powerReport.facilityStates.begin(),
            powerReport.facilityStates.end(),
            [instanceId] (const FacilityPowerState& state) { return state.instanceId == instanceId; }
        );
        if (it == powerReport.facilityStates.end()) {
            return {};
        }
        return *it;
    }

    double pathPenaltyMultiplier(int pathLength) {
        if (pathLength <= 1) {
            return 1.0;
        }
        return 1.0 + 0.25 * static_cast<double>(pathLength - 1);
    }

    auto resolveBestPath(
        const SimulationState& state,
        int startInstanceId,
        const std::vector<int>& targets
    ) -> std::optional<std::pair<int, PathResult>> {
        std::optional<std::pair<int, PathResult>> best;

        for (const int targetInstanceId : targets) {
            PathResult path = PathFinder::findPath(state, {startInstanceId, targetInstanceId});
            if (!path.found) {
                continue;
            }

            if (!best.has_value() || path.length < best->second.length) {
                best = std::make_pair(targetInstanceId, std::move(path));
            }
        }

        return best;
    }

    auto accumulatePathUsage(
        std::unordered_map<int, double>& usageByInstanceId,
        const PathResult& path,
        double throughput
    ) -> void {
        for (const int instanceId : path.traversedInstanceIds) {
            usageByInstanceId[instanceId] += throughput;
        }
    }
}  // namespace

ThroughputReport ThroughputEvaluator::evaluateThroughput(const SimulationState& state) {
    ThroughputReport report;
    const PowerReport powerReport = PowerSystem::evaluatePower(state);
    StorageEndpoints endpoints = StorageSystem::collectEndpoints(state);
    std::unordered_map<int, double> pathUsageByInstanceId;

    const auto isActiveInstance = [&powerReport, &state] (int instanceId) {
        const FacilityInstance* instance = state.grid.findFacility(instanceId);
        if (instance == nullptr) {
            return false;
        }
        const FacilityDefinition* definition = state.catalog.findDefinition(instance->definitionId);
        if (definition == nullptr) {
            return false;
        }
        const FacilityPowerState powerState = findPowerState(powerReport, instanceId);
        return powerState.powered || !requiresExternalPower(*definition);
    };

    std::erase_if(
        endpoints.inputInstanceIds,
        [&isActiveInstance] (int instanceId) { return !isActiveInstance(instanceId); }
    );
    std::erase_if(
        endpoints.outputInstanceIds,
        [&isActiveInstance] (int instanceId) { return !isActiveInstance(instanceId); }
    );

    for (const FacilityInstance& instance : state.grid.getFacilities()) {
        const FacilityDefinition* definition = state.catalog.findDefinition(instance.definitionId);
        if (definition == nullptr) {
            continue;
        }

        FacilityThroughputResult facilityResult;
        facilityResult.instanceId = instance.instanceId;
        facilityResult.definitionId = definition->id;
        facilityResult.powered = findPowerState(powerReport, instance.instanceId).powered;

        const bool active = facilityResult.powered || !requiresExternalPower(*definition);
        const double localLimit = std::max({
                definition->baseThroughput,
                definition->productionRate,
                definition->consumptionRate,
                0.0,
            });

        if (isProductionFacility(*definition) || isThermalMachine(*definition)) {
            double outputRate = 0.0;
            double inputRate = 0.0;

            if (active && definition->productionRate > 0.0) {
                if (const auto outputPath = resolveBestPath(state, instance.instanceId, endpoints.outputInstanceIds); outputPath.has_value()) {
                    const double transportLimit =
                        outputPath->second.bottleneckThroughput / pathPenaltyMultiplier(outputPath->second.length);
                    outputRate = std::min({definition->productionRate, definition->baseThroughput, transportLimit});
                    facilityResult.pathLengthToOutput = outputPath->second.length;
                    facilityResult.bottleneckInstanceId =
                        outputPath->second.traversedInstanceIds.empty()
                            ? 0
                            : outputPath->second.traversedInstanceIds.front();
                    facilityResult.bottleneckReason = "output_path";
                    report.networkResults.push_back({
                            instance.instanceId,
                            outputPath->first,
                            outputPath->second.length,
                            outputPath->second.bottleneckThroughput,
                            "output",
                        });
                    accumulatePathUsage(pathUsageByInstanceId, outputPath->second, outputRate);
                } else {
                    facilityResult.bottleneckReason = "missing_output_path";
                }
            }

            if (active && definition->consumptionRate > 0.0) {
                if (const auto inputPath = resolveBestPath(state, instance.instanceId, endpoints.inputInstanceIds); inputPath.has_value()) {
                    const double transportLimit =
                        inputPath->second.bottleneckThroughput / pathPenaltyMultiplier(inputPath->second.length);
                    inputRate = std::min({definition->consumptionRate, definition->baseThroughput, transportLimit});
                    facilityResult.pathLengthFromInput = inputPath->second.length;
                    if (facilityResult.bottleneckReason.empty()) {
                        facilityResult.bottleneckInstanceId =
                            inputPath->second.traversedInstanceIds.empty()
                                ? 0
                                : inputPath->second.traversedInstanceIds.front();
                        facilityResult.bottleneckReason = "input_path";
                    }
                    report.networkResults.push_back({
                            inputPath->first,
                            instance.instanceId,
                            inputPath->second.length,
                            inputPath->second.bottleneckThroughput,
                            "input",
                        });
                    accumulatePathUsage(pathUsageByInstanceId, inputPath->second, inputRate);
                } else if (facilityResult.bottleneckReason.empty()) {
                    facilityResult.bottleneckReason = "missing_input_path";
                }
            }

            if (definition->productionRate > 0.0 && definition->consumptionRate > 0.0) {
                facilityResult.throughputPerSecond = std::min(inputRate, outputRate);
                facilityResult.productionPerSecond = facilityResult.throughputPerSecond;
                facilityResult.consumptionPerSecond = facilityResult.throughputPerSecond;
            } else {
                facilityResult.productionPerSecond = outputRate;
                facilityResult.consumptionPerSecond = inputRate;
                facilityResult.throughputPerSecond = std::max(outputRate, inputRate);
            }
        } else if (isChestFacility(*definition)) {
            if (usesWirelessStorage(*definition, instance) && active) {
                facilityResult.throughputPerSecond = hasAnyStoredItems(instance)
                                                         ? std::max(definition->baseThroughput, 1.0)
                                                         : 0.0;
                facilityResult.productionPerSecond = facilityResult.throughputPerSecond;
                facilityResult.bottleneckReason = "wireless_storage";
            } else {
                facilityResult.bottleneckReason = "buffer_storage";
            }
        } else if (isHubFacility(*definition) || definition->category == FacilityCategory::Storage_port) {
            facilityResult.bottleneckReason = "warehouse_link";
        } else if (isLogisticsFacility(definition->category)) {
            facilityResult.bottleneckReason = "path_carrier";
        } else if (definition->category == FacilityCategory::Generator || isThermalMachine(*definition)) {
            facilityResult.bottleneckReason = "power_generation";
        }

        if (localLimit > 0.0) {
            facilityResult.utilization = facilityResult.throughputPerSecond / localLimit;
        }

        report.totalThroughput += facilityResult.throughputPerSecond;
        report.totalProduction += facilityResult.productionPerSecond;
        report.totalConsumption += facilityResult.consumptionPerSecond;
        report.facilityResults.push_back(facilityResult);
    }

    for (FacilityThroughputResult& facilityResult : report.facilityResults) {
        const FacilityDefinition* definition = state.catalog.findDefinition(facilityResult.definitionId);
        if (definition == nullptr || !isLogisticsFacility(definition->category)) {
            continue;
        }

        const double usedRate = pathUsageByInstanceId[facilityResult.instanceId];
        facilityResult.throughputPerSecond = usedRate;
        facilityResult.productionPerSecond = usedRate;
        facilityResult.utilization = definition->baseThroughput > 0.0 ? usedRate / definition->baseThroughput : 0.0;
        report.totalInput += usedRate;
        report.totalOutput += usedRate;
    }

    report.totalInput += report.totalConsumption;
    report.totalOutput += report.totalProduction;

    return report;
}
}  // namespace endfield_base
