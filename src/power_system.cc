#include "endfield_base/power_system.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace endfield_base {
namespace {
    auto isCoveredByAnyPowerPole(
        const SimulationState& state,
        const FacilityInstance& target,
        const FacilityDefinition& targetDefinition
    ) -> bool {
        const auto targetCells = state.grid.getOccupiedCells(targetDefinition, target);

        for (const FacilityInstance& candidate : state.grid.getFacilities()) {
            const FacilityDefinition* candidateDefinition = state.catalog.findDefinition(candidate.definitionId);
            if (candidateDefinition == nullptr || !isPowerFacility(*candidateDefinition)) {
                continue;
            }

            const std::unique_ptr<PowerFacilityInterface> powerFacility =
                createPowerFacilityInterface(*candidateDefinition);
            if (powerFacility == nullptr) {
                continue;
            }

            const auto poleCells = state.grid.getOccupiedCells(*candidateDefinition, candidate);
            for (const GridPoint& targetCell : targetCells) {
                for (const GridPoint& poleCell : poleCells) {
                    const int manhattanDistance = std::abs(targetCell.x - poleCell.x) + std::abs(
                                                      targetCell.y - poleCell.y);
                    if (manhattanDistance <= powerFacility->getPowerRange(*candidateDefinition)) {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    bool isGeneratingFacilityActive(const FacilityDefinition& definition, const FacilityInstance& instance) {
        if (definition.powerGeneration <= 0.0) {
            return false;
        }

        if (definition.category == FacilityCategory::Generator) {
            return true;
        }

        if (isThermalMachine(definition)) {
            return hasAnyStoredItems(instance);
        }

        return true;
    }
}  // namespace

PowerReport PowerSystem::evaluatePower(const SimulationState& state) {
    PowerReport report;

    struct PendingConsumer {
        std::size_t facilityStateIndex = 0;
        double requestedPower = 0.0;
    };

    std::vector<PendingConsumer> coveredConsumers;

    for (const FacilityInstance& instance : state.grid.getFacilities()) {
        const FacilityDefinition* definition = state.catalog.findDefinition(instance.definitionId);
        if (definition == nullptr) {
            continue;
        }

        FacilityPowerState facilityState;
        facilityState.instanceId = instance.instanceId;
        facilityState.requestedPower = definition->powerUsage;
        facilityState.covered = !requiresExternalPower(*definition)
                                || isCoveredByAnyPowerPole(state, instance, *definition);

        if (isGeneratingFacilityActive(*definition, instance)) {
            report.totalGeneration += definition->powerGeneration;
        }

        if (definition->powerUsage > 0.0) {
            report.totalRequested += definition->powerUsage;
        }

        if (!requiresExternalPower(*definition)) {
            facilityState.powered = true;
            facilityState.servedPower = definition->powerUsage;
            report.totalServed += definition->powerUsage;
        } else if (facilityState.covered) {
            coveredConsumers.push_back({report.facilityStates.size(), definition->powerUsage});
        }

        report.facilityStates.push_back(facilityState);
    }

    std::sort(
        coveredConsumers.begin(),
        coveredConsumers.end(),
        [&report] (const PendingConsumer& left, const PendingConsumer& right) {
            return report.facilityStates.at(left.facilityStateIndex).instanceId
                   < report.facilityStates.at(right.facilityStateIndex).instanceId;
        }
    );

    double remainingGeneration = report.totalGeneration;
    for (const PendingConsumer& consumer : coveredConsumers) {
        FacilityPowerState& facilityState = report.facilityStates.at(consumer.facilityStateIndex);
        if (remainingGeneration >= consumer.requestedPower) {
            facilityState.powered = true;
            facilityState.servedPower = consumer.requestedPower;
            remainingGeneration -= consumer.requestedPower;
            report.totalServed += consumer.requestedPower;
        }
    }

    return report;
}
}  // namespace endfield_base
