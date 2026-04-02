using EndfieldBase.Core.Models;

namespace EndfieldBase.Core.Power;

public sealed class FacilityPowerState
{
    public int InstanceId { get; set; }
    public bool Covered { get; set; }
    public bool Powered { get; set; }
    public double RequestedPower { get; set; }
    public double ServedPower { get; set; }
}

public sealed class PowerReport
{
    public double TotalGeneration { get; set; }
    public double TotalRequested { get; set; }
    public double TotalServed { get; set; }
    public List<FacilityPowerState> FacilityStates { get; } = [];
}

public static class PowerSystem
{
    public static PowerReport EvaluatePower(SimulationState state) {
        PowerReport report = new();
        List<(int FacilityStateIndex, double RequestedPower)> coveredConsumers = new();

        foreach (var instance in state.Grid.GetFacilities()) {
            var definition = state.Catalog.FindDefinition(instance.DefinitionId);
            if (definition is null) continue;

            FacilityPowerState facilityState = new()
                {
                    InstanceId = instance.InstanceId,
                    RequestedPower = definition.PowerUsage,
                    Covered = !FacilityRules.RequiresExternalPower(definition) ||
                              IsCoveredByAnyPowerPole(state, instance, definition)
                };

            if (IsGeneratingFacilityActive(definition, instance)) report.TotalGeneration += definition.PowerGeneration;

            if (definition.PowerUsage > 0.0) report.TotalRequested += definition.PowerUsage;

            if (!FacilityRules.RequiresExternalPower(definition)) {
                facilityState.Powered = true;
                facilityState.ServedPower = definition.PowerUsage;
                report.TotalServed += definition.PowerUsage;
            }
            else if (facilityState.Covered) {
                coveredConsumers.Add((report.FacilityStates.Count, definition.PowerUsage));
            }

            report.FacilityStates.Add(facilityState);
        }

        coveredConsumers.Sort((left, right) =>
            report.FacilityStates[left.FacilityStateIndex].InstanceId
                .CompareTo(report.FacilityStates[right.FacilityStateIndex].InstanceId));

        var remainingGeneration = report.TotalGeneration;
        foreach (var consumer in coveredConsumers) {
            var facilityState = report.FacilityStates[consumer.FacilityStateIndex];
            if (remainingGeneration < consumer.RequestedPower) continue;

            facilityState.Powered = true;
            facilityState.ServedPower = consumer.RequestedPower;
            remainingGeneration -= consumer.RequestedPower;
            report.TotalServed += consumer.RequestedPower;
        }

        return report;
    }

    private static bool IsCoveredByAnyPowerPole(
        SimulationState state,
        FacilityInstance target,
        FacilityDefinition targetDefinition) {
        List<GridPoint> targetCells = state.Grid.GetOccupiedCells(targetDefinition, target);
        foreach (var candidate in state.Grid.GetFacilities()) {
            var candidateDefinition = state.Catalog.FindDefinition(candidate.DefinitionId);
            if (candidateDefinition is null || !FacilityRules.IsPowerFacility(candidateDefinition) ||
                candidateDefinition.PowerRange <= 0) continue;

            List<GridPoint> poleCells = state.Grid.GetOccupiedCells(candidateDefinition, candidate);
            foreach (var targetCell in targetCells)
                foreach (var poleCell in poleCells) {
                    var manhattanDistance = Math.Abs(targetCell.X - poleCell.X) + Math.Abs(targetCell.Y - poleCell.Y);
                    if (manhattanDistance <= candidateDefinition.PowerRange) return true;
                }
        }

        return false;
    }

    private static bool IsGeneratingFacilityActive(FacilityDefinition definition, FacilityInstance instance) {
        if (definition.PowerGeneration <= 0.0) return false;

        if (definition.Category == FacilityCategory.Generator) return true;

        return !FacilityRules.IsThermalMachine(definition) || FacilityRules.HasAnyStoredItems(instance);
    }
}