#pragma once

#include <string>
#include <vector>

#include "endfield_base/simulation_state.h"

namespace endfield_base {
struct FacilityPowerState {
    int instanceId = 0;
    bool covered = false;
    bool powered = false;
    double requestedPower = 0.0;
    double servedPower = 0.0;
};

struct PowerReport {
    double totalGeneration = 0.0;
    double totalRequested = 0.0;
    double totalServed = 0.0;
    std::vector<FacilityPowerState> facilityStates;
};

class PowerSystem {
public:
    [[nodiscard]] static auto evaluatePower(const SimulationState& state)
        -> PowerReport;
};
}  // namespace endfield_base
