using EndfieldBase.Core.Models;

namespace EndfieldBase.Core.Logistics;

public sealed class PathRequest
{
    public int StartInstanceId { get; set; }
    public int EndInstanceId { get; set; }
}

public sealed class PathStep
{
    public GridPoint Position { get; set; }
    public bool BridgeLayer { get; set; }
    public int? OwnerInstanceId { get; set; }
}

public sealed class PathResult
{
    public bool Found { get; set; }
    public int Length { get; set; }
    public double BottleneckThroughput { get; set; }
    public List<PathStep> Steps { get; } = [];
    public List<int> TraversedInstanceIds { get; } = [];
}

public static class PathFinder
{
    public static PathResult FindPath(SimulationState state, PathRequest request) {
        LogisticsGraph graph = new(state);
        List<LayeredGridPoint> startPoints = graph.GetEndpointsForInstance(request.StartInstanceId);
        List<LayeredGridPoint> endPoints = graph.GetEndpointsForInstance(request.EndInstanceId);
        if (startPoints.Count == 0 || endPoints.Count == 0) return new PathResult();

        Dictionary<LayeredGridPoint, LayeredGridPoint> parents = new();
        Dictionary<LayeredGridPoint, int> distanceByPoint = new();
        Queue<LayeredGridPoint> frontier = new();
        foreach (var startPoint in startPoints) {
            frontier.Enqueue(startPoint);
            distanceByPoint[startPoint] = 0;
        }

        LayeredGridPoint? foundPoint = null;
        while (frontier.Count > 0 && foundPoint is null) {
            var current = frontier.Dequeue();
            if (endPoints.Contains(current)) {
                foundPoint = current;
                break;
            }

            foreach (var neighbor in graph.GetNeighbors(current)) {
                if (distanceByPoint.ContainsKey(neighbor)) continue;

                parents[neighbor] = current;
                distanceByPoint[neighbor] = distanceByPoint[current] + 1;
                frontier.Enqueue(neighbor);
            }
        }

        if (foundPoint is null) return new PathResult();

        PathResult result = new()
            {
                Found = true,
                Length = distanceByPoint[foundPoint.Value],
                BottleneckThroughput = double.MaxValue
            };

        var cursor = foundPoint.Value;
        while (true) {
            var cell = graph.FindCell(cursor) ??
                       throw new InvalidOperationException("Path reconstruction failed.");
            result.Steps.Add(new PathStep
                {
                    Position = cursor.Position,
                    BridgeLayer = cursor.Layer == LogisticsLayer.Bridge,
                    OwnerInstanceId = cell.OwnerInstanceId
                });
            result.BottleneckThroughput = Math.Min(result.BottleneckThroughput, cell.Capacity);

            if (!parents.TryGetValue(cursor, out var parent)) break;

            cursor = parent;
        }

        result.Steps.Reverse();
        if (result.BottleneckThroughput == double.MaxValue) result.BottleneckThroughput = 0.0;

        foreach (var step in result.Steps)
            if (step.OwnerInstanceId is int ownerInstanceId && !result.TraversedInstanceIds.Contains(ownerInstanceId))
                result.TraversedInstanceIds.Add(ownerInstanceId);

        return result;
    }
}