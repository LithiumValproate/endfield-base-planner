#include "endfield_base/facility.h"

#include <algorithm>
#include <array>
#include <ranges>
#include <stdexcept>

namespace endfield_base {
namespace {
    class PowerPoleFacility final : public PowerFacilityInterface {
    public:
        [[nodiscard]] auto getPowerRange(const FacilityDefinition& definition
        ) const -> int override {
            return definition.powerRange;
        }
    };

    class PowerRelayFacility final : public PowerFacilityInterface {
    public:
        [[nodiscard]] auto getPowerRange(const FacilityDefinition& definition
        ) const -> int override {
            return definition.powerRange;
        }
    };

    class GenericProductionFacility final : public ProductionFacilityInterface {
    public:
        [[nodiscard]] auto getInputDirections(
            const FacilityDefinition& definition,
            Rotation rotation
        ) const -> std::vector<GridPoint> override {
            return getOrderedInputDirections(definition, rotation);
        }

        [[nodiscard]] auto getOutputDirections(
            const FacilityDefinition& definition,
            Rotation rotation
        ) const -> std::vector<GridPoint> override {
            return getOrderedOutputDirections(definition, rotation);
        }
    };

    constexpr std::array<GridPoint, 3> Splitter_Output_Order{
        {
            {0, 1},
            {1, 0},
            {0, -1},
        }
    };

    constexpr std::array<GridPoint, 1> Splitter_Input_Order{
        {
            {-1, 0},
        }
    };

    std::vector<GridPoint> filterPortsByDirection(const FacilityDefinition& definition,
                                                  PortDirection filterDirection,
                                                  Rotation rotation) {
        std::vector<GridPoint> directions;
        for (const auto& [direction, portDirection] : definition.ioPorts) {
            if (portDirection != filterDirection) {
                continue;
            }

            directions.push_back(rotateDirection(direction, rotation));
        }
        return directions;
    }
}  // namespace

void FacilityCatalog::addDefinition(FacilityDefinition definition) {
    if (definition.role == FacilityRole::Generic) {
        definition.role = defaultFacilityRole(definition.category);
    }

    if (definition.powerFacilityType == PowerFacilityType::None) {
        definition.powerFacilityType = defaultPowerFacilityType(definition.category);
    }

    if (definition.itemStackLimit <= 0 && definition.inventorySlotCount > 0) {
        definition.itemStackLimit = 50;
    }

    indexById_[definition.id] = definitions_.size();
    definitions_.push_back(std::move(definition));
}

const FacilityDefinition* FacilityCatalog::findDefinition(std::string_view id) const {
    const auto it = indexById_.find(id);
    if (it == indexById_.end()) {
        return nullptr;
    }

    return &definitions_.at(it->second);
}

const std::vector<FacilityDefinition>& FacilityCatalog::getDefinitions() const {
    return definitions_;
}

bool FacilityCatalog::empty() const {
    return definitions_.empty();
}

std::string toString(FacilityCategory category) {
    switch (category) {
    case FacilityCategory::Machine: return "machine";
    case FacilityCategory::Conveyor: return "conveyor";
    case FacilityCategory::Splitter: return "splitter";
    case FacilityCategory::Merger: return "merger";
    case FacilityCategory::Bridge: return "bridge";
    case FacilityCategory::Power_pole: return "power_pole";
    case FacilityCategory::Power_relay: return "power_relay";
    case FacilityCategory::Generator: return "generator";
    case FacilityCategory::Chest: return "chest";
    case FacilityCategory::Hub: return "hub";
    case FacilityCategory::Storage_port: return "storage_port";
    case FacilityCategory::Servo: return "servo";
    case FacilityCategory::Storage_in: return "storage_in";
    case FacilityCategory::Storage_out: return "storage_out";
    }

    throw std::runtime_error("Unknown facility category");
}

FacilityCategory facilityCategoryFromString(std::string_view value) {
    if (value == "machine") {
        return FacilityCategory::Machine;
    }
    if (value == "conveyor") {
        return FacilityCategory::Conveyor;
    }
    if (value == "splitter") {
        return FacilityCategory::Splitter;
    }
    if (value == "merger") {
        return FacilityCategory::Merger;
    }
    if (value == "bridge") {
        return FacilityCategory::Bridge;
    }
    if (value == "power_pole") {
        return FacilityCategory::Power_pole;
    }
    if (value == "power_relay") {
        return FacilityCategory::Power_relay;
    }
    if (value == "generator") {
        return FacilityCategory::Generator;
    }
    if (value == "chest") {
        return FacilityCategory::Chest;
    }
    if (value == "hub") {
        return FacilityCategory::Hub;
    }
    if (value == "storage_port") {
        return FacilityCategory::Storage_port;
    }
    if (value == "servo") {
        return FacilityCategory::Servo;
    }
    if (value == "storage_in") {
        return FacilityCategory::Storage_in;
    }
    if (value == "storage_out") {
        return FacilityCategory::Storage_out;
    }

    throw std::runtime_error("Unsupported facility category string: " + std::string(value));
}

std::string toString(FacilityRole role) {
    switch (role) {
    case FacilityRole::Generic: return "generic";
    case FacilityRole::Thermal_machine: return "thermal_machine";
    case FacilityRole::Power_facility: return "power_facility";
    case FacilityRole::Production_facility: return "production_facility";
    case FacilityRole::Chest: return "chest";
    case FacilityRole::Hub: return "hub";
    case FacilityRole::Storage_port: return "storage_port";
    case FacilityRole::Servo: return "servo";
    }

    throw std::runtime_error("Unknown facility role");
}

FacilityRole facilityRoleFromString(std::string_view value) {
    if (value == "generic") {
        return FacilityRole::Generic;
    }
    if (value == "thermal_machine") {
        return FacilityRole::Thermal_machine;
    }
    if (value == "power_facility") {
        return FacilityRole::Power_facility;
    }
    if (value == "production_facility") {
        return FacilityRole::Production_facility;
    }
    if (value == "chest") {
        return FacilityRole::Chest;
    }
    if (value == "hub") {
        return FacilityRole::Hub;
    }
    if (value == "storage_port") {
        return FacilityRole::Storage_port;
    }
    if (value == "servo") {
        return FacilityRole::Servo;
    }

    throw std::runtime_error("Unsupported facility role string: " + std::string(value));
}

std::string toString(Rotation rotation) {
    switch (rotation) {
    case Rotation::Deg_0: return "deg0";
    case Rotation::Deg_90: return "deg90";
    case Rotation::Deg_180: return "deg180";
    case Rotation::Deg_270: return "deg270";
    }

    throw std::runtime_error("Unknown rotation");
}

Rotation rotationFromString(std::string_view value) {
    if (value == "deg0") {
        return Rotation::Deg_0;
    }
    if (value == "deg90") {
        return Rotation::Deg_90;
    }
    if (value == "deg180") {
        return Rotation::Deg_180;
    }
    if (value == "deg270") {
        return Rotation::Deg_270;
    }

    throw std::runtime_error("Unsupported rotation string: " + std::string(value));
}

std::string toString(PortDirection portDirection) {
    switch (portDirection) {
    case PortDirection::Input: return "input";
    case PortDirection::Output: return "output";
    }

    throw std::runtime_error("Unknown port direction");
}

PortDirection portDirectionFromString(std::string_view value) {
    if (value == "input") {
        return PortDirection::Input;
    }
    if (value == "output") {
        return PortDirection::Output;
    }

    throw std::runtime_error("Unsupported port direction string: " + std::string(value));
}

std::string toString(StorageMode storageMode) {
    switch (storageMode) {
    case StorageMode::Conveyor: return "conveyor";
    case StorageMode::Wireless: return "wireless";
    }

    throw std::runtime_error("Unknown storage mode");
}

StorageMode storageModeFromString(std::string_view value) {
    if (value == "conveyor") {
        return StorageMode::Conveyor;
    }
    if (value == "wireless") {
        return StorageMode::Wireless;
    }

    throw std::runtime_error("Unsupported storage mode string: " + std::string(value));
}

std::string toString(PowerFacilityType powerFacilityType) {
    switch (powerFacilityType) {
    case PowerFacilityType::None: return "none";
    case PowerFacilityType::Pole: return "pole";
    case PowerFacilityType::Repeater: return "repeater";
    }

    throw std::runtime_error("Unknown power facility type");
}

PowerFacilityType powerFacilityTypeFromString(std::string_view value) {
    if (value == "none") {
        return PowerFacilityType::None;
    }
    if (value == "pole") {
        return PowerFacilityType::Pole;
    }
    if (value == "repeater") {
        return PowerFacilityType::Repeater;
    }

    throw std::runtime_error("Unsupported power facility type string: " + std::string(value));
}

bool hasAnyStoredItems(const FacilityInstance& instance) {
    return std::ranges::any_of(instance.inventorySlots, [] (const InventorySlot& inventorySlot) {
        return inventorySlot.count > 0;
    });
}

std::vector<GridPoint> getOrderedInputDirections(const FacilityDefinition& definition, Rotation rotation) {
    std::vector<GridPoint> directions = filterPortsByDirection(definition, PortDirection::Input, rotation);
    if (!directions.empty()) {
        return directions;
    }

    if (definition.category == FacilityCategory::Splitter) {
        directions.assign(Splitter_Input_Order.begin(), Splitter_Input_Order.end());
        for (GridPoint& direction : directions) {
            direction = rotateDirection(direction, rotation);
        }
        return directions;
    }

    if (definition.maxInputs > 0) {
        return {rotateDirection({-1, 0}, rotation)};
    }

    return {};
}

std::vector<GridPoint> getOrderedOutputDirections(const FacilityDefinition& definition, Rotation rotation) {
    std::vector<GridPoint> directions = filterPortsByDirection(definition, PortDirection::Output, rotation);
    if (!directions.empty()) {
        return directions;
    }

    if (definition.category == FacilityCategory::Splitter || definition.maxOutputs > 1) {
        directions.assign(Splitter_Output_Order.begin(), Splitter_Output_Order.end());
        for (GridPoint& direction : directions) {
            direction = rotateDirection(direction, rotation);
        }
        return directions;
    }

    if (definition.maxOutputs > 0) {
        return {rotateDirection({1, 0}, rotation)};
    }

    return {};
}

std::unique_ptr<PowerFacilityInterface> createPowerFacilityInterface(const FacilityDefinition& definition) {
    switch (definition.powerFacilityType) {
    case PowerFacilityType::Pole: return std::make_unique<PowerPoleFacility>();
    case PowerFacilityType::Repeater: return std::make_unique<PowerRelayFacility>();
    case PowerFacilityType::None: return nullptr;
    }

    return nullptr;
}

std::unique_ptr<ProductionFacilityInterface> createProductionFacilityInterface(const FacilityDefinition& definition) {
    if (!isProductionFacility(definition)) {
        return nullptr;
    }

    return std::make_unique<GenericProductionFacility>();
}

double defaultTraversalCapacity(const FacilityDefinition& definition) {
    return std::max({
        definition.baseThroughput,
        definition.productionRate,
        definition.consumptionRate,
        1.0,
    });
}
}  // namespace endfield_base
