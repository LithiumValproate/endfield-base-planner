#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace endfield_base {
enum class FacilityCategory {
    Machine,
    Conveyor,
    Splitter,
    Merger,
    Bridge,
    Power_pole,
    Power_relay,
    Generator,
    Chest,
    Hub,
    Storage_port,
    Servo,
    Storage_in,
    Storage_out,
};

enum class FacilityRole {
    Generic,
    Thermal_machine,
    Power_facility,
    Production_facility,
    Chest,
    Hub,
    Storage_port,
    Servo,
};

enum class Rotation {
    Deg_0,
    Deg_90,
    Deg_180,
    Deg_270,
};

enum class PortDirection {
    Input,
    Output,
};

enum class StorageMode {
    Conveyor,
    Wireless,
};

enum class PowerFacilityType {
    None,
    Pole,
    Repeater,
};

struct GridPoint {
    int x = 0;
    int y = 0;

    auto operator==(const GridPoint& other) const
        -> bool = default;
};

struct GridSize {
    int width = 1;
    int height = 1;

    auto operator==(const GridSize& other) const
        -> bool = default;
};

struct IoPortDefinition {
    GridPoint direction;
    PortDirection portDirection = PortDirection::Input;
};

struct InventorySlot {
    std::string itemId;
    int count = 0;
};

struct FacilityDefinition {
    std::string id;
    std::string displayName;
    FacilityCategory category = FacilityCategory::Machine;
    FacilityRole role = FacilityRole::Generic;
    GridSize footprint{1, 1};
    double baseThroughput = 0.0;
    double productionRate = 0.0;
    double consumptionRate = 0.0;
    double powerUsage = 0.0;
    double powerGeneration = 0.0;
    int powerRange = 0;
    int maxInputs = 0;
    int maxOutputs = 0;
    int inventorySlotCount = 0;
    int itemStackLimit = 0;
    bool requiresPower = false;
    bool supportsWirelessStorage = false;
    bool occupiesBaseGrid = true;
    bool canMountOnConveyor = false;
    bool canAttachToStorageLine = false;
    PowerFacilityType powerFacilityType = PowerFacilityType::None;
    std::vector<IoPortDefinition> ioPorts;
};

struct FacilityInstance {
    int instanceId = 0;
    std::string definitionId;
    GridPoint position;
    Rotation rotation = Rotation::Deg_0;
    StorageMode storageMode = StorageMode::Conveyor;
    std::vector<InventorySlot> inventorySlots;
    int passedItemCount = 0;
    std::optional<int> passedItemLimit;
};

class PowerFacilityInterface {
public:
    virtual ~PowerFacilityInterface() = default;

    [[nodiscard]] virtual auto getPowerRange(const FacilityDefinition& definition) const
        -> int = 0;
};

class ProductionFacilityInterface {
public:
    virtual ~ProductionFacilityInterface() = default;

    [[nodiscard]] virtual auto getInputDirections(
        const FacilityDefinition& definition,
        Rotation rotation
    ) const -> std::vector<GridPoint> = 0;
    [[nodiscard]] virtual auto getOutputDirections(
        const FacilityDefinition& definition,
        Rotation rotation
    ) const -> std::vector<GridPoint> = 0;
};

class FacilityCatalog {
public:
    void addDefinition(FacilityDefinition definition);
    [[nodiscard]] auto findDefinition(std::string_view id) const
        -> const FacilityDefinition*;
    [[nodiscard]] auto getDefinitions() const
        -> const std::vector<FacilityDefinition>&;
    [[nodiscard]] auto empty() const
        -> bool;

private:
    std::vector<FacilityDefinition> definitions_;
    std::unordered_map<std::string, std::size_t> indexById_;
};

[[nodiscard]] auto toString(FacilityCategory category)
    -> std::string;
[[nodiscard]] auto facilityCategoryFromString(std::string_view value)
    -> FacilityCategory;
[[nodiscard]] auto toString(FacilityRole role)
    -> std::string;
[[nodiscard]] auto facilityRoleFromString(std::string_view value)
    -> FacilityRole;
[[nodiscard]] auto toString(Rotation rotation)
    -> std::string;
[[nodiscard]] auto rotationFromString(std::string_view value)
    -> Rotation;
[[nodiscard]] auto toString(PortDirection portDirection)
    -> std::string;
[[nodiscard]] auto portDirectionFromString(std::string_view value)
    -> PortDirection;
[[nodiscard]] auto toString(StorageMode storageMode)
    -> std::string;
[[nodiscard]] auto storageModeFromString(std::string_view value)
    -> StorageMode;
[[nodiscard]] auto toString(PowerFacilityType powerFacilityType)
    -> std::string;
[[nodiscard]] auto powerFacilityTypeFromString(std::string_view value)
    -> PowerFacilityType;
[[nodiscard]] auto rotateFootprint(GridSize footprint, Rotation rotation)
    -> GridSize;
[[nodiscard]] auto rotateDirection(GridPoint direction, Rotation rotation)
    -> GridPoint;
[[nodiscard]] auto isLogisticsFacility(FacilityCategory category)
    -> bool;
[[nodiscard]] auto ignoresPowerCoverage(FacilityCategory category)
    -> bool;
[[nodiscard]] auto isPowerTransmitter(FacilityCategory category)
    -> bool;
[[nodiscard]] auto supportsBridgeLayer(FacilityCategory category)
    -> bool;
[[nodiscard]] auto isStorageFacility(FacilityCategory category)
    -> bool;
[[nodiscard]] auto isMachineFacility(FacilityCategory category)
    -> bool;
[[nodiscard]] auto isProductionFacility(const FacilityDefinition& definition)
    -> bool;
[[nodiscard]] auto isThermalMachine(const FacilityDefinition& definition)
    -> bool;
[[nodiscard]] auto isPowerFacility(const FacilityDefinition& definition)
    -> bool;
[[nodiscard]] auto isHubFacility(const FacilityDefinition& definition)
    -> bool;
[[nodiscard]] auto isChestFacility(const FacilityDefinition& definition)
    -> bool;
[[nodiscard]] auto requiresExternalPower(const FacilityDefinition& definition)
    -> bool;
[[nodiscard]] auto usesWirelessStorage(const FacilityDefinition& definition, const FacilityInstance& instance)
    -> bool;
[[nodiscard]] auto hasInventoryCapacity(const FacilityDefinition& definition)
    -> bool;
[[nodiscard]] auto hasAnyStoredItems(const FacilityInstance& instance)
    -> bool;
[[nodiscard]] auto defaultFacilityRole(FacilityCategory category)
    -> FacilityRole;
[[nodiscard]] auto defaultRequiresPower(const FacilityDefinition& definition)
    -> bool;
[[nodiscard]] auto defaultPowerFacilityType(FacilityCategory category)
    -> PowerFacilityType;
[[nodiscard]] auto getOrderedInputDirections(const FacilityDefinition& definition, Rotation rotation)
    -> std::vector<GridPoint>;
[[nodiscard]] auto getOrderedOutputDirections(const FacilityDefinition& definition, Rotation rotation)
    -> std::vector<GridPoint>;
[[nodiscard]] auto createPowerFacilityInterface(const FacilityDefinition& definition)
    -> std::unique_ptr<PowerFacilityInterface>;
[[nodiscard]] auto createProductionFacilityInterface(const FacilityDefinition& definition)
    -> std::unique_ptr<ProductionFacilityInterface>;
[[nodiscard]] auto defaultTraversalCapacity(const FacilityDefinition& definition)
    -> double;
}  // namespace endfield_base
