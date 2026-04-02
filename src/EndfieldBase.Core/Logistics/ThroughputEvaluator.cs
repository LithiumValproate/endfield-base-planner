using EndfieldBase.Core.Models;
using EndfieldBase.Core.Power;
using EndfieldBase.Core.Storage;

namespace EndfieldBase.Core.Logistics;

public sealed class FacilityThroughputResult
{
    public int InstanceId { get; set; }
    public string DefinitionId { get; set; } = string.Empty;
    public bool Powered { get; set; }
    public double ThroughputPerSecond { get; set; }
    public double ProductionPerSecond { get; set; }
    public double ConsumptionPerSecond { get; set; }
    public double Utilization { get; set; }
    public int PathLengthFromInput { get; set; } = -1;
    public int PathLengthToOutput { get; set; } = -1;
    public int BottleneckInstanceId { get; set; }
    public string BottleneckReason { get; set; } = string.Empty;
}

public sealed class NetworkPathResult
{
    public int FromInstanceId { get; set; }
    public int ToInstanceId { get; set; }
    public int PathLength { get; set; }
    public double BottleneckThroughput { get; set; }
    public string PathRole { get; set; } = string.Empty;
}

public sealed class ResolvedPath
{
    public int TargetInstanceId { get; set; }
    public required PathResult Path { get; set; }
}

public sealed class ThroughputReport
{
    public double TotalThroughput { get; set; }
    public double TotalProduction { get; set; }
    public double TotalConsumption { get; set; }
    public double TotalInput { get; set; }
    public double TotalOutput { get; set; }
    public List<FacilityThroughputResult> FacilityResults { get; } = [];
    public List<NetworkPathResult> NetworkResults { get; } = [];
}

public static class ThroughputEvaluator
{
    public static ThroughputReport EvaluateThroughput(SimulationState state) {
        ThroughputReport report = new();
        var powerReport = PowerSystem.EvaluatePower(state);
        var endpoints = StorageSystem.CollectEndpoints(state);
        Dictionary<int, double> pathUsageByInstanceId = new();

        static FacilityPowerState FindPowerState(PowerReport powerReport, int instanceId) {
            return powerReport.FacilityStates.FirstOrDefault(state => state.InstanceId == instanceId) ??
                   new FacilityPowerState();
        }

        bool IsActiveInstance(int instanceId) {
            var instance = state.Grid.FindFacility(instanceId);
            if (instance is null) return false;

            var definition = state.Catalog.FindDefinition(instance.DefinitionId);
            if (definition is null) return false;

            var powerState = FindPowerState(powerReport, instanceId);
            return powerState.Powered || !FacilityRules.RequiresExternalPower(definition);
        }

        endpoints.InputInstanceIds.RemoveAll(instanceId => !IsActiveInstance(instanceId));
        endpoints.OutputInstanceIds.RemoveAll(instanceId => !IsActiveInstance(instanceId));

        foreach (var instance in state.Grid.GetFacilities()) {
            var definition = state.Catalog.FindDefinition(instance.DefinitionId);
            if (definition is null) continue;

            FacilityThroughputResult facilityResult = new()
                {
                    InstanceId = instance.InstanceId,
                    DefinitionId = definition.Id,
                    Powered = FindPowerState(powerReport, instance.InstanceId).Powered,
                };

            var active = facilityResult.Powered || !FacilityRules.RequiresExternalPower(definition);
            var localLimit = Math.Max(Math.Max(definition.BaseThroughput, definition.ProductionRate),
                Math.Max(definition.ConsumptionRate, 0.0));

            if (FacilityRules.IsProductionFacility(definition) || FacilityRules.IsThermalMachine(definition)) {
                var outputRate = 0.0;
                var inputRate = 0.0;

                if (active && definition.ProductionRate > 0.0) {
                    var outputPath = ResolveBestPath(state, instance.InstanceId, endpoints.OutputInstanceIds);
                    if (outputPath is not null) {
                        var transportLimit = outputPath.Path.BottleneckThroughput /
                                             PathPenaltyMultiplier(outputPath.Path.Length);
                        outputRate = Math.Min(Math.Min(definition.ProductionRate, definition.BaseThroughput),
                            transportLimit);
                        facilityResult.PathLengthToOutput = outputPath.Path.Length;
                        facilityResult.BottleneckInstanceId = outputPath.Path.TraversedInstanceIds.FirstOrDefault();
                        facilityResult.BottleneckReason = "output_path";
                        report.NetworkResults.Add(new NetworkPathResult
                            {
                                FromInstanceId = instance.InstanceId,
                                ToInstanceId = outputPath.TargetInstanceId,
                                PathLength = outputPath.Path.Length,
                                BottleneckThroughput = outputPath.Path.BottleneckThroughput,
                                PathRole = "output",
                            });
                        AccumulatePathUsage(pathUsageByInstanceId, outputPath.Path, outputRate);
                    } else {
                        facilityResult.BottleneckReason = "missing_output_path";
                    }
                }

                if (active && definition.ConsumptionRate > 0.0) {
                    var inputPath = ResolveBestPath(state, instance.InstanceId, endpoints.InputInstanceIds);
                    if (inputPath is not null) {
                        var transportLimit = inputPath.Path.BottleneckThroughput /
                                             PathPenaltyMultiplier(inputPath.Path.Length);
                        inputRate = Math.Min(Math.Min(definition.ConsumptionRate, definition.BaseThroughput),
                            transportLimit);
                        facilityResult.PathLengthFromInput = inputPath.Path.Length;
                        if (string.IsNullOrEmpty(facilityResult.BottleneckReason)) {
                            facilityResult.BottleneckInstanceId = inputPath.Path.TraversedInstanceIds.FirstOrDefault();
                            facilityResult.BottleneckReason = "input_path";
                        }

                        report.NetworkResults.Add(new NetworkPathResult
                            {
                                FromInstanceId = inputPath.TargetInstanceId,
                                ToInstanceId = instance.InstanceId,
                                PathLength = inputPath.Path.Length,
                                BottleneckThroughput = inputPath.Path.BottleneckThroughput,
                                PathRole = "input",
                            });
                        AccumulatePathUsage(pathUsageByInstanceId, inputPath.Path, inputRate);
                    } else if (string.IsNullOrEmpty(facilityResult.BottleneckReason)) {
                        facilityResult.BottleneckReason = "missing_input_path";
                    }
                }

                if (definition.ProductionRate > 0.0 && definition.ConsumptionRate > 0.0) {
                    facilityResult.ThroughputPerSecond = Math.Min(inputRate, outputRate);
                    facilityResult.ProductionPerSecond = facilityResult.ThroughputPerSecond;
                    facilityResult.ConsumptionPerSecond = facilityResult.ThroughputPerSecond;
                } else {
                    facilityResult.ProductionPerSecond = outputRate;
                    facilityResult.ConsumptionPerSecond = inputRate;
                    facilityResult.ThroughputPerSecond = Math.Max(outputRate, inputRate);
                }
            } else if (FacilityRules.IsChestFacility(definition)) {
                if (FacilityRules.UsesWirelessStorage(definition, instance) && active) {
                    facilityResult.ThroughputPerSecond = FacilityRules.HasAnyStoredItems(instance)
                        ? Math.Max(definition.BaseThroughput, 1.0)
                        : 0.0;
                    facilityResult.ProductionPerSecond = facilityResult.ThroughputPerSecond;
                    facilityResult.BottleneckReason = "wireless_storage";
                } else {
                    facilityResult.BottleneckReason = "buffer_storage";
                }
            } else if (FacilityRules.IsHubFacility(definition) || definition.Category == FacilityCategory.StoragePort) {
                facilityResult.BottleneckReason = "warehouse_link";
            } else if (FacilityRules.IsLogisticsFacility(definition.Category)) {
                facilityResult.BottleneckReason = "path_carrier";
            } else if (definition.Category == FacilityCategory.Generator ||
                       FacilityRules.IsThermalMachine(definition)) {
                facilityResult.BottleneckReason = "power_generation";
            }

            if (localLimit > 0.0) facilityResult.Utilization = facilityResult.ThroughputPerSecond / localLimit;

            report.TotalThroughput += facilityResult.ThroughputPerSecond;
            report.TotalProduction += facilityResult.ProductionPerSecond;
            report.TotalConsumption += facilityResult.ConsumptionPerSecond;
            report.FacilityResults.Add(facilityResult);
        }

        foreach (var facilityResult in report.FacilityResults) {
            var definition = state.Catalog.FindDefinition(facilityResult.DefinitionId);
            if (definition is null || !FacilityRules.IsLogisticsFacility(definition.Category)) continue;

            pathUsageByInstanceId.TryGetValue(facilityResult.InstanceId, out var usedRate);
            facilityResult.ThroughputPerSecond = usedRate;
            facilityResult.ProductionPerSecond = usedRate;
            facilityResult.Utilization = definition.BaseThroughput > 0.0 ? usedRate / definition.BaseThroughput : 0.0;
            report.TotalInput += usedRate;
            report.TotalOutput += usedRate;
        }

        report.TotalInput += report.TotalConsumption;
        report.TotalOutput += report.TotalProduction;
        return report;
    }

    private static double PathPenaltyMultiplier(int pathLength) {
        return pathLength <= 1 ? 1.0 : 1.0 + 0.25 * (pathLength - 1);
    }

    private static ResolvedPath? ResolveBestPath(SimulationState state, int startInstanceId, List<int> targets) {
        ResolvedPath? bestPath = null;
        foreach (var targetInstanceId in targets) {
            var path = PathFinder.FindPath(state,
                new PathRequest { StartInstanceId = startInstanceId, EndInstanceId = targetInstanceId });
            if (!path.Found) continue;

            if (bestPath is null || path.Length < bestPath.Path.Length)
                bestPath = new ResolvedPath
                    {
                        TargetInstanceId = targetInstanceId,
                        Path = path,
                    };
        }

        return bestPath;
    }

    private static void AccumulatePathUsage(Dictionary<int, double> usageByInstanceId, PathResult path,
                                            double throughput) {
        foreach (var instanceId in path.TraversedInstanceIds)
            usageByInstanceId[instanceId] = usageByInstanceId.TryGetValue(instanceId, out var existing)
                ? existing + throughput
                : throughput;
    }
}