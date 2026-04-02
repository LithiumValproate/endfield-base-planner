namespace EndfieldBase.Core.Models;

public static class FacilityRules
{
    private static readonly GridPoint[] SplitterOutputOrder =
        [
            new(0, 1),
            new(1, 0),
            new(0, -1)
        ];

    private static readonly GridPoint[] SplitterInputOrder =
        [
            new(-1, 0)
        ];

    public static GridSize RotateFootprint(GridSize footprint, Rotation rotation) {
        return rotation is Rotation.Deg90 or Rotation.Deg270
            ? new GridSize(footprint.Height, footprint.Width)
            : footprint;
    }

    public static GridPoint RotateDirection(GridPoint direction, Rotation rotation) {
        return rotation switch
            {
                Rotation.Deg0 => direction,
                Rotation.Deg90 => new GridPoint(-direction.Y, direction.X),
                Rotation.Deg180 => new GridPoint(-direction.X, -direction.Y),
                Rotation.Deg270 => new GridPoint(direction.Y, -direction.X),
                _ => direction
            };
    }

    public static bool IsLogisticsFacility(FacilityCategory category) {
        return category is FacilityCategory.Conveyor or FacilityCategory.Splitter or FacilityCategory.Merger
            or FacilityCategory.Bridge;
    }

    public static bool IgnoresPowerCoverage(FacilityCategory category) {
        return IsLogisticsFacility(category)
               || category is FacilityCategory.Generator
                   or FacilityCategory.PowerPole
                   or FacilityCategory.PowerRelay
                   or FacilityCategory.Hub
                   or FacilityCategory.StorageIn
                   or FacilityCategory.StorageOut;
    }

    public static bool IsPowerTransmitter(FacilityCategory category) {
        return category is FacilityCategory.PowerPole or FacilityCategory.PowerRelay;
    }

    public static bool SupportsBridgeLayer(FacilityCategory category) {
        return category == FacilityCategory.Bridge;
    }

    public static bool IsStorageFacility(FacilityCategory category) {
        return category is FacilityCategory.StorageIn or FacilityCategory.StorageOut or FacilityCategory.StoragePort;
    }

    public static bool IsProductionFacility(FacilityDefinition definition) {
        return definition.Role == FacilityRole.ProductionFacility || definition.Category == FacilityCategory.Machine;
    }

    public static bool IsThermalMachine(FacilityDefinition definition) {
        return definition.Role == FacilityRole.ThermalMachine || definition.Category == FacilityCategory.Generator;
    }

    public static bool IsPowerFacility(FacilityDefinition definition) {
        return definition.Role == FacilityRole.PowerFacility || IsPowerTransmitter(definition.Category);
    }

    public static bool IsHubFacility(FacilityDefinition definition) {
        return definition.Role == FacilityRole.Hub || definition.Category == FacilityCategory.Hub;
    }

    public static bool IsChestFacility(FacilityDefinition definition) {
        return definition.Role == FacilityRole.Chest || definition.Category == FacilityCategory.Chest;
    }

    public static bool RequiresExternalPower(FacilityDefinition definition) {
        return !IgnoresPowerCoverage(definition.Category) && definition.RequiresPower;
    }

    public static bool UsesWirelessStorage(FacilityDefinition definition, FacilityInstance instance) {
        return IsChestFacility(definition)
               && definition.SupportsWirelessStorage
               && instance.StorageMode == StorageMode.Wireless;
    }

    public static bool HasAnyStoredItems(FacilityInstance instance) {
        return instance.InventorySlots.Any(slot => slot.Count > 0);
    }

    public static FacilityRole DefaultFacilityRole(FacilityCategory category) {
        return category switch
            {
                FacilityCategory.Machine => FacilityRole.ProductionFacility,
                FacilityCategory.PowerPole or FacilityCategory.PowerRelay => FacilityRole.PowerFacility,
                FacilityCategory.Generator => FacilityRole.ThermalMachine,
                FacilityCategory.Chest => FacilityRole.Chest,
                FacilityCategory.Hub => FacilityRole.Hub,
                FacilityCategory.StoragePort or FacilityCategory.StorageIn or FacilityCategory.StorageOut =>
                    FacilityRole
                        .StoragePort,
                FacilityCategory.Servo => FacilityRole.Servo,
                _ => FacilityRole.Generic
            };
    }

    public static bool DefaultRequiresPower(FacilityDefinition definition) {
        return definition.Category == FacilityCategory.Chest
               || definition.Role == FacilityRole.ProductionFacility
               || definition.Role == FacilityRole.Chest;
    }

    public static PowerFacilityType DefaultPowerFacilityType(FacilityCategory category) {
        return category switch
            {
                FacilityCategory.PowerPole => PowerFacilityType.Pole,
                FacilityCategory.PowerRelay => PowerFacilityType.Repeater,
                _ => PowerFacilityType.None
            };
    }

    public static IReadOnlyList<GridPoint> GetOrderedInputDirections(FacilityDefinition definition, Rotation rotation) {
        List<GridPoint> directions = FilterPortsByDirection(definition, PortDirection.Input, rotation);
        if (directions.Count > 0) return directions;

        if (definition.Category == FacilityCategory.Splitter)
            return SplitterInputOrder.Select(direction => RotateDirection(direction, rotation)).ToArray();

        return definition.MaxInputs > 0 ? [RotateDirection(new GridPoint(-1, 0), rotation)] : [];
    }

    public static IReadOnlyList<GridPoint>
        GetOrderedOutputDirections(FacilityDefinition definition, Rotation rotation) {
        List<GridPoint> directions = FilterPortsByDirection(definition, PortDirection.Output, rotation);
        if (directions.Count > 0) return directions;

        if (definition.Category == FacilityCategory.Splitter || definition.MaxOutputs > 1)
            return SplitterOutputOrder.Select(direction => RotateDirection(direction, rotation)).ToArray();

        return definition.MaxOutputs > 0 ? [RotateDirection(new GridPoint(1, 0), rotation)] : [];
    }

    public static double DefaultTraversalCapacity(FacilityDefinition definition) {
        return Math.Max(Math.Max(definition.BaseThroughput, definition.ProductionRate),
            Math.Max(definition.ConsumptionRate, 1.0));
    }

    private static List<GridPoint> FilterPortsByDirection(
        FacilityDefinition definition,
        PortDirection filterDirection,
        Rotation rotation) {
        List<GridPoint> directions = new();
        foreach (var ioPort in definition.IoPorts) {
            if (ioPort.PortDirection != filterDirection) continue;

            directions.Add(RotateDirection(ioPort.Direction, rotation));
        }

        return directions;
    }
}