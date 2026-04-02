using System.Collections.ObjectModel;

namespace EndfieldBase.Core.Models;

public enum FacilityCategory
{
    Machine,
    Conveyor,
    Splitter,
    Merger,
    Bridge,
    PowerPole,
    PowerRelay,
    Generator,
    Chest,
    Hub,
    StoragePort,
    Servo,
    StorageIn,
    StorageOut
}

public enum FacilityRole
{
    Generic,
    ThermalMachine,
    PowerFacility,
    ProductionFacility,
    Chest,
    Hub,
    StoragePort,
    Servo
}

public enum Rotation
{
    Deg0,
    Deg90,
    Deg180,
    Deg270
}

public enum PortDirection
{
    Input,
    Output
}

public enum StorageMode
{
    Conveyor,
    Wireless
}

public enum PowerFacilityType
{
    None,
    Pole,
    Repeater
}

public readonly record struct GridPoint(int X, int Y)
{
    public static GridPoint operator +(GridPoint left, GridPoint right) {
        return new GridPoint(left.X + right.X, left.Y + right.Y);
    }

    public static GridPoint operator -(GridPoint left, GridPoint right) {
        return new GridPoint(left.X - right.X, left.Y - right.Y);
    }
}

public readonly record struct GridSize(int Width, int Height);

public sealed class IoPortDefinition
{
    public GridPoint Direction { get; init; }
    public PortDirection PortDirection { get; init; } = PortDirection.Input;
}

public sealed class InventorySlot
{
    public string ItemId { get; set; } = string.Empty;
    public int Count { get; set; }
}

public sealed class FacilityDefinition
{
    public string Id { get; set; } = string.Empty;
    public string DisplayName { get; set; } = string.Empty;
    public FacilityCategory Category { get; set; } = FacilityCategory.Machine;
    public FacilityRole Role { get; set; } = FacilityRole.Generic;
    public GridSize Footprint { get; set; } = new(1, 1);
    public double BaseThroughput { get; set; }
    public double ProductionRate { get; set; }
    public double ConsumptionRate { get; set; }
    public double PowerUsage { get; set; }
    public double PowerGeneration { get; set; }
    public int PowerRange { get; set; }
    public int MaxInputs { get; set; }
    public int MaxOutputs { get; set; }
    public int InventorySlotCount { get; set; }
    public int ItemStackLimit { get; set; }
    public bool RequiresPower { get; set; }
    public bool SupportsWirelessStorage { get; set; }
    public bool OccupiesBaseGrid { get; set; } = true;
    public bool CanMountOnConveyor { get; set; }
    public bool CanAttachToStorageLine { get; set; }
    public PowerFacilityType PowerFacilityType { get; set; } = PowerFacilityType.None;
    public Collection<IoPortDefinition> IoPorts { get; } = [];
}