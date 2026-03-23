#pragma once

#include <filesystem>

#include "endfield_base/result_report.h"
#include "endfield_base/simulation_state.h"

namespace endfield_base {
[[nodiscard]] auto loadFacilityDefinitions(const std::filesystem::path& path)
    -> FacilityCatalog;
[[nodiscard]] auto loadMap(const std::filesystem::path& path)
    -> SimulationState;
void saveMap(const std::filesystem::path& path, const SimulationState& state);
}  // namespace endfield_base
