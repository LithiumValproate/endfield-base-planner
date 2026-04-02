using EndfieldBase.Core.Grid;
using EndfieldBase.Core.Models;

namespace EndfieldBase.Core.Tests;

public sealed class GridMapTests
{
    [Fact]
    public void PlaceFacility_ShouldOccupyBaseLayer() {
        var state = CreateState();
        FacilityDefinition conveyor = new()
            {
                Id = "conveyor",
                DisplayName = "Conveyor",
                Category = FacilityCategory.Conveyor,
                Footprint = new GridSize(1, 1),
                BaseThroughput = 8.0,
                MaxInputs = 1,
                MaxOutputs = 1,
            };
        state.Catalog.AddDefinition(conveyor);

        var instanceId = state.Grid.PlaceFacility(
            conveyor,
            conveyor.Id,
            new GridPoint(2, 3),
            Rotation.Deg0,
            out var reason);

        Assert.Equal(0, string.IsNullOrEmpty(reason) ? 0 : 1);
        Assert.NotEqual(0, instanceId);

        var cell = state.Grid.GetCell(2, 3);
        Assert.NotNull(cell);
        Assert.Equal(instanceId, cell!.BaseInstanceId);
    }

    [Fact]
    public void PlaceFacility_ShouldRejectOverlap() {
        var state = CreateState();
        FacilityDefinition machine = new()
            {
                Id = "machine",
                DisplayName = "Machine",
                Category = FacilityCategory.Machine,
                Footprint = new GridSize(2, 2),
            };
        state.Catalog.AddDefinition(machine);

        _ = state.Grid.PlaceFacility(machine, machine.Id, new GridPoint(1, 1), Rotation.Deg0, out _);
        var secondPlacement =
            state.Grid.PlaceFacility(machine, machine.Id, new GridPoint(2, 2), Rotation.Deg0, out var reason);

        Assert.Equal(0, secondPlacement);
        Assert.Equal("base layer already occupied", reason);
    }

    private static SimulationState CreateState() {
        return new SimulationState
            {
                Grid = new GridMap(12, 8),
            };
    }
}