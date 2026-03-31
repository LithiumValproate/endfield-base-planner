#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "endfield_base/facility.h"

namespace endfield_base {
// Tracks which facility instances occupy one map cell on each layer.
struct CellOccupancy {
    std::optional<int> baseInstanceId;
    std::optional<int> bridgeInstanceId;
    std::vector<int> attachedInstanceIds;
};

// Owns all placed facilities and validates map-layer occupancy rules.
class GridMap {
public:
    GridMap() = default;
    // Creates an empty grid with the requested dimensions.
    GridMap(int width, int height);

    // Returns the current map width in cells.
    [[nodiscard]] auto getWidth() const
        -> int;
    // Returns the current map height in cells.
    [[nodiscard]] auto getHeight() const
        -> int;
    // Returns the next instance id reserved for placement.
    [[nodiscard]] auto getNextInstanceId() const
        -> int;
    // Overrides the next instance id used by future placements.
    void setNextInstanceId(int nextInstanceId);

    // Returns all placed facilities in insertion order.
    [[nodiscard]] auto getFacilities() const
        -> const std::vector<FacilityInstance>&;
    // Finds a mutable facility instance by id.
    [[nodiscard]] auto findFacility(int instanceId)
        -> FacilityInstance*;
    // Finds a read-only facility instance by id.
    [[nodiscard]] auto findFacility(int instanceId) const
        -> const FacilityInstance*;
    // Returns occupancy data for one cell or nullptr when out of bounds.
    [[nodiscard]] auto getCell(int x, int y) const
        -> const CellOccupancy*;
    // Returns every cell occupied by the given facility instance.
    [[nodiscard]] auto getOccupiedCells(
        const FacilityDefinition& definition,
        const FacilityInstance& instance
    ) const -> std::vector<GridPoint>;
    // Returns all facility ids that touch the requested cell.
    [[nodiscard]] auto getFacilityIdsAt(const GridPoint& cell) const
        -> std::vector<int>;
    // Validates whether a facility can be placed at the requested origin.
    [[nodiscard]] auto canPlaceFacility(
        const FacilityDefinition& definition,
        const GridPoint& origin,
        Rotation rotation,
        std::string* reason = nullptr
    ) const -> bool;
    // Places a facility and returns its instance id, or 0 on failure.
    [[nodiscard]] auto placeFacility(
        const FacilityDefinition& definition,
        std::string_view definitionId,
        const GridPoint& origin,
        Rotation rotation,
        std::optional<int> forcedInstanceId = std::nullopt,
        std::string* reason = nullptr
    ) -> int;
    // Removes one facility and rebuilds occupancy if it existed.
    [[nodiscard]] bool removeFacility(int instanceId, const FacilityCatalog& catalog);
    // Clears all state and resizes the grid.
    void reset(int width, int height);

private:
    [[nodiscard]] auto indexForCell(int x, int y) const
        -> std::size_t;
    [[nodiscard]] auto isInsideBounds(const GridPoint& cell) const
        -> bool;
    [[nodiscard]] auto isPerimeterCell(const GridPoint& cell) const
        -> bool;
    void rebuildOccupancy(const FacilityCatalog& catalog);

    int width_ = 0;
    int height_ = 0;
    int nextInstanceId_ = 1;
    std::vector<FacilityInstance> facilities_;
    std::vector<CellOccupancy> cells_;
};
}  // namespace endfield_base
