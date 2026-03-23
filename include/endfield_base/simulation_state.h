#pragma once

#include <filesystem>
#include <string>

#include "endfield_base/facility.h"
#include "endfield_base/grid_map.h"

namespace endfield_base {
struct SimulationState {
    FacilityCatalog catalog;
    GridMap grid;
    std::string facilityCatalogPath;
    std::filesystem::path loadedFrom;
};
}  // namespace endfield_base
