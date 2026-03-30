#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "endfield_base/facility.h"

namespace endfield_base {
struct CellOccupancy {
    std::optional<int> baseInstanceId;
    std::optional<int> bridgeInstanceId;
    std::vector<int> attachedInstanceIds;
};

class GridMap {
public:
    GridMap() = default;
    GridMap(int width, int height);

    [[nodiscard]] auto getWidth() const
        -> int;
    [[nodiscard]] auto getHeight() const
        -> int;
    [[nodiscard]] auto getNextInstanceId() const
        -> int;
    void setNextInstanceId(int nextInstanceId);

    [[nodiscard]] auto getFacilities() const
        -> const std::vector<FacilityInstance>&;
    [[nodiscard]] auto findFacility(int instanceId)
        -> FacilityInstance*;
    [[nodiscard]] auto findFacility(int instanceId) const
        -> const FacilityInstance*;
    [[nodiscard]] auto getCell(int x, int y) const
        -> const CellOccupancy*;
    [[nodiscard]] auto getOccupiedCells(
        const FacilityDefinition& definition,
        const FacilityInstance& instance
    ) const -> std::vector<GridPoint>;
    [[nodiscard]] auto getFacilityIdsAt(const GridPoint& cell) const
        -> std::vector<int>;
    [[nodiscard]] auto canPlaceFacility(
        const FacilityDefinition& definition,
        const GridPoint& origin,
        Rotation rotation,
        std::string* reason = nullptr
    ) const -> bool;
    [[nodiscard]] auto placeFacility(
        const FacilityDefinition& definition,
        std::string_view definitionId,
        const GridPoint& origin,
        Rotation rotation,
        std::optional<int> forcedInstanceId = std::nullopt,
        std::string* reason = nullptr
    ) -> int;
    [[nodiscard]] bool removeFacility(int instanceId, const FacilityCatalog& catalog);
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
