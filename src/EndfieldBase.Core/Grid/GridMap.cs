using EndfieldBase.Core.Models;

namespace EndfieldBase.Core.Grid;

public sealed class CellOccupancy
{
    public int? BaseInstanceId { get; set; }
    public int? BridgeInstanceId { get; set; }
    public List<int> AttachedInstanceIds { get; } = [];
}

public sealed class GridMap
{
    private int _width;
    private int _height;
    private int _nextInstanceId = 1;
    private readonly List<FacilityInstance> _facilities = [];
    private List<CellOccupancy> _cells = [];

    public GridMap() {}

    public GridMap(int width, int height) {
        Reset(width, height);
    }

    public int GetWidth() {
        return _width;
    }

    public int GetHeight() {
        return _height;
    }

    public int GetNextInstanceId() {
        return _nextInstanceId;
    }

    public void SetNextInstanceId(int nextInstanceId) {
        _nextInstanceId = nextInstanceId;
    }

    public IReadOnlyList<FacilityInstance> GetFacilities() {
        return _facilities;
    }

    public FacilityInstance? FindFacility(int instanceId) {
        return _facilities.FirstOrDefault(instance => instance.InstanceId == instanceId);
    }

    public CellOccupancy? GetCell(int x, int y) {
        return x < 0 || y < 0 || x >= _width || y >= _height ? null : _cells[IndexForCell(x, y)];
    }

    public List<GridPoint> GetOccupiedCells(FacilityDefinition definition, FacilityInstance instance) {
        var footprint = FacilityRules.RotateFootprint(definition.Footprint, instance.Rotation);
        List<GridPoint> occupiedCells = new(footprint.Width * footprint.Height);
        for (var offsetY = 0; offsetY < footprint.Height; offsetY++)
            for (var offsetX = 0; offsetX < footprint.Width; offsetX++)
                occupiedCells.Add(new GridPoint(instance.Position.X + offsetX, instance.Position.Y + offsetY));

        return occupiedCells;
    }

    public List<int> GetFacilityIdsAt(GridPoint cell) {
        var occupancy = GetCell(cell.X, cell.Y);
        if (occupancy is null) return [];

        List<int> ids = new(occupancy.AttachedInstanceIds.Count + (occupancy.BaseInstanceId.HasValue ? 1 : 0) +
                            (occupancy.BridgeInstanceId.HasValue ? 1 : 0));
        if (occupancy.BaseInstanceId.HasValue) ids.Add(occupancy.BaseInstanceId.Value);

        if (occupancy.BridgeInstanceId.HasValue) ids.Add(occupancy.BridgeInstanceId.Value);

        ids.AddRange(occupancy.AttachedInstanceIds);
        return ids;
    }

    public bool CanPlaceFacility(
        FacilityDefinition definition,
        GridPoint origin,
        Rotation rotation,
        out string? reason) {
        var footprint = FacilityRules.RotateFootprint(definition.Footprint, rotation);
        if (origin.X < 0 || origin.Y < 0 || origin.X + footprint.Width > _width ||
            origin.Y + footprint.Height > _height) {
            reason = "placement out of bounds";
            return false;
        }

        for (var offsetY = 0; offsetY < footprint.Height; offsetY++)
            for (var offsetX = 0; offsetX < footprint.Width; offsetX++) {
                GridPoint currentCell = new(origin.X + offsetX, origin.Y + offsetY);
                var occupancy = _cells[IndexForCell(currentCell.X, currentCell.Y)];

                if (!definition.OccupiesBaseGrid) {
                    if (definition.CanAttachToStorageLine && !IsPerimeterCell(currentCell)) {
                        reason = "storage port must attach to perimeter line";
                        return false;
                    }

                    if (definition.CanMountOnConveyor && !occupancy.BaseInstanceId.HasValue) {
                        reason = "mounted facility requires base carrier";
                        return false;
                    }

                    if (occupancy.AttachedInstanceIds.Count > 0) {
                        reason = "attachment layer already occupied";
                        return false;
                    }

                    continue;
                }

                if (FacilityRules.SupportsBridgeLayer(definition.Category)) {
                    if (occupancy.BridgeInstanceId.HasValue) {
                        reason = "bridge layer already occupied";
                        return false;
                    }
                }
                else if (occupancy.BaseInstanceId.HasValue) {
                    reason = "base layer already occupied";
                    return false;
                }
            }

        reason = null;
        return true;
    }

    public int PlaceFacility(
        FacilityDefinition definition,
        string definitionId,
        GridPoint origin,
        Rotation rotation,
        out string? reason,
        int? forcedInstanceId = null) {
        if (!CanPlaceFacility(definition, origin, rotation, out reason)) return 0;

        var instanceId = forcedInstanceId ?? _nextInstanceId;
        _nextInstanceId = Math.Max(_nextInstanceId, instanceId + 1);
        FacilityInstance instance = new()
            {
                InstanceId = instanceId,
                DefinitionId = definitionId,
                Position = origin,
                Rotation = rotation
            };
        _facilities.Add(instance);

        var footprint = FacilityRules.RotateFootprint(definition.Footprint, rotation);
        for (var offsetY = 0; offsetY < footprint.Height; offsetY++)
            for (var offsetX = 0; offsetX < footprint.Width; offsetX++) {
                var occupancy = _cells[IndexForCell(origin.X + offsetX, origin.Y + offsetY)];
                if (!definition.OccupiesBaseGrid)
                    occupancy.AttachedInstanceIds.Add(instanceId);
                else if (FacilityRules.SupportsBridgeLayer(definition.Category))
                    occupancy.BridgeInstanceId = instanceId;
                else
                    occupancy.BaseInstanceId = instanceId;
            }

        return instanceId;
    }

    public bool RemoveFacility(int instanceId, FacilityCatalog catalog) {
        var instance = FindFacility(instanceId);
        if (instance is null) return false;

        var definition = catalog.FindDefinition(instance.DefinitionId)
                         ?? throw new InvalidOperationException(
                             $"Unknown definition id while removing facility: {instance.DefinitionId}");

        foreach (var cell in GetOccupiedCells(definition, instance)) {
            var occupancy = _cells[IndexForCell(cell.X, cell.Y)];
            if (!definition.OccupiesBaseGrid)
                occupancy.AttachedInstanceIds.Remove(instanceId);
            else if (FacilityRules.SupportsBridgeLayer(definition.Category))
                occupancy.BridgeInstanceId = null;
            else
                occupancy.BaseInstanceId = null;
        }

        _facilities.Remove(instance);
        return true;
    }

    public void Reset(int width, int height) {
        _width = width;
        _height = height;
        _nextInstanceId = 1;
        _facilities.Clear();
        _cells = Enumerable.Range(0, width * height).Select(_ => new CellOccupancy()).ToList();
    }

    public void RebuildOccupancy(FacilityCatalog catalog) {
        _cells = Enumerable.Range(0, _width * _height).Select(_ => new CellOccupancy()).ToList();
        foreach (var instance in _facilities) {
            var definition = catalog.FindDefinition(instance.DefinitionId)
                             ?? throw new InvalidOperationException(
                                 $"Unknown definition id in map: {instance.DefinitionId}");

            foreach (var cell in GetOccupiedCells(definition, instance)) {
                var occupancy = _cells[IndexForCell(cell.X, cell.Y)];
                if (!definition.OccupiesBaseGrid)
                    occupancy.AttachedInstanceIds.Add(instance.InstanceId);
                else if (FacilityRules.SupportsBridgeLayer(definition.Category))
                    occupancy.BridgeInstanceId = instance.InstanceId;
                else
                    occupancy.BaseInstanceId = instance.InstanceId;
            }
        }
    }

    private int IndexForCell(int x, int y) {
        return y * _width + x;
    }

    private bool IsInsideBounds(GridPoint cell) {
        return cell.X >= 0 && cell.Y >= 0 && cell.X < _width && cell.Y < _height;
    }

    private bool IsPerimeterCell(GridPoint cell) {
        return IsInsideBounds(cell)
               && (cell.X == 0 || cell.Y == 0 || cell.X == _width - 1 || cell.Y == _height - 1);
    }
}