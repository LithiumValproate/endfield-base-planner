#include "endfield_base/grid_map.h"

#include <algorithm>
#include <ranges>
#include <stdexcept>

namespace endfield_base {
GridMap::GridMap(int width, int height) {
    reset(width, height);
}

int GridMap::getWidth() const {
    return width_;
}

int GridMap::getHeight() const {
    return height_;
}

int GridMap::getNextInstanceId() const {
    return nextInstanceId_;
}

void GridMap::setNextInstanceId(int nextInstanceId) {
    nextInstanceId_ = nextInstanceId;
}

const std::vector<FacilityInstance>& GridMap::getFacilities() const {
    return facilities_;
}

FacilityInstance* GridMap::findFacility(int instanceId) {
    const auto it = std::ranges::find_if(
        facilities_,
        [instanceId] (const FacilityInstance& instance) { return instance.instanceId == instanceId; }
    );

    if (it == facilities_.end()) {
        return nullptr;
    }

    return &*it;
}

const FacilityInstance* GridMap::findFacility(int instanceId) const {
    const auto it = std::ranges::find_if(
        facilities_,
        [instanceId] (const FacilityInstance& instance) { return instance.instanceId == instanceId; }
    );

    if (it == facilities_.end()) {
        return nullptr;
    }

    return &*it;
}

const CellOccupancy* GridMap::getCell(int x, int y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        return nullptr;
    }

    return &cells_.at(indexForCell(x, y));
}

auto GridMap::getOccupiedCells(
    const FacilityDefinition& definition,
    const FacilityInstance& instance
) const -> std::vector<GridPoint> {
    const GridSize footprint = rotateFootprint(definition.footprint, instance.rotation);
    std::vector<GridPoint> occupiedCells;
    occupiedCells.reserve(static_cast<std::size_t>(footprint.width * footprint.height));
    for (int offsetY = 0; offsetY < footprint.height; ++offsetY) {
        for (int offsetX = 0; offsetX < footprint.width; ++offsetX) {
            occupiedCells.push_back({instance.position.x + offsetX, instance.position.y + offsetY});
        }
    }
    return occupiedCells;
}

std::vector<int> GridMap::getFacilityIdsAt(const GridPoint& cell) const {
    const CellOccupancy* occupancy = getCell(cell.x, cell.y);
    if (occupancy == nullptr) {
        return {};
    }

    std::vector<int> ids;
    ids.reserve(
        occupancy->attachedInstanceIds.size()
        + static_cast<std::size_t>(occupancy->baseInstanceId.has_value())
        + static_cast<std::size_t>(occupancy->bridgeInstanceId.has_value())
    );
    if (const auto baseInstanceId = occupancy->baseInstanceId) {
        ids.push_back(*baseInstanceId);
    }
    if (const auto bridgeInstanceId = occupancy->bridgeInstanceId) {
        ids.push_back(*bridgeInstanceId);
    }
    ids.insert(ids.end(), occupancy->attachedInstanceIds.begin(), occupancy->attachedInstanceIds.end());
    return ids;
}

auto GridMap::canPlaceFacility(
    const FacilityDefinition& definition,
    const GridPoint& origin,
    Rotation rotation,
    std::string* reason
) const -> bool {
    const GridSize footprint = rotateFootprint(definition.footprint, rotation);
    if (origin.x < 0 || origin.y < 0 || origin.x + footprint.width > width_ || origin.y + footprint.height >
        height_) {
        if (reason != nullptr) {
            *reason = "placement out of bounds";
        }
        return false;
    }

    for (int offsetY = 0; offsetY < footprint.height; ++offsetY) {
        for (int offsetX = 0; offsetX < footprint.width; ++offsetX) {
            const GridPoint currentCell{origin.x + offsetX, origin.y + offsetY};
            const CellOccupancy& occupancy = cells_.at(indexForCell(currentCell.x, currentCell.y));

            if (!definition.occupiesBaseGrid) {
                if (definition.canAttachToStorageLine && !isPerimeterCell(currentCell)) {
                    if (reason != nullptr) {
                        *reason = "storage port must attach to perimeter line";
                    }
                    return false;
                }

                if (definition.canMountOnConveyor && !occupancy.baseInstanceId.has_value()) {
                    if (reason != nullptr) {
                        *reason = "mounted facility requires base carrier";
                    }
                    return false;
                }

                if (!occupancy.attachedInstanceIds.empty()) {
                    if (reason != nullptr) {
                        *reason = "attachment layer already occupied";
                    }
                    return false;
                }

                continue;
            }

            if (supportsBridgeLayer(definition.category)) {
                if (occupancy.bridgeInstanceId.has_value()) {
                    if (reason != nullptr) {
                        *reason = "bridge layer already occupied";
                    }
                    return false;
                }
            } else {
                if (occupancy.baseInstanceId.has_value()) {
                    if (reason != nullptr) {
                        *reason = "base layer already occupied";
                    }
                    return false;
                }
            }
        }
    }

    return true;
}

auto GridMap::placeFacility(
    const FacilityDefinition& definition,
    std::string_view definitionId,
    const GridPoint& origin,
    Rotation rotation,
    std::optional<int> forcedInstanceId,
    std::string* reason
) -> int {
    if (!canPlaceFacility(definition, origin, rotation, reason)) {
        return 0;
    }

    const int instanceId = forcedInstanceId.value_or(nextInstanceId_);
    nextInstanceId_ = std::max(nextInstanceId_, instanceId + 1);
    facilities_.emplace_back(FacilityInstance{
        instanceId,
        std::string(definitionId),
        origin,
        rotation,
    });
    const GridSize footprint = rotateFootprint(definition.footprint, rotation);

    for (int offsetY = 0; offsetY < footprint.height; ++offsetY) {
        for (int offsetX = 0; offsetX < footprint.width; ++offsetX) {
            CellOccupancy& occupancy = cells_.at(indexForCell(origin.x + offsetX, origin.y + offsetY));
            if (!definition.occupiesBaseGrid) {
                occupancy.attachedInstanceIds.push_back(instanceId);
            } else if (supportsBridgeLayer(definition.category)) {
                occupancy.bridgeInstanceId = instanceId;
            } else {
                occupancy.baseInstanceId = instanceId;
            }
        }
    }

    return instanceId;
}

bool GridMap::removeFacility(int instanceId, const FacilityCatalog& catalog) {
    const auto it = std::ranges::find_if(
        facilities_,
        [instanceId] (const FacilityInstance& instance) { return instance.instanceId == instanceId; }
    );
    if (it == facilities_.end()) {
        return false;
    }

    const FacilityDefinition* definition = catalog.findDefinition(it->definitionId);
    if (definition == nullptr) {
        throw std::runtime_error("Unknown definition id while removing facility: " + it->definitionId);
    }

    for (const auto occupiedCells = getOccupiedCells(*definition, *it); const GridPoint& cell : occupiedCells) {
        CellOccupancy& occupancy = cells_.at(indexForCell(cell.x, cell.y));
        if (!definition->occupiesBaseGrid) {
            std::erase(
                occupancy.attachedInstanceIds,
                instanceId
            );
        } else if (supportsBridgeLayer(definition->category)) {
            occupancy.bridgeInstanceId.reset();
        } else {
            occupancy.baseInstanceId.reset();
        }
    }

    facilities_.erase(it);
    return true;
}

void GridMap::reset(int width, int height) {
    width_ = width;
    height_ = height;
    nextInstanceId_ = 1;
    facilities_.clear();
    cells_.assign(static_cast<std::size_t>(width_ * height_), {});
}

std::size_t GridMap::indexForCell(int x, int y) const {
    return static_cast<std::size_t>(y * width_ + x);
}

bool GridMap::isInsideBounds(const GridPoint& cell) const {
    return cell.x >= 0 && cell.y >= 0 && cell.x < width_ && cell.y < height_;
}

bool GridMap::isPerimeterCell(const GridPoint& cell) const {
    if (!isInsideBounds(cell)) {
        return false;
    }

    return cell.x == 0 || cell.y == 0 || cell.x == width_ - 1 || cell.y == height_ - 1;
}

void GridMap::rebuildOccupancy(const FacilityCatalog& catalog) {
    cells_.assign(static_cast<std::size_t>(width_ * height_), {});
    for (const FacilityInstance& instance : facilities_) {
        const FacilityDefinition* definition = catalog.findDefinition(instance.definitionId);
        if (definition == nullptr) {
            throw std::runtime_error("Unknown definition id in map: " + instance.definitionId);
        }

        for (const auto occupiedCells = getOccupiedCells(*definition, instance);
             const GridPoint& cell : occupiedCells) {
            CellOccupancy& occupancy = cells_.at(indexForCell(cell.x, cell.y));
            if (!definition->occupiesBaseGrid) {
                occupancy.attachedInstanceIds.push_back(instance.instanceId);
            } else if (supportsBridgeLayer(definition->category)) {
                occupancy.bridgeInstanceId = instance.instanceId;
            } else {
                occupancy.baseInstanceId = instance.instanceId;
            }
        }
    }
}
}  // namespace endfield_base
