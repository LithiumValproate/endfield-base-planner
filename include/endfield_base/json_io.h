#pragma once

#include <filesystem>

#include "endfield_base/result_report.h"
#include "endfield_base/simulation_state.h"

namespace endfield_base {
// Loads every facility definition from a JSON catalog file.
[[nodiscard]] auto loadFacilityDefinitions(const std::filesystem::path& path)
    -> FacilityCatalog;
// Loads a map JSON file together with its referenced facility catalog.
[[nodiscard]] auto loadMap(const std::filesystem::path& path)
    -> SimulationState;
// Saves the current simulation map state back to JSON.
void saveMap(const std::filesystem::path& path, const SimulationState& state);
}  // namespace endfield_base
