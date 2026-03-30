#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace endfield_base {
struct TransparentStringHash {
    using is_transparent = void;

    [[nodiscard]] auto operator()(std::string_view value) const noexcept
        -> std::size_t {
        return std::hash<std::string_view>{}(value);
    }
};

struct TransparentStringEqual {
    using is_transparent = void;

    [[nodiscard]] auto operator()(std::string_view left, std::string_view right) const noexcept
        -> bool {
        return left == right;
    }
};

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

    auto operator==(const GridPoint& other) const -> bool = default;
};

struct GridSize {
    int width = 1;
    int height = 1;

    auto operator==(const GridSize& other) const -> bool = default;
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
    std::unordered_map<std::string, std::size_t, TransparentStringHash, TransparentStringEqual> indexById_;
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

[[nodiscard]] constexpr auto rotateFootprint(GridSize footprint, Rotation rotation) noexcept
    -> GridSize {
    if (rotation == Rotation::Deg_90 || rotation == Rotation::Deg_270) {
        return {footprint.height, footprint.width};
    }

    return footprint;
}

[[nodiscard]] constexpr auto rotateDirection(GridPoint direction, Rotation rotation) noexcept
    -> GridPoint {
    switch (rotation) {
    case Rotation::Deg_0: return direction;
    case Rotation::Deg_90: return {-direction.y, direction.x};
    case Rotation::Deg_180: return {-direction.x, -direction.y};
    case Rotation::Deg_270: return {direction.y, -direction.x};
    }

    return direction;
}

[[nodiscard]] constexpr auto isLogisticsFacility(FacilityCategory category) noexcept
    -> bool {
    return category == FacilityCategory::Conveyor
           || category == FacilityCategory::Splitter
           || category == FacilityCategory::Merger
           || category == FacilityCategory::Bridge;
}

[[nodiscard]] constexpr auto ignoresPowerCoverage(FacilityCategory category) noexcept
    -> bool {
    return isLogisticsFacility(category)
           || category == FacilityCategory::Generator
           || category == FacilityCategory::Power_pole
           || category == FacilityCategory::Power_relay
           || category == FacilityCategory::Hub
           || category == FacilityCategory::Storage_in
           || category == FacilityCategory::Storage_out;
}

[[nodiscard]] constexpr auto isPowerTransmitter(FacilityCategory category) noexcept
    -> bool {
    return category == FacilityCategory::Power_pole || category == FacilityCategory::Power_relay;
}

[[nodiscard]] constexpr auto supportsBridgeLayer(FacilityCategory category) noexcept
    -> bool {
    return category == FacilityCategory::Bridge;
}

[[nodiscard]] constexpr auto isStorageFacility(FacilityCategory category) noexcept
    -> bool {
    return category == FacilityCategory::Storage_in
           || category == FacilityCategory::Storage_out
           || category == FacilityCategory::Storage_port;
}

[[nodiscard]] constexpr auto isMachineFacility(FacilityCategory category) noexcept
    -> bool {
    return category == FacilityCategory::Machine
           || category == FacilityCategory::Chest
           || category == FacilityCategory::Hub;
}

[[nodiscard]] constexpr auto isProductionFacility(const FacilityDefinition& definition) noexcept
    -> bool {
    return definition.role == FacilityRole::Production_facility || definition.category == FacilityCategory::Machine;
}

[[nodiscard]] constexpr auto isThermalMachine(const FacilityDefinition& definition) noexcept
    -> bool {
    return definition.role == FacilityRole::Thermal_machine || definition.category == FacilityCategory::Generator;
}

[[nodiscard]] constexpr auto isPowerFacility(const FacilityDefinition& definition) noexcept
    -> bool {
    return definition.role == FacilityRole::Power_facility || isPowerTransmitter(definition.category);
}

[[nodiscard]] constexpr auto isHubFacility(const FacilityDefinition& definition) noexcept
    -> bool {
    return definition.role == FacilityRole::Hub || definition.category == FacilityCategory::Hub;
}

[[nodiscard]] constexpr auto isChestFacility(const FacilityDefinition& definition) noexcept
    -> bool {
    return definition.role == FacilityRole::Chest || definition.category == FacilityCategory::Chest;
}

[[nodiscard]] constexpr auto requiresExternalPower(const FacilityDefinition& definition) noexcept
    -> bool {
    return !ignoresPowerCoverage(definition.category) && definition.requiresPower;
}

[[nodiscard]] constexpr auto usesWirelessStorage(
    const FacilityDefinition& definition,
    const FacilityInstance& instance
) noexcept
    -> bool {
    return isChestFacility(definition)
           && definition.supportsWirelessStorage
           && instance.storageMode == StorageMode::Wireless;
}

[[nodiscard]] constexpr auto hasInventoryCapacity(const FacilityDefinition& definition) noexcept
    -> bool {
    return definition.inventorySlotCount > 0;
}

[[nodiscard]] auto hasAnyStoredItems(const FacilityInstance& instance)
    -> bool;

[[nodiscard]] constexpr auto defaultFacilityRole(FacilityCategory category) noexcept
    -> FacilityRole {
    switch (category) {
    case FacilityCategory::Machine: return FacilityRole::Production_facility;
    case FacilityCategory::Power_pole:
    case FacilityCategory::Power_relay: return FacilityRole::Power_facility;
    case FacilityCategory::Generator: return FacilityRole::Thermal_machine;
    case FacilityCategory::Chest: return FacilityRole::Chest;
    case FacilityCategory::Hub: return FacilityRole::Hub;
    case FacilityCategory::Storage_port:
    case FacilityCategory::Storage_in:
    case FacilityCategory::Storage_out: return FacilityRole::Storage_port;
    case FacilityCategory::Servo: return FacilityRole::Servo;
    default: return FacilityRole::Generic;
    }
}

[[nodiscard]] constexpr auto defaultRequiresPower(const FacilityDefinition& definition) noexcept
    -> bool {
    return definition.category == FacilityCategory::Chest
           || definition.role == FacilityRole::Production_facility
           || definition.role == FacilityRole::Chest;
}

[[nodiscard]] constexpr auto defaultPowerFacilityType(FacilityCategory category) noexcept
    -> PowerFacilityType {
    switch (category) {
    case FacilityCategory::Power_pole: return PowerFacilityType::Pole;
    case FacilityCategory::Power_relay: return PowerFacilityType::Repeater;
    default: return PowerFacilityType::None;
    }
}

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
