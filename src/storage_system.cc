#include "endfield_base/storage_system.h"

namespace endfield_base {
StorageEndpoints StorageSystem::collectEndpoints(const SimulationState& state) {
    StorageEndpoints endpoints;
    for (const FacilityInstance& instance : state.grid.getFacilities()) {
        const FacilityDefinition* definition = state.catalog.findDefinition(instance.definitionId);
        if (definition == nullptr) {
            continue;
        }

        if (definition->category == FacilityCategory::Storage_in) {
            endpoints.inputInstanceIds.push_back(instance.instanceId);
        } else if (definition->category == FacilityCategory::Storage_out) {
            endpoints.outputInstanceIds.push_back(instance.instanceId);
        } else if (isHubFacility(*definition) || definition->category == FacilityCategory::Storage_port) {
            if (definition->maxInputs > 0) {
                endpoints.outputInstanceIds.push_back(instance.instanceId);
            }
            if (definition->maxOutputs > 0) {
                endpoints.inputInstanceIds.push_back(instance.instanceId);
            }
        } else if (usesWirelessStorage(*definition, instance)) {
            if (definition->maxOutputs > 0) {
                endpoints.outputInstanceIds.push_back(instance.instanceId);
            }
        }
    }

    return endpoints;
}
}  // namespace endfield_base
