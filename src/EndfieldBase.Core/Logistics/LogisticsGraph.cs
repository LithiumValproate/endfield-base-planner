using EndfieldBase.Core.Grid;
using EndfieldBase.Core.Models;

namespace EndfieldBase.Core.Logistics;

public enum LogisticsLayer
{
    Ground,
    Bridge,
}

public readonly record struct LayeredGridPoint(GridPoint Position, LogisticsLayer Layer);

public sealed class LogisticsCell
{
    public bool Traversable { get; set; }
    public double Capacity { get; set; }
    public int? OwnerInstanceId { get; set; }
}

public sealed class LogisticsGraph
{
    private static readonly GridPoint[] Directions =
        [
            new(1, 0),
            new(-1, 0),
            new(0, 1),
            new(0, -1),
        ];

    private readonly int _width;
    private readonly int _height;
    private readonly LogisticsCell[] _groundCells;
    private readonly LogisticsCell[] _bridgeCells;
    private readonly SimulationState _state;

    public LogisticsGraph(SimulationState state) {
        _state = state;
        _width = state.Grid.GetWidth();
        _height = state.Grid.GetHeight();
        _groundCells = Enumerable.Range(0, _width * _height).Select(_ => new LogisticsCell()).ToArray();
        _bridgeCells = Enumerable.Range(0, _width * _height).Select(_ => new LogisticsCell()).ToArray();

        foreach (var instance in state.Grid.GetFacilities()) {
            var definition = state.Catalog.FindDefinition(instance.DefinitionId)
                             ?? throw new InvalidOperationException(
                                 $"Unknown definition id while building logistics graph: {instance.DefinitionId}");
            AddInstanceCells(instance, definition, state.Grid);
        }

        foreach (var instance in state.Grid.GetFacilities()) {
            var definition = state.Catalog.FindDefinition(instance.DefinitionId);
            if (definition is not null) ApplySpecialFacilityCapacity(instance, definition);
        }
    }

    public LogisticsCell? FindCell(LayeredGridPoint point) {
        if (point.Position.X < 0 || point.Position.Y < 0 || point.Position.X >= _width ||
            point.Position.Y >= _height) return null;

        var index = IndexForCell(point);
        return point.Layer == LogisticsLayer.Ground ? _groundCells[index] : _bridgeCells[index];
    }

    public List<LayeredGridPoint> GetNeighbors(LayeredGridPoint point) {
        List<LayeredGridPoint> neighbors = new();
        var currentCell = FindCell(point);
        if (currentCell is null || !currentCell.Traversable) return neighbors;

        foreach (var direction in Directions) {
            LayeredGridPoint sameLayer = new(point.Position + direction, point.Layer);
            var sameLayerCell = FindCell(sameLayer);
            if (sameLayerCell is not null && sameLayerCell.Traversable && CanTraverseBetween(point, sameLayer))
                neighbors.Add(sameLayer);

            LayeredGridPoint otherLayer = new(
                point.Position + direction,
                point.Layer == LogisticsLayer.Ground ? LogisticsLayer.Bridge : LogisticsLayer.Ground);
            var otherLayerCell = FindCell(otherLayer);
            if (otherLayerCell is not null && otherLayerCell.Traversable && CanTraverseBetween(point, otherLayer))
                neighbors.Add(otherLayer);
        }

        return neighbors;
    }

    public List<LayeredGridPoint> GetEndpointsForInstance(int instanceId) {
        var instance = _state.Grid.FindFacility(instanceId);
        if (instance is null) return [];

        var definition = _state.Catalog.FindDefinition(instance.DefinitionId);
        if (definition is null) return [];

        List<LayeredGridPoint> endpoints = new();
        foreach (var cell in _state.Grid.GetOccupiedCells(definition, instance)) {
            LayeredGridPoint point = new(
                cell,
                FacilityRules.SupportsBridgeLayer(definition.Category) ? LogisticsLayer.Bridge : LogisticsLayer.Ground);
            var logisticsCell = FindCell(point);
            if (logisticsCell is not null && logisticsCell.Traversable) endpoints.Add(point);
        }

        return endpoints;
    }

    private int IndexForCell(LayeredGridPoint point) {
        return point.Position.Y * _width + point.Position.X;
    }

    private void SetCell(LayeredGridPoint point, LogisticsCell cell) {
        var index = IndexForCell(point);
        if (point.Layer == LogisticsLayer.Ground)
            _groundCells[index] = cell;
        else
            _bridgeCells[index] = cell;
    }

    private bool GroundAllowsDirection(LayeredGridPoint point, GridPoint direction) {
        var occupancy = _state.Grid.GetCell(point.Position.X, point.Position.Y);
        if (occupancy is null || !occupancy.BridgeInstanceId.HasValue) return true;

        var bridgeInstance = _state.Grid.FindFacility(occupancy.BridgeInstanceId.Value);
        if (bridgeInstance is null) return true;

        return IsHorizontalRotation(bridgeInstance.Rotation) ? direction.Y != 0 : direction.X != 0;
    }

    private bool BridgeAllowsDirection(LogisticsCell cell, GridPoint direction) {
        if (!cell.OwnerInstanceId.HasValue) return false;

        var instance = _state.Grid.FindFacility(cell.OwnerInstanceId.Value);
        if (instance is null) return false;

        var definition = _state.Catalog.FindDefinition(instance.DefinitionId);
        if (definition is null || definition.Category != FacilityCategory.Bridge) return false;

        return IsHorizontalRotation(instance.Rotation) ? direction.X != 0 : direction.Y != 0;
    }

    private bool CanTraverseBetween(LayeredGridPoint from, LayeredGridPoint to) {
        var fromCell = FindCell(from);
        var toCell = FindCell(to);
        if (fromCell is null || toCell is null || !fromCell.Traversable || !toCell.Traversable) return false;

        var direction = to.Position - from.Position;
        if (from.Layer == to.Layer)
            return from.Layer == LogisticsLayer.Ground
                ? GroundAllowsDirection(from, direction) && GroundAllowsDirection(to, direction)
                : BridgeAllowsDirection(fromCell, direction) && BridgeAllowsDirection(toCell, direction);

        return (fromCell.OwnerInstanceId.HasValue && BridgeAllowsDirection(fromCell, direction))
               || (toCell.OwnerInstanceId.HasValue && BridgeAllowsDirection(toCell, direction));
    }

    private int CountSameLayerConnections(FacilityInstance instance, FacilityDefinition definition) {
        var layer = FacilityRules.SupportsBridgeLayer(definition.Category)
            ? LogisticsLayer.Bridge
            : LogisticsLayer.Ground;
        var connectionCount = 0;
        foreach (var occupiedCell in _state.Grid.GetOccupiedCells(definition, instance))
            foreach (var direction in Directions) {
                LayeredGridPoint neighborPoint = new(occupiedCell + direction, layer);
                var neighborCell = FindCell(neighborPoint);
                if (neighborCell is null || !neighborCell.Traversable) continue;

                if (neighborCell.OwnerInstanceId == instance.InstanceId) continue;

                connectionCount++;
            }

        return connectionCount;
    }

    private int CountConnectedDirections(FacilityInstance instance, LogisticsLayer layer,
                                         IReadOnlyList<GridPoint> directions) {
        var connectionCount = 0;
        foreach (var direction in directions) {
            LayeredGridPoint neighborPoint = new(instance.Position + direction, layer);
            var neighborCell = FindCell(neighborPoint);
            if (neighborCell is null || !neighborCell.Traversable) continue;

            if (neighborCell.OwnerInstanceId == instance.InstanceId) continue;

            connectionCount++;
        }

        return connectionCount;
    }

    private void ApplySpecialFacilityCapacity(FacilityInstance instance, FacilityDefinition definition) {
        if (definition.Category != FacilityCategory.Splitter
            && definition.Category != FacilityCategory.Merger
            && !(FacilityRules.IsProductionFacility(definition) && definition.MaxOutputs > 1))
            return;

        var sharedBranchCount = 1;
        if (definition.Category == FacilityCategory.Splitter ||
            (FacilityRules.IsProductionFacility(definition) && definition.MaxOutputs > 1))
            sharedBranchCount = CountConnectedDirections(instance, LogisticsLayer.Ground,
                FacilityRules.GetOrderedOutputDirections(definition, instance.Rotation));
        else
            sharedBranchCount = Math.Clamp(CountSameLayerConnections(instance, definition) - 1, 1, 3);

        if (sharedBranchCount <= 1 || definition.BaseThroughput <= 0.0) return;

        var sharedCapacity = definition.BaseThroughput / sharedBranchCount;
        foreach (var occupiedCell in _state.Grid.GetOccupiedCells(definition, instance)) {
            LayeredGridPoint point = new(occupiedCell, LogisticsLayer.Ground);
            var currentCell = FindCell(point);
            if (currentCell is null) continue;

            LogisticsCell updatedCell = new()
                {
                    Traversable = currentCell.Traversable,
                    Capacity = Math.Min(currentCell.Capacity, sharedCapacity),
                    OwnerInstanceId = currentCell.OwnerInstanceId,
                };
            SetCell(point, updatedCell);
        }
    }

    private void AddInstanceCells(FacilityInstance instance, FacilityDefinition definition, GridMap grid) {
        if (!FacilityRules.IsLogisticsFacility(definition.Category) && !IsEndpointDefinition(definition)) return;

        var layer = FacilityRules.SupportsBridgeLayer(definition.Category)
            ? LogisticsLayer.Bridge
            : LogisticsLayer.Ground;
        foreach (var cellPosition in grid.GetOccupiedCells(definition, instance)) {
            LayeredGridPoint point = new(cellPosition, layer);
            var existingCell = FindCell(point);
            var newCell = existingCell is null
                ? new LogisticsCell()
                : new LogisticsCell
                    {
                        Traversable = existingCell.Traversable,
                        Capacity = existingCell.Capacity,
                        OwnerInstanceId = existingCell.OwnerInstanceId,
                    };

            newCell.Traversable = true;
            newCell.Capacity = Math.Max(newCell.Capacity, FacilityRules.DefaultTraversalCapacity(definition));
            if (!newCell.OwnerInstanceId.HasValue || definition.OccupiesBaseGrid)
                newCell.OwnerInstanceId = instance.InstanceId;

            SetCell(point, newCell);
        }
    }

    private static bool IsHorizontalRotation(Rotation rotation) {
        return rotation is Rotation.Deg0 or Rotation.Deg180;
    }

    private static bool IsEndpointDefinition(FacilityDefinition definition) {
        return FacilityRules.IsProductionFacility(definition)
               || FacilityRules.IsThermalMachine(definition)
               || FacilityRules.IsChestFacility(definition)
               || FacilityRules.IsHubFacility(definition)
               || FacilityRules.IsStorageFacility(definition.Category);
    }
}