#include "endfield_base/path_finder.h"

#include <algorithm>
#include <deque>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <unordered_map>

#include "endfield_base/logistics_graph.h"

namespace endfield_base {
namespace {
    struct LayeredPointHash {
        std::size_t operator()(const LayeredGridPoint& point) const noexcept {
            const std::size_t layer = point.layer == LogisticsLayer::Ground ? 0U : 1U;
            return static_cast<std::size_t>((point.position.x * 73856093) ^ (point.position.y * 19349663) ^ (
                                                layer * 83492791));
        }
    };
} // namespace

PathResult PathFinder::findPath(const SimulationState& state, const PathRequest& request) {
    LogisticsGraph graph(state);
    const auto startPoints = graph.getEndpointsForInstance(request.startInstanceId);
    const auto endPoints = graph.getEndpointsForInstance(request.endInstanceId);
    if (startPoints.empty() || endPoints.empty()) {
        return {};
    }

    std::unordered_map<LayeredGridPoint, LayeredGridPoint, LayeredPointHash> parents;
    std::unordered_map<LayeredGridPoint, int, LayeredPointHash> distanceByPoint;
    std::deque<LayeredGridPoint> frontier;
    for (const LayeredGridPoint& startPoint : startPoints) {
        frontier.push_back(startPoint);
        distanceByPoint[startPoint] = 0;
    }

    const auto isTarget = [&endPoints] (const LayeredGridPoint& candidate) {
        return std::ranges::find(endPoints, candidate) != endPoints.end();
    };

    std::optional<LayeredGridPoint> foundPoint;
    while (!frontier.empty() && !foundPoint.has_value()) {
        const LayeredGridPoint current = frontier.front();
        frontier.pop_front();

        if (isTarget(current)) {
            foundPoint = current;
            break;
        }

        for (const LayeredGridPoint& neighbor : graph.getNeighbors(current)) {
            if (distanceByPoint.contains(neighbor)) {
                continue;
            }

            parents.emplace(neighbor, current);
            distanceByPoint.emplace(neighbor, distanceByPoint.at(current) + 1);
            frontier.push_back(neighbor);
        }
    }

    if (!foundPoint.has_value()) {
        return {};
    }

    PathResult result;
    result.found = true;
    result.length = distanceByPoint.at(*foundPoint);
    result.bottleneckThroughput = std::numeric_limits<double>::max();

    LayeredGridPoint cursor = *foundPoint;
    while (true) {
        const LogisticsCell* cell = graph.findCell(cursor);
        if (cell == nullptr) {
            throw std::runtime_error("Path reconstruction failed");
        }

        result.steps.push_back({
                cursor.position,
                cursor.layer == LogisticsLayer::Bridge,
                cell->ownerInstanceId,
            });
        result.bottleneckThroughput = std::min(result.bottleneckThroughput, cell->capacity);

        if (!parents.contains(cursor)) {
            break;
        }

        cursor = parents.at(cursor);
    }

    std::reverse(result.steps.begin(), result.steps.end());
    if (result.bottleneckThroughput == std::numeric_limits<double>::max()) {
        result.bottleneckThroughput = 0.0;
    }

    for (const PathStep& step : result.steps) {
        if (!step.ownerInstanceId.has_value()) {
            continue;
        }
        if (std::ranges::find(result.traversedInstanceIds, *step.ownerInstanceId) == result.traversedInstanceIds.end()) {
            result.traversedInstanceIds.push_back(*step.ownerInstanceId);
        }
    }

    return result;
}
} // namespace endfield_base
