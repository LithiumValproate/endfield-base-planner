#pragma once

#include <optional>
#include <vector>

#include "endfield_base/facility.h"
#include "endfield_base/simulation_state.h"

namespace endfield_base {
// Identifies which traversal layer a logistics cell belongs to.
enum class LogisticsLayer {
    Ground,
    Bridge,
};

// Identifies one traversable point in the layered logistics graph.
struct LayeredGridPoint {
    GridPoint position;
    LogisticsLayer layer = LogisticsLayer::Ground;

    bool operator==(const LayeredGridPoint& other) const = default;
};

// Stores traversal data for one cell in one logistics layer.
struct LogisticsCell {
    bool traversable = false;
    double capacity = 0.0;
    std::optional<int> ownerInstanceId;
};

// Materializes a layered traversal graph derived from the current map state.
class LogisticsGraph {
public:
    // Builds traversal cells and endpoints from the current simulation state.
    explicit LogisticsGraph(const SimulationState& state);

    // Looks up one cell in the layered graph.
    [[nodiscard]] auto findCell(const LayeredGridPoint& point) const
        -> const LogisticsCell*;
    // Returns all graph neighbors reachable from the given point.
    [[nodiscard]] auto getNeighbors(const LayeredGridPoint& point) const
        -> std::vector<LayeredGridPoint>;
    // Returns all graph endpoints associated with one facility instance.
    [[nodiscard]] auto getEndpointsForInstance(int instanceId) const
        -> std::vector<LayeredGridPoint>;

private:
    [[nodiscard]] auto indexForCell(const LayeredGridPoint& point) const
        -> std::size_t;
    [[nodiscard]] auto groundAllowsDirection(
        const LayeredGridPoint& point,
        const GridPoint& direction
    ) const -> bool;
    [[nodiscard]] auto bridgeAllowsDirection(
        const LogisticsCell& cell,
        const GridPoint& direction
    ) const -> bool;
    [[nodiscard]] auto canTraverseBetween(
        const LayeredGridPoint& from,
        const LayeredGridPoint& to
    ) const -> bool;
    void setCell(const LayeredGridPoint& point, const LogisticsCell& cell);
    [[nodiscard]] auto countSameLayerConnections(
        const FacilityInstance& instance,
        const FacilityDefinition& definition
    ) const -> int;
    [[nodiscard]] auto countConnectedDirections(
        const FacilityInstance& instance,
        LogisticsLayer layer,
        const std::vector<GridPoint>& directions
    ) const -> int;
    void applySpecialFacilityCapacity(const FacilityInstance& instance, const FacilityDefinition& definition);
    void addInstanceCells(
        const FacilityInstance& instance,
        const FacilityDefinition& definition,
        const GridMap& grid
    );

    int width_ = 0;
    int height_ = 0;
    std::vector<LogisticsCell> groundCells_;
    std::vector<LogisticsCell> bridgeCells_;
    const SimulationState& state_;
};
}  // namespace endfield_base
