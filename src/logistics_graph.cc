#include "endfield_base/logistics_graph.h"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace endfield_base {
namespace {
    constexpr std::array<GridPoint, 4> Directions{
        {
            {1, 0},
            {-1, 0},
            {0, 1},
            {0, -1},
        }
    };

    bool isHorizontalRotation(Rotation rotation) {
        return rotation == Rotation::Deg_0 || rotation == Rotation::Deg_180;
    }

    bool isEndpointDefinition(const FacilityDefinition& definition) {
        return isProductionFacility(definition)
               || isThermalMachine(definition)
               || isChestFacility(definition)
               || isHubFacility(definition)
               || isStorageFacility(definition.category);
    }
}  // namespace

LogisticsGraph::LogisticsGraph(const SimulationState& state) : width_(state.grid.getWidth()),
                                                               height_(state.grid.getHeight()),
                                                               groundCells_(static_cast<std::size_t>(width_ * height_)),
                                                               bridgeCells_(static_cast<std::size_t>(width_ * height_)),
                                                               state_(state) {
    for (const FacilityInstance& instance : state.grid.getFacilities()) {
        const FacilityDefinition* definition = state.catalog.findDefinition(instance.definitionId);
        if (definition == nullptr) {
            throw std::runtime_error(
                "Unknown definition id while building logistics graph: " + instance.definitionId);
        }
        addInstanceCells(instance, *definition, state.grid);
    }

    for (const FacilityInstance& instance : state.grid.getFacilities()) {
        const FacilityDefinition* definition = state.catalog.findDefinition(instance.definitionId);
        if (definition == nullptr) {
            continue;
        }
        applySpecialFacilityCapacity(instance, *definition);
    }
}

const LogisticsCell* LogisticsGraph::findCell(const LayeredGridPoint& point) const {
    if (point.position.x < 0 || point.position.y < 0 || point.position.x >= width_ || point.position.y >= height_) {
        return nullptr;
    }

    const std::size_t index = indexForCell(point);
    return point.layer == LogisticsLayer::Ground ? &groundCells_.at(index) : &bridgeCells_.at(index);
}

std::vector<LayeredGridPoint> LogisticsGraph::getNeighbors(const LayeredGridPoint& point) const {
    std::vector<LayeredGridPoint> neighbors;
    if (const LogisticsCell* currentCell = findCell(point); currentCell == nullptr || !currentCell->traversable) {
        return neighbors;
    }

    for (const GridPoint direction : Directions) {
        const LayeredGridPoint sameLayer{
            {point.position.x + direction.x, point.position.y + direction.y},
            point.layer,
        };
        if (const LogisticsCell* sameLayerCell = findCell(sameLayer);
            sameLayerCell != nullptr && sameLayerCell->traversable && canTraverseBetween(point, sameLayer)) {
            neighbors.push_back(sameLayer);
        }

        const LayeredGridPoint otherLayer{
            {point.position.x + direction.x, point.position.y + direction.y},
            point.layer == LogisticsLayer::Ground ? LogisticsLayer::Bridge : LogisticsLayer::Ground,
        };
        if (const LogisticsCell* otherLayerCell = findCell(otherLayer);
            otherLayerCell != nullptr && otherLayerCell->traversable && canTraverseBetween(point, otherLayer)) {
            neighbors.push_back(otherLayer);
        }
    }

    return neighbors;
}

std::vector<LayeredGridPoint> LogisticsGraph::getEndpointsForInstance(int instanceId) const {
    const FacilityInstance* instance = state_.grid.findFacility(instanceId);
    if (instance == nullptr) {
        return {};
    }

    const FacilityDefinition* definition = state_.catalog.findDefinition(instance->definitionId);
    if (definition == nullptr) {
        return {};
    }

    const auto occupiedCells = state_.grid.getOccupiedCells(*definition, *instance);
    std::vector<LayeredGridPoint> endpoints;
    endpoints.reserve(occupiedCells.size());

    for (const GridPoint& cell : occupiedCells) {
        const LayeredGridPoint point{
            cell,
            supportsBridgeLayer(definition->category) ? LogisticsLayer::Bridge : LogisticsLayer::Ground,
        };
        if (const LogisticsCell* logisticsCell = findCell(point);
            logisticsCell != nullptr && logisticsCell->traversable) {
            endpoints.push_back(point);
        }
    }

    return endpoints;
}

std::size_t LogisticsGraph::indexForCell(const LayeredGridPoint& point) const {
    return static_cast<std::size_t>(point.position.y * width_ + point.position.x);
}

void LogisticsGraph::setCell(const LayeredGridPoint& point, const LogisticsCell& cell) {
    const std::size_t index = indexForCell(point);
    if (point.layer == LogisticsLayer::Ground) {
        groundCells_.at(index) = cell;
    } else {
        bridgeCells_.at(index) = cell;
    }
}

bool LogisticsGraph::groundAllowsDirection(const LayeredGridPoint& point, const GridPoint& direction) const {
    const CellOccupancy* occupancy = state_.grid.getCell(point.position.x, point.position.y);
    if (occupancy == nullptr || !occupancy->bridgeInstanceId.has_value()) {
        return true;
    }

    const FacilityInstance* bridgeInstance = state_.grid.findFacility(*occupancy->bridgeInstanceId);
    if (bridgeInstance == nullptr) {
        return true;
    }

    if (isHorizontalRotation(bridgeInstance->rotation)) {
        return direction.y != 0;
    }

    return direction.x != 0;
}

bool LogisticsGraph::bridgeAllowsDirection(const LogisticsCell& cell, const GridPoint& direction) const {
    if (!cell.ownerInstanceId.has_value()) {
        return false;
    }

    const FacilityInstance* instance = state_.grid.findFacility(*cell.ownerInstanceId);
    if (instance == nullptr) {
        return false;
    }

    if (const FacilityDefinition* definition = state_.catalog.findDefinition(instance->definitionId);
        definition == nullptr || definition->category != FacilityCategory::Bridge) {
        return false;
    }

    if (isHorizontalRotation(instance->rotation)) {
        return direction.x != 0;
    }

    return direction.y != 0;
}

bool LogisticsGraph::canTraverseBetween(const LayeredGridPoint& from, const LayeredGridPoint& to) const {
    const LogisticsCell* fromCell = findCell(from);
    const LogisticsCell* toCell = findCell(to);
    if (fromCell == nullptr || toCell == nullptr || !fromCell->traversable || !toCell->traversable) {
        return false;
    }

    const GridPoint direction{
        to.position.x - from.position.x,
        to.position.y - from.position.y,
    };

    if (from.layer == to.layer) {
        if (from.layer == LogisticsLayer::Ground) {
            return groundAllowsDirection(from, direction) && groundAllowsDirection(to, direction);
        }

        return bridgeAllowsDirection(*fromCell, direction) && bridgeAllowsDirection(*toCell, direction);
    }

    if (fromCell->ownerInstanceId.has_value() && bridgeAllowsDirection(*fromCell, direction)) {
        return true;
    }

    if (toCell->ownerInstanceId.has_value() && bridgeAllowsDirection(*toCell, direction)) {
        return true;
    }

    return false;
}

auto LogisticsGraph::countSameLayerConnections(
    const FacilityInstance& instance,
    const FacilityDefinition& definition
) const -> int {
    const LogisticsLayer layer = supportsBridgeLayer(definition.category)
                                     ? LogisticsLayer::Bridge
                                     : LogisticsLayer::Ground;
    const auto occupiedCells = state_.grid.getOccupiedCells(definition, instance);
    int connectionCount = 0;

    for (const GridPoint& occupiedCell : occupiedCells) {
        for (const GridPoint direction : Directions) {
            const LayeredGridPoint neighborPoint{
                {occupiedCell.x + direction.x, occupiedCell.y + direction.y},
                layer,
            };
            const LogisticsCell* neighborCell = findCell(neighborPoint);
            if (neighborCell == nullptr || !neighborCell->traversable) {
                continue;
            }
            if (neighborCell->ownerInstanceId.has_value()
                && *neighborCell->ownerInstanceId == instance.instanceId) {
                continue;
            }
            ++connectionCount;
        }
    }

    return connectionCount;
}

auto LogisticsGraph::countConnectedDirections(
    const FacilityInstance& instance,
    LogisticsLayer layer,
    const std::vector<GridPoint>& directions
) const -> int {
    int connectionCount = 0;
    for (const GridPoint& direction : directions) {
        const LayeredGridPoint neighborPoint{
            {instance.position.x + direction.x, instance.position.y + direction.y},
            layer,
        };
        const LogisticsCell* neighborCell = findCell(neighborPoint);
        if (neighborCell == nullptr || !neighborCell->traversable) {
            continue;
        }
        if (neighborCell->ownerInstanceId.has_value()
            && *neighborCell->ownerInstanceId == instance.instanceId) {
            continue;
        }
        ++connectionCount;
    }

    return connectionCount;
}

auto LogisticsGraph::applySpecialFacilityCapacity(
    const FacilityInstance& instance,
    const FacilityDefinition& definition
) -> void {
    if (definition.category != FacilityCategory::Splitter
        && definition.category != FacilityCategory::Merger
        && !(isProductionFacility(definition) && definition.maxOutputs > 1)) {
        return;
    }

    int sharedBranchCount = 1;
    if (definition.category == FacilityCategory::Splitter) {
        sharedBranchCount = countConnectedDirections(
            instance,
            LogisticsLayer::Ground,
            getOrderedOutputDirections(definition, instance.rotation)
        );
    } else if (isProductionFacility(definition) && definition.maxOutputs > 1) {
        sharedBranchCount = countConnectedDirections(
            instance,
            LogisticsLayer::Ground,
            getOrderedOutputDirections(definition, instance.rotation)
        );
    } else {
        const int connectionCount = countSameLayerConnections(instance, definition);
        sharedBranchCount = std::clamp(connectionCount - 1, 1, 3);
    }

    if (sharedBranchCount <= 1 || definition.baseThroughput <= 0.0) {
        return;
    }

    const double sharedCapacity = definition.baseThroughput / static_cast<double>(sharedBranchCount);

    for (const auto occupiedCells = state_.grid.getOccupiedCells(definition, instance); const GridPoint& occupiedCell :
         occupiedCells) {
        const LayeredGridPoint point{occupiedCell, LogisticsLayer::Ground};
        const LogisticsCell* currentCell = findCell(point);
        if (currentCell == nullptr) {
            continue;
        }

        LogisticsCell updatedCell = *currentCell;
        updatedCell.capacity = std::min(updatedCell.capacity, sharedCapacity);
        setCell(point, updatedCell);
    }
}

auto LogisticsGraph::addInstanceCells(
    const FacilityInstance& instance,
    const FacilityDefinition& definition,
    const GridMap& grid
) -> void {
    if (!isLogisticsFacility(definition.category) && !isEndpointDefinition(definition)) {
        return;
    }

    const LogisticsLayer layer = supportsBridgeLayer(definition.category)
                                     ? LogisticsLayer::Bridge
                                     : LogisticsLayer::Ground;
    for (const auto occupiedCells = grid.getOccupiedCells(definition, instance); const GridPoint& cellPosition :
         occupiedCells) {
        const LayeredGridPoint point{cellPosition, layer};
        const LogisticsCell* existingCell = findCell(point);
        LogisticsCell newCell;
        if (existingCell != nullptr) {
            newCell = *existingCell;
        }

        newCell.traversable = true;
        newCell.capacity = std::max(newCell.capacity, defaultTraversalCapacity(definition));
        if (!newCell.ownerInstanceId.has_value() || definition.occupiesBaseGrid) {
            newCell.ownerInstanceId = instance.instanceId;
        }
        setCell(point, newCell);
    }
}
}  // namespace endfield_base
