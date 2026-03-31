#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace endfield_base {
// Hash helper that allows heterogeneous lookup by string-like views.
struct TransparentStringHash {
    using is_transparent = void;

    [[nodiscard]] auto operator()(std::string_view value) const noexcept
        -> std::size_t {
        return std::hash<std::string_view>{}(value);
    }
};

// Equality helper paired with TransparentStringHash for transparent lookup.
struct TransparentStringEqual {
    using is_transparent = void;

    [[nodiscard]] auto operator()(std::string_view left, std::string_view right) const noexcept
        -> bool {
        return left == right;
    }
};

// Enumerates every supported facility category in the simulation.
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

// Describes the high-level gameplay role of a facility definition.
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

// Represents the clockwise rotation applied to a placed facility.
enum class Rotation {
    Deg_0,
    Deg_90,
    Deg_180,
    Deg_270,
};

// Marks whether an IO port consumes or emits items.
enum class PortDirection {
    Input,
    Output,
};

// Selects how a storage-capable instance exchanges items.
enum class StorageMode {
    Conveyor,
    Wireless,
};

// Distinguishes power poles from repeaters for coverage logic.
enum class PowerFacilityType {
    None,
    Pole,
    Repeater,
};

// Identifies a single cell on the 2D grid.
struct GridPoint {
    int x = 0;
    int y = 0;

    auto operator==(const GridPoint& other) const -> bool = default;
};

// Stores the unrotated width and height of a facility footprint.
struct GridSize {
    int width = 1;
    int height = 1;

    auto operator==(const GridSize& other) const -> bool = default;
};

// Defines one directional IO port relative to the facility origin.
struct IoPortDefinition {
    GridPoint direction;
    PortDirection portDirection = PortDirection::Input;
};

// Represents one inventory stack stored inside an instance.
struct InventorySlot {
    std::string itemId;
    int count = 0;
};

// Declares the static properties of a facility type loaded from JSON.
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

// Stores the mutable state of one placed facility on the map.
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

// Polymorphic interface for facilities that provide power coverage.
class PowerFacilityInterface {
public:
    virtual ~PowerFacilityInterface() = default;

    // Returns the effective coverage range for a power facility definition.
    [[nodiscard]] virtual auto getPowerRange(const FacilityDefinition& definition) const
        -> int = 0;
};

// Polymorphic interface for facilities that expose directional item flow.
class ProductionFacilityInterface {
public:
    virtual ~ProductionFacilityInterface() = default;

    // Returns all input directions after applying the instance rotation.
    [[nodiscard]] virtual auto getInputDirections(
        const FacilityDefinition& definition,
        Rotation rotation
    ) const -> std::vector<GridPoint> = 0;
    // Returns all output directions after applying the instance rotation.
    [[nodiscard]] virtual auto getOutputDirections(
        const FacilityDefinition& definition,
        Rotation rotation
    ) const -> std::vector<GridPoint> = 0;
};

// Owns all loaded facility definitions and supports lookup by id.
class FacilityCatalog {
public:
    // Adds or replaces a facility definition in the catalog.
    void addDefinition(FacilityDefinition definition);
    // Finds a definition by id and returns nullptr when it is missing.
    [[nodiscard]] auto findDefinition(std::string_view id) const
        -> const FacilityDefinition*;
    // Returns the full ordered definition list.
    [[nodiscard]] auto getDefinitions() const
        -> const std::vector<FacilityDefinition>&;
    // Reports whether the catalog currently contains any definitions.
    [[nodiscard]] auto empty() const
        -> bool;

private:
    std::vector<FacilityDefinition> definitions_;
    std::unordered_map<std::string, std::size_t, TransparentStringHash, TransparentStringEqual> indexById_;
};

// Converts a facility category to its JSON/string representation.
[[nodiscard]] auto toString(FacilityCategory category)
    -> std::string;
// Parses a facility category from its JSON/string representation.
[[nodiscard]] auto facilityCategoryFromString(std::string_view value)
    -> FacilityCategory;
// Converts a facility role to its JSON/string representation.
[[nodiscard]] auto toString(FacilityRole role)
    -> std::string;
// Parses a facility role from its JSON/string representation.
[[nodiscard]] auto facilityRoleFromString(std::string_view value)
    -> FacilityRole;
// Converts a rotation to its JSON/string representation.
[[nodiscard]] auto toString(Rotation rotation)
    -> std::string;
// Parses a rotation from its JSON/string representation.
[[nodiscard]] auto rotationFromString(std::string_view value)
    -> Rotation;
// Converts a port direction to its JSON/string representation.
[[nodiscard]] auto toString(PortDirection portDirection)
    -> std::string;
// Parses a port direction from its JSON/string representation.
[[nodiscard]] auto portDirectionFromString(std::string_view value)
    -> PortDirection;
// Converts a storage mode to its JSON/string representation.
[[nodiscard]] auto toString(StorageMode storageMode)
    -> std::string;
// Parses a storage mode from its JSON/string representation.
[[nodiscard]] auto storageModeFromString(std::string_view value)
    -> StorageMode;
// Converts a power facility type to its JSON/string representation.
[[nodiscard]] auto toString(PowerFacilityType powerFacilityType)
    -> std::string;
// Parses a power facility type from its JSON/string representation.
[[nodiscard]] auto powerFacilityTypeFromString(std::string_view value)
    -> PowerFacilityType;

// Returns the footprint dimensions after applying rotation.
[[nodiscard]] constexpr auto rotateFootprint(GridSize footprint, Rotation rotation) noexcept
    -> GridSize {
    if (rotation == Rotation::Deg_90 || rotation == Rotation::Deg_270) {
        return {footprint.height, footprint.width};
    }

    return footprint;
}

// Rotates one relative direction vector clockwise.
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

// Reports whether the category participates in item logistics traversal.
[[nodiscard]] constexpr auto isLogisticsFacility(FacilityCategory category) noexcept
    -> bool {
    return category == FacilityCategory::Conveyor
           || category == FacilityCategory::Splitter
           || category == FacilityCategory::Merger
           || category == FacilityCategory::Bridge;
}

// Reports whether the category ignores external power coverage checks.
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

// Reports whether the category transmits power to nearby facilities.
[[nodiscard]] constexpr auto isPowerTransmitter(FacilityCategory category) noexcept
    -> bool {
    return category == FacilityCategory::Power_pole || category == FacilityCategory::Power_relay;
}

// Reports whether the category occupies the bridge traversal layer.
[[nodiscard]] constexpr auto supportsBridgeLayer(FacilityCategory category) noexcept
    -> bool {
    return category == FacilityCategory::Bridge;
}

// Reports whether the category is part of the storage endpoint system.
[[nodiscard]] constexpr auto isStorageFacility(FacilityCategory category) noexcept
    -> bool {
    return category == FacilityCategory::Storage_in
           || category == FacilityCategory::Storage_out
           || category == FacilityCategory::Storage_port;
}

// Reports whether the category behaves like a placeable machine-sized building.
[[nodiscard]] constexpr auto isMachineFacility(FacilityCategory category) noexcept
    -> bool {
    return category == FacilityCategory::Machine
           || category == FacilityCategory::Chest
           || category == FacilityCategory::Hub;
}

// Reports whether the definition produces or consumes items.
[[nodiscard]] constexpr auto isProductionFacility(const FacilityDefinition& definition) noexcept
    -> bool {
    return definition.role == FacilityRole::Production_facility || definition.category == FacilityCategory::Machine;
}

// Reports whether the definition is treated as a thermal machine.
[[nodiscard]] constexpr auto isThermalMachine(const FacilityDefinition& definition) noexcept
    -> bool {
    return definition.role == FacilityRole::Thermal_machine || definition.category == FacilityCategory::Generator;
}

// Reports whether the definition belongs to the power system.
[[nodiscard]] constexpr auto isPowerFacility(const FacilityDefinition& definition) noexcept
    -> bool {
    return definition.role == FacilityRole::Power_facility || isPowerTransmitter(definition.category);
}

// Reports whether the definition represents the central hub.
[[nodiscard]] constexpr auto isHubFacility(const FacilityDefinition& definition) noexcept
    -> bool {
    return definition.role == FacilityRole::Hub || definition.category == FacilityCategory::Hub;
}

// Reports whether the definition behaves like a chest inventory container.
[[nodiscard]] constexpr auto isChestFacility(const FacilityDefinition& definition) noexcept
    -> bool {
    return definition.role == FacilityRole::Chest || definition.category == FacilityCategory::Chest;
}

// Reports whether the definition needs power coverage from another facility.
[[nodiscard]] constexpr auto requiresExternalPower(const FacilityDefinition& definition) noexcept
    -> bool {
    return !ignoresPowerCoverage(definition.category) && definition.requiresPower;
}

// Reports whether a chest instance currently uses wireless storage mode.
[[nodiscard]] constexpr auto usesWirelessStorage(
    const FacilityDefinition& definition,
    const FacilityInstance& instance
) noexcept -> bool {
    return isChestFacility(definition)
           && definition.supportsWirelessStorage
           && instance.storageMode == StorageMode::Wireless;
}

// Reports whether the definition exposes inventory slots.
[[nodiscard]] constexpr auto hasInventoryCapacity(const FacilityDefinition& definition) noexcept
    -> bool {
    return definition.inventorySlotCount > 0;
}

// Reports whether any inventory slot currently contains items.
[[nodiscard]] auto hasAnyStoredItems(const FacilityInstance& instance)
    -> bool;

// Returns the default role inferred from the facility category.
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

// Returns the default power requirement inferred from the definition.
[[nodiscard]] constexpr auto defaultRequiresPower(const FacilityDefinition& definition) noexcept
    -> bool {
    return definition.category == FacilityCategory::Chest
           || definition.role == FacilityRole::Production_facility
           || definition.role == FacilityRole::Chest;
}

// Returns the default power facility type inferred from the category.
[[nodiscard]] constexpr auto defaultPowerFacilityType(FacilityCategory category) noexcept
    -> PowerFacilityType {
    switch (category) {
    case FacilityCategory::Power_pole: return PowerFacilityType::Pole;
    case FacilityCategory::Power_relay: return PowerFacilityType::Repeater;
    default: return PowerFacilityType::None;
    }
}

// Returns the input directions in the traversal order used by rules.
[[nodiscard]] auto getOrderedInputDirections(const FacilityDefinition& definition, Rotation rotation)
    -> std::vector<GridPoint>;
// Returns the output directions in the traversal order used by rules.
[[nodiscard]] auto getOrderedOutputDirections(const FacilityDefinition& definition, Rotation rotation)
    -> std::vector<GridPoint>;
// Creates the power-behavior adapter for a facility definition when applicable.
[[nodiscard]] auto createPowerFacilityInterface(const FacilityDefinition& definition)
    -> std::unique_ptr<PowerFacilityInterface>;
// Creates the production-behavior adapter for a facility definition when applicable.
[[nodiscard]] auto createProductionFacilityInterface(const FacilityDefinition& definition)
    -> std::unique_ptr<ProductionFacilityInterface>;
// Returns the default traversal capacity contributed by the definition.
[[nodiscard]] auto defaultTraversalCapacity(const FacilityDefinition& definition)
    -> double;
}  // namespace endfield_base
