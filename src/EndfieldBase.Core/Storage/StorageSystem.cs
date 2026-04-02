using EndfieldBase.Core.Models;

namespace EndfieldBase.Core.Storage;

public sealed class StorageEndpoints
{
    public List<int> InputInstanceIds { get; } = [];
    public List<int> OutputInstanceIds { get; } = [];
}

public static class StorageSystem
{
    public static StorageEndpoints CollectEndpoints(SimulationState state) {
        StorageEndpoints endpoints = new();
        foreach (var instance in state.Grid.GetFacilities()) {
            var definition = state.Catalog.FindDefinition(instance.DefinitionId);
            if (definition is null) continue;

            if (definition.Category == FacilityCategory.StorageIn) {
                endpoints.InputInstanceIds.Add(instance.InstanceId);
            }
            else if (definition.Category == FacilityCategory.StorageOut) {
                endpoints.OutputInstanceIds.Add(instance.InstanceId);
            }
            else if (FacilityRules.IsHubFacility(definition) || definition.Category == FacilityCategory.StoragePort) {
                if (definition.MaxInputs > 0) endpoints.OutputInstanceIds.Add(instance.InstanceId);

                if (definition.MaxOutputs > 0) endpoints.InputInstanceIds.Add(instance.InstanceId);
            }
            else if (FacilityRules.UsesWirelessStorage(definition, instance) && definition.MaxOutputs > 0) {
                endpoints.OutputInstanceIds.Add(instance.InstanceId);
            }
        }

        return endpoints;
    }
}