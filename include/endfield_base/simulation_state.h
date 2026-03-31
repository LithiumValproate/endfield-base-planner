#pragma once

#include <filesystem>
#include <string>

#include "endfield_base/facility.h"
#include "endfield_base/grid_map.h"

namespace endfield_base {
// Owns all mutable state required to analyze or edit one map.
struct SimulationState {
    FacilityCatalog catalog;
    GridMap grid;
    std::string facilityCatalogPath;
    std::filesystem::path loadedFrom;
};
}  // namespace endfield_base
