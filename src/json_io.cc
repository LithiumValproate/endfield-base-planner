#include "endfield_base/json_io.h"

#include <fstream>
#include <stdexcept>
#include <string_view>

#include <nlohmann/json.hpp>

namespace endfield_base {
namespace {
    nlohmann::json readJson(const std::filesystem::path& path) {
        std::ifstream input(path);
        if (!input.is_open()) {
            throw std::runtime_error("Failed to open json file: " + path.string());
        }

        nlohmann::json json;
        input >> json;
        return json;
    }

    std::filesystem::path resolveCatalogPath(const std::filesystem::path& mapPath, std::string_view catalogPath) {
        const std::filesystem::path candidate(catalogPath);
        if (candidate.is_absolute()) {
            return candidate;
        }
        return mapPath.parent_path() / candidate;
    }
}  // namespace

FacilityCatalog loadFacilityDefinitions(const std::filesystem::path& path) {
    const nlohmann::json json = readJson(path);
    FacilityCatalog catalog;

    for (const nlohmann::json& definitionJson : json.at("facilities")) {
        FacilityDefinition definition;
        definition.id = definitionJson.at("id").get<std::string>();
        definition.displayName = definitionJson.at("displayName").get<std::string>();
        definition.category = facilityCategoryFromString(definitionJson.at("category").get<std::string>());
        definition.role = definitionJson.contains("role")
                              ? facilityRoleFromString(definitionJson.at("role").get<std::string>())
                              : defaultFacilityRole(definition.category);
        definition.footprint.width = definitionJson.at("footprint").at("width").get<int>();
        definition.footprint.height = definitionJson.at("footprint").at("height").get<int>();
        definition.baseThroughput = definitionJson.value("baseThroughput", 0.0);
        definition.productionRate = definitionJson.value("productionRate", 0.0);
        definition.consumptionRate = definitionJson.value("consumptionRate", 0.0);
        definition.powerUsage = definitionJson.value("powerUsage", 0.0);
        definition.powerGeneration = definitionJson.value("powerGeneration", 0.0);
        definition.powerRange = definitionJson.value("powerRange", 0);
        definition.maxInputs = definitionJson.value("maxInputs", 0);
        definition.maxOutputs = definitionJson.value("maxOutputs", 0);
        definition.inventorySlotCount = definitionJson.value("inventorySlotCount", 0);
        definition.itemStackLimit = definitionJson.value(
            "itemStackLimit",
            definition.inventorySlotCount > 0 ? 50 : 0
        );
        definition.requiresPower = definitionJson.value("requiresPower", defaultRequiresPower(definition));
        definition.supportsWirelessStorage = definitionJson.value("supportsWirelessStorage", false);
        definition.occupiesBaseGrid = definitionJson.value("occupiesBaseGrid", true);
        definition.canMountOnConveyor = definitionJson.value("canMountOnConveyor", false);
        definition.canAttachToStorageLine = definitionJson.value("canAttachToStorageLine", false);
        definition.powerFacilityType = definitionJson.contains("powerFacilityType")
                                           ? powerFacilityTypeFromString(
                                               definitionJson.at("powerFacilityType").get<std::string>()
                                           )
                                           : defaultPowerFacilityType(definition.category);

        if (definitionJson.contains("ioPorts")) {
            for (const nlohmann::json& ioPortJson : definitionJson.at("ioPorts")) {
                definition.ioPorts.push_back({
                        {
                            ioPortJson.at("direction").at("x").get<int>(),
                            ioPortJson.at("direction").at("y").get<int>(),
                        },
                        portDirectionFromString(ioPortJson.at("portDirection").get<std::string>()),
                    });
            }
        }
        catalog.addDefinition(std::move(definition));
    }

    return catalog;
}

SimulationState loadMap(const std::filesystem::path& path) {
    const nlohmann::json json = readJson(path);

    SimulationState state;
    state.facilityCatalogPath = json.at("facilityCatalog").get<std::string>();
    state.loadedFrom = path;
    state.catalog = loadFacilityDefinitions(resolveCatalogPath(path, state.facilityCatalogPath));
    state.grid = GridMap(json.at("width").get<int>(), json.at("height").get<int>());
    state.grid.setNextInstanceId(json.value("nextInstanceId", 1));

    for (const nlohmann::json& instanceJson : json.at("facilities")) {
        const std::string definitionId = instanceJson.at("definitionId").get<std::string>();
        const FacilityDefinition* definition = state.catalog.findDefinition(definitionId);
        if (definition == nullptr) {
            throw std::runtime_error("Map references unknown definition: " + definitionId);
        }

        std::string reason;
        FacilityInstance facilityInstance;
        facilityInstance.instanceId = instanceJson.value("instanceId", 0);
        facilityInstance.definitionId = definitionId;
        facilityInstance.position = {
                instanceJson.at("position").at("x").get<int>(),
                instanceJson.at("position").at("y").get<int>(),
            };
        facilityInstance.rotation = rotationFromString(instanceJson.at("rotation").get<std::string>());
        facilityInstance.storageMode = instanceJson.contains("storageMode")
                                           ? storageModeFromString(instanceJson.at("storageMode").get<std::string>())
                                           : StorageMode::Conveyor;
        facilityInstance.passedItemCount = instanceJson.value("passedItemCount", 0);
        if (instanceJson.contains("passedItemLimit") && !instanceJson.at("passedItemLimit").is_null()) {
            facilityInstance.passedItemLimit = instanceJson.at("passedItemLimit").get<int>();
        }
        if (instanceJson.contains("inventorySlots")) {
            for (const nlohmann::json& inventoryJson : instanceJson.at("inventorySlots")) {
                facilityInstance.inventorySlots.push_back({
                        inventoryJson.value("itemId", std::string{}),
                        inventoryJson.value("count", 0),
                    });
            }
        }

        if (facilityInstance.inventorySlots.empty() && definition->inventorySlotCount > 0) {
            facilityInstance.inventorySlots.resize(static_cast<std::size_t>(definition->inventorySlotCount));
        }

        const int instanceId = state.grid.placeFacility(
            *definition,
            definitionId,
            facilityInstance.position,
            facilityInstance.rotation,
            facilityInstance.instanceId,
            &reason
        );

        if (instanceId == 0) {
            throw std::runtime_error("Failed to place instance from map: " + reason);
        }

        if (FacilityInstance* placedInstance = state.grid.findFacility(instanceId); placedInstance != nullptr) {
            placedInstance->storageMode = facilityInstance.storageMode;
            placedInstance->inventorySlots = facilityInstance.inventorySlots;
            placedInstance->passedItemCount = facilityInstance.passedItemCount;
            placedInstance->passedItemLimit = facilityInstance.passedItemLimit;
        }
    }

    state.grid.setNextInstanceId(std::max(state.grid.getNextInstanceId(), json.value("nextInstanceId", 1)));
    return state;
}

void saveMap(const std::filesystem::path& path, const SimulationState& state) {
    nlohmann::json json;
    json["facilityCatalog"] = state.facilityCatalogPath;
    json["width"] = state.grid.getWidth();
    json["height"] = state.grid.getHeight();
    json["nextInstanceId"] = state.grid.getNextInstanceId();
    json["facilities"] = nlohmann::json::array();

    for (const FacilityInstance& instance : state.grid.getFacilities()) {
        nlohmann::json instanceJson = {
                {"instanceId", instance.instanceId},
                {"definitionId", instance.definitionId},
                {"position", {{"x", instance.position.x}, {"y", instance.position.y}}},
                {"rotation", toString(instance.rotation)},
                {"storageMode", toString(instance.storageMode)},
                {"passedItemCount", instance.passedItemCount},
            };
        instanceJson["inventorySlots"] = nlohmann::json::array();
        for (const InventorySlot& inventorySlot : instance.inventorySlots) {
            instanceJson["inventorySlots"].push_back({
                    {"itemId", inventorySlot.itemId},
                    {"count", inventorySlot.count},
                });
        }
        if (instance.passedItemLimit.has_value()) {
            instanceJson["passedItemLimit"] = *instance.passedItemLimit;
        }
        json["facilities"].push_back(std::move(instanceJson));
    }

    std::ofstream output(path);
    output << json.dump(2) << '\n';
}
}  // namespace endfield_base
