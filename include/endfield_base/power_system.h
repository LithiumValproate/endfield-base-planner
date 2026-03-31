#pragma once

#include <string>
#include <vector>

#include "endfield_base/simulation_state.h"

namespace endfield_base {
// Reports power coverage and served power for one facility instance.
struct FacilityPowerState {
    int instanceId = 0;
    bool covered = false;
    bool powered = false;
    double requestedPower = 0.0;
    double servedPower = 0.0;
};

// Aggregates power totals and per-facility power states for a map.
struct PowerReport {
    double totalGeneration = 0.0;
    double totalRequested = 0.0;
    double totalServed = 0.0;
    std::vector<FacilityPowerState> facilityStates;
};

// Evaluates power generation, coverage, and served demand across the map.
class PowerSystem {
public:
    // Runs a full power analysis for the current simulation state.
    [[nodiscard]] static auto evaluatePower(const SimulationState& state)
        -> PowerReport;
};
}  // namespace endfield_base
