#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string_view>

#include <nlohmann/json.hpp>

#include "endfield_base/json_io.h"
#include "endfield_base/path_finder.h"
#include "endfield_base/storage_system.h"

using namespace endfield_base;

namespace {
auto makeDefinition(
    std::string_view id,
    std::string_view displayName,
    FacilityCategory category,
    GridSize footprint
) -> FacilityDefinition {
    FacilityDefinition definition;
    definition.id = id;
    definition.displayName = displayName;
    definition.category = category;
    definition.role = defaultFacilityRole(category);
    definition.footprint = footprint;
    definition.powerFacilityType = defaultPowerFacilityType(category);
    definition.requiresPower = defaultRequiresPower(definition);
    return definition;
}

void writeFile(const std::filesystem::path& path, std::string_view content) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path);
    output << content;
}

FacilityCatalog createTestCatalog() {
    FacilityCatalog catalog;
    FacilityDefinition assembler = makeDefinition("assembler", "Assembler", FacilityCategory::Machine, {2, 2});
    assembler.baseThroughput = 6.0;
    assembler.productionRate = 5.0;
    assembler.consumptionRate = 3.0;
    assembler.powerUsage = 4.0;
    assembler.maxInputs = 1;
    assembler.maxOutputs = 1;
    assembler.inventorySlotCount = 2;
    assembler.itemStackLimit = 50;
    catalog.addDefinition(assembler);

    FacilityDefinition conveyor = makeDefinition("conveyor", "Conveyor", FacilityCategory::Conveyor, {1, 1});
    conveyor.baseThroughput = 8.0;
    conveyor.maxInputs = 1;
    conveyor.maxOutputs = 1;
    catalog.addDefinition(conveyor);

    FacilityDefinition bridge = makeDefinition("bridge", "Bridge", FacilityCategory::Bridge, {1, 1});
    bridge.baseThroughput = 8.0;
    bridge.maxInputs = 1;
    bridge.maxOutputs = 1;
    catalog.addDefinition(bridge);

    FacilityDefinition powerPole = makeDefinition("power_pole", "Power Pole", FacilityCategory::Power_pole, {1, 1});
    powerPole.powerRange = 3;
    catalog.addDefinition(powerPole);

    FacilityDefinition generator = makeDefinition("generator", "Generator", FacilityCategory::Generator, {2, 2});
    generator.powerGeneration = 20.0;
    catalog.addDefinition(generator);

    FacilityDefinition thermalMachine = makeDefinition(
        "thermal_machine",
        "Thermal Machine",
        FacilityCategory::Machine,
        {1, 1}
    );
    thermalMachine.role = FacilityRole::Thermal_machine;
    thermalMachine.baseThroughput = 4.0;
    thermalMachine.consumptionRate = 2.0;
    thermalMachine.powerGeneration = 12.0;
    thermalMachine.maxInputs = 1;
    thermalMachine.inventorySlotCount = 1;
    thermalMachine.itemStackLimit = 50;
    thermalMachine.ioPorts = {{{-1, 0}, PortDirection::Input}};
    thermalMachine.requiresPower = false;
    catalog.addDefinition(thermalMachine);

    FacilityDefinition storageIn = makeDefinition("storage_in", "Input", FacilityCategory::Storage_in, {1, 1});
    storageIn.baseThroughput = 10.0;
    storageIn.maxOutputs = 1;
    catalog.addDefinition(storageIn);

    FacilityDefinition storageOut = makeDefinition("storage_out", "Output", FacilityCategory::Storage_out, {1, 1});
    storageOut.baseThroughput = 10.0;
    storageOut.maxInputs = 1;
    catalog.addDefinition(storageOut);

    FacilityDefinition splitter = makeDefinition("splitter", "Splitter", FacilityCategory::Splitter, {1, 1});
    splitter.baseThroughput = 9.0;
    splitter.maxInputs = 1;
    splitter.maxOutputs = 3;
    catalog.addDefinition(splitter);

    FacilityDefinition merger = makeDefinition("merger", "Merger", FacilityCategory::Merger, {1, 1});
    merger.baseThroughput = 9.0;
    merger.maxInputs = 3;
    merger.maxOutputs = 1;
    catalog.addDefinition(merger);

    FacilityDefinition producer = makeDefinition("producer", "Producer", FacilityCategory::Machine, {1, 1});
    producer.baseThroughput = 9.0;
    producer.productionRate = 9.0;
    producer.powerUsage = 1.0;
    producer.maxOutputs = 1;
    catalog.addDefinition(producer);

    FacilityDefinition consumer = makeDefinition("consumer", "Consumer", FacilityCategory::Machine, {1, 1});
    consumer.baseThroughput = 9.0;
    consumer.consumptionRate = 9.0;
    consumer.powerUsage = 1.0;
    consumer.maxInputs = 1;
    catalog.addDefinition(consumer);

    FacilityDefinition sorter = makeDefinition("sorter", "Sorter", FacilityCategory::Machine, {1, 1});
    sorter.baseThroughput = 9.0;
    sorter.productionRate = 9.0;
    sorter.powerUsage = 1.0;
    sorter.maxOutputs = 3;
    sorter.ioPorts = {
        {{-1, 0}, PortDirection::Input},
        {{0, 1}, PortDirection::Output},
        {{1, 0}, PortDirection::Output},
        {{0, -1}, PortDirection::Output},
    };
    catalog.addDefinition(sorter);

    FacilityDefinition chest = makeDefinition("chest", "Chest", FacilityCategory::Chest, {1, 1});
    chest.role = FacilityRole::Chest;
    chest.baseThroughput = 6.0;
    chest.powerUsage = 1.0;
    chest.maxInputs = 1;
    chest.maxOutputs = 1;
    chest.inventorySlotCount = 1;
    chest.itemStackLimit = 50;
    chest.supportsWirelessStorage = true;
    chest.requiresPower = true;
    catalog.addDefinition(chest);

    FacilityDefinition hub = makeDefinition("hub", "Hub", FacilityCategory::Hub, {1, 1});
    hub.role = FacilityRole::Hub;
    hub.baseThroughput = 10.0;
    hub.maxInputs = 1;
    hub.maxOutputs = 1;
    hub.requiresPower = false;
    catalog.addDefinition(hub);

    FacilityDefinition storagePort = makeDefinition("storage_port", "Storage Port", FacilityCategory::Storage_port,
                                                    {1, 1});
    storagePort.role = FacilityRole::Storage_port;
    storagePort.baseThroughput = 10.0;
    storagePort.maxInputs = 1;
    storagePort.maxOutputs = 1;
    storagePort.occupiesBaseGrid = false;
    storagePort.canAttachToStorageLine = true;
    storagePort.requiresPower = false;
    catalog.addDefinition(storagePort);

    return catalog;
}

SimulationState createTestState() {
    SimulationState state;
    state.catalog = createTestCatalog();
    state.grid = GridMap(12, 8);
    state.facilityCatalogPath = "../facilities.json";
    return state;
}
}  // namespace

TEST_CASE("grid placement handles footprint rotation and overlap") {
    SimulationState state = createTestState();
    const FacilityDefinition* assembler = state.catalog.findDefinition("assembler");
    REQUIRE(assembler != nullptr);

    std::string reason;
    CHECK_NE(state.grid.placeFacility(*assembler, assembler->id, {0, 0}, Rotation::Deg_0, std::nullopt, &reason), 0);
    CHECK_EQ(state.grid.placeFacility(*assembler, assembler->id, {1, 1}, Rotation::Deg_90, std::nullopt, &reason), 0);
    CHECK(state.grid.canPlaceFacility(*assembler, {11, 7}, Rotation::Deg_90, &reason) == false);
}

TEST_CASE("bridge placement can coexist with conveyor on another layer") {
    SimulationState state = createTestState();
    const FacilityDefinition* conveyor = state.catalog.findDefinition("conveyor");
    const FacilityDefinition* bridge = state.catalog.findDefinition("bridge");
    REQUIRE(conveyor != nullptr);
    REQUIRE(bridge != nullptr);

    CHECK_NE(state.grid.placeFacility(*conveyor, conveyor->id, {4, 4}, Rotation::Deg_0), 0);
    CHECK_NE(state.grid.placeFacility(*bridge, bridge->id, {4, 4}, Rotation::Deg_0), 0);

    const CellOccupancy* cell = state.grid.getCell(4, 4);
    REQUIRE(cell != nullptr);
    CHECK(cell->baseInstanceId.has_value());
    CHECK(cell->bridgeInstanceId.has_value());
}

TEST_CASE("bridge lets horizontal and vertical lines cross without mixing") {
    SimulationState state = createTestState();
    const FacilityDefinition* storageIn = state.catalog.findDefinition("storage_in");
    const FacilityDefinition* storageOut = state.catalog.findDefinition("storage_out");
    const FacilityDefinition* conveyor = state.catalog.findDefinition("conveyor");
    const FacilityDefinition* bridge = state.catalog.findDefinition("bridge");

    REQUIRE(storageIn != nullptr);
    REQUIRE(storageOut != nullptr);
    REQUIRE(conveyor != nullptr);
    REQUIRE(bridge != nullptr);

    const int leftId = state.grid.placeFacility(*storageIn, storageIn->id, {2, 4}, Rotation::Deg_0);
    static_cast<void>(state.grid.placeFacility(*conveyor, conveyor->id, {3, 4}, Rotation::Deg_0));
    static_cast<void>(state.grid.placeFacility(*conveyor, conveyor->id, {4, 4}, Rotation::Deg_0));
    static_cast<void>(state.grid.placeFacility(*bridge, bridge->id, {4, 4}, Rotation::Deg_0));
    static_cast<void>(state.grid.placeFacility(*conveyor, conveyor->id, {5, 4}, Rotation::Deg_0));
    const int rightId = state.grid.placeFacility(*storageOut, storageOut->id, {6, 4}, Rotation::Deg_0);

    const int topId = state.grid.placeFacility(*storageIn, storageIn->id, {4, 2}, Rotation::Deg_0);
    static_cast<void>(state.grid.placeFacility(*conveyor, conveyor->id, {4, 3}, Rotation::Deg_0));
    static_cast<void>(state.grid.placeFacility(*conveyor, conveyor->id, {4, 5}, Rotation::Deg_0));
    const int bottomId = state.grid.placeFacility(*storageOut, storageOut->id, {4, 6}, Rotation::Deg_0);

    const PathResult horizontalPath = PathFinder::findPath(state, {leftId, rightId});
    const PathResult verticalPath = PathFinder::findPath(state, {topId, bottomId});
    const PathResult mixedPath = PathFinder::findPath(state, {leftId, bottomId});

    CHECK(horizontalPath.found);
    CHECK(verticalPath.found);
    CHECK(!mixedPath.found);
}

TEST_CASE("storage port must attach to perimeter and does not block edge conveyor") {
    SimulationState state = createTestState();
    const FacilityDefinition* storagePort = state.catalog.findDefinition("storage_port");
    const FacilityDefinition* conveyor = state.catalog.findDefinition("conveyor");
    REQUIRE(storagePort != nullptr);
    REQUIRE(conveyor != nullptr);

    std::string reason;
    CHECK(!state.grid.canPlaceFacility(*storagePort, {4, 4}, Rotation::Deg_0, &reason));
    CHECK(reason == "storage port must attach to perimeter line");

    CHECK_NE(state.grid.placeFacility(*conveyor, conveyor->id, {0, 3}, Rotation::Deg_0), 0);
    CHECK_NE(state.grid.placeFacility(*storagePort, storagePort->id, {0, 3}, Rotation::Deg_0), 0);

    const CellOccupancy* cell = state.grid.getCell(0, 3);
    REQUIRE(cell != nullptr);
    CHECK(cell->baseInstanceId.has_value());
    CHECK_EQ(cell->attachedInstanceIds.size(), 1);
}

TEST_CASE("power evaluation respects generator and coverage rules") {
    SimulationState state = createTestState();
    const FacilityDefinition* generator = state.catalog.findDefinition("generator");
    const FacilityDefinition* pole = state.catalog.findDefinition("power_pole");
    const FacilityDefinition* assembler = state.catalog.findDefinition("assembler");
    const FacilityDefinition* conveyor = state.catalog.findDefinition("conveyor");

    REQUIRE(generator != nullptr);
    REQUIRE(pole != nullptr);
    REQUIRE(assembler != nullptr);
    REQUIRE(conveyor != nullptr);

    CHECK_NE(state.grid.placeFacility(*generator, generator->id, {0, 0}, Rotation::Deg_0), 0);
    CHECK_NE(state.grid.placeFacility(*pole, pole->id, {3, 1}, Rotation::Deg_0), 0);
    CHECK_NE(state.grid.placeFacility(*pole, pole->id, {3, 4}, Rotation::Deg_0), 0);
    CHECK_NE(state.grid.placeFacility(*pole, pole->id, {8, 3}, Rotation::Deg_0), 0);
    const int assemblerId = state.grid.placeFacility(*assembler, assembler->id, {4, 1}, Rotation::Deg_0);
    const int conveyorId = state.grid.placeFacility(*conveyor, conveyor->id, {9, 7}, Rotation::Deg_0);

    const PowerReport report = PowerSystem::evaluatePower(state);
    const auto assemblerState = std::find_if(
        report.facilityStates.begin(),
        report.facilityStates.end(),
        [assemblerId] (const FacilityPowerState& facilityState) { return facilityState.instanceId == assemblerId; }
    );
    const auto conveyorState = std::find_if(
        report.facilityStates.begin(),
        report.facilityStates.end(),
        [conveyorId] (const FacilityPowerState& facilityState) { return facilityState.instanceId == conveyorId; }
    );

    REQUIRE(assemblerState != report.facilityStates.end());
    REQUIRE(conveyorState != report.facilityStates.end());
    CHECK(assemblerState->covered);
    CHECK(assemblerState->powered);
    CHECK(conveyorState->powered);
}

TEST_CASE("thermal machine generates power only when fuel is stored") {
    SimulationState state = createTestState();
    const FacilityDefinition* thermalMachine = state.catalog.findDefinition("thermal_machine");
    REQUIRE(thermalMachine != nullptr);

    const int coldId = state.grid.placeFacility(*thermalMachine, thermalMachine->id, {1, 1}, Rotation::Deg_0);
    const int fueledId = state.grid.placeFacility(*thermalMachine, thermalMachine->id, {4, 1}, Rotation::Deg_0);
    FacilityInstance* fueledMachine = state.grid.findFacility(fueledId);
    REQUIRE(fueledMachine != nullptr);
    fueledMachine->inventorySlots = {{"coal", 20}};

    const PowerReport report = PowerSystem::evaluatePower(state);
    const auto coldState = std::find_if(
        report.facilityStates.begin(),
        report.facilityStates.end(),
        [coldId] (const FacilityPowerState& facilityState) { return facilityState.instanceId == coldId; }
    );
    const auto fueledState = std::find_if(
        report.facilityStates.begin(),
        report.facilityStates.end(),
        [fueledId] (const FacilityPowerState& facilityState) { return facilityState.instanceId == fueledId; }
    );

    REQUIRE(coldState != report.facilityStates.end());
    REQUIRE(fueledState != report.facilityStates.end());
    CHECK(coldState->powered);
    CHECK(fueledState->powered);
    CHECK_EQ(report.totalGeneration, thermalMachine->powerGeneration);
}

TEST_CASE("throughput evaluation accounts for logistics path length and bottlenecks") {
    SimulationState state = createTestState();
    const FacilityDefinition* generator = state.catalog.findDefinition("generator");
    const FacilityDefinition* pole = state.catalog.findDefinition("power_pole");
    const FacilityDefinition* storageIn = state.catalog.findDefinition("storage_in");
    const FacilityDefinition* storageOut = state.catalog.findDefinition("storage_out");
    const FacilityDefinition* assembler = state.catalog.findDefinition("assembler");
    const FacilityDefinition* conveyor = state.catalog.findDefinition("conveyor");

    REQUIRE(generator != nullptr);
    REQUIRE(pole != nullptr);
    REQUIRE(storageIn != nullptr);
    REQUIRE(storageOut != nullptr);
    REQUIRE(assembler != nullptr);
    REQUIRE(conveyor != nullptr);

    CHECK_NE(state.grid.placeFacility(*generator, generator->id, {0, 0}, Rotation::Deg_0), 0);
    CHECK_NE(state.grid.placeFacility(*pole, pole->id, {3, 1}, Rotation::Deg_0), 0);
    CHECK_NE(state.grid.placeFacility(*pole, pole->id, {3, 4}, Rotation::Deg_0), 0);
    CHECK_NE(state.grid.placeFacility(*pole, pole->id, {10, 0}, Rotation::Deg_0), 0);
    CHECK_NE(state.grid.placeFacility(*storageIn, storageIn->id, {4, 4}, Rotation::Deg_0), 0);
    const int assemblerId = state.grid.placeFacility(*assembler, assembler->id, {5, 2}, Rotation::Deg_0);
    CHECK_NE(assemblerId, 0);
    CHECK_NE(state.grid.placeFacility(*storageOut, storageOut->id, {10, 3}, Rotation::Deg_0), 0);

    CHECK_NE(state.grid.placeFacility(*conveyor, conveyor->id, {4, 3}, Rotation::Deg_0), 0);
    for (int x = 7; x <= 9; ++x) {
        CHECK_NE(state.grid.placeFacility(*conveyor, conveyor->id, {x, 3}, Rotation::Deg_0), 0);
    }

    const ThroughputReport report = ThroughputEvaluator::evaluateThroughput(state);
    const auto facilityResult = std::find_if(
        report.facilityResults.begin(),
        report.facilityResults.end(),
        [assemblerId] (const FacilityThroughputResult& result) { return result.instanceId == assemblerId; }
    );

    REQUIRE(facilityResult != report.facilityResults.end());
    CHECK(facilityResult->throughputPerSecond > 0.0);
    CHECK(facilityResult->pathLengthFromInput >= 0);
    CHECK(facilityResult->pathLengthToOutput >= 0);
    CHECK(!report.networkResults.empty());
}

TEST_CASE("production facility shares throughput across connected outputs like splitter") {
    SimulationState state = createTestState();
    const FacilityDefinition* generator = state.catalog.findDefinition("generator");
    const FacilityDefinition* pole = state.catalog.findDefinition("power_pole");
    const FacilityDefinition* sorter = state.catalog.findDefinition("sorter");
    const FacilityDefinition* conveyor = state.catalog.findDefinition("conveyor");
    const FacilityDefinition* storageOut = state.catalog.findDefinition("storage_out");

    REQUIRE(generator != nullptr);
    REQUIRE(pole != nullptr);
    REQUIRE(sorter != nullptr);
    REQUIRE(conveyor != nullptr);
    REQUIRE(storageOut != nullptr);

    CHECK_NE(state.grid.placeFacility(*generator, generator->id, {0, 0}, Rotation::Deg_0), 0);
    CHECK_NE(state.grid.placeFacility(*pole, pole->id, {4, 2}, Rotation::Deg_0), 0);
    CHECK_NE(state.grid.placeFacility(*sorter, sorter->id, {4, 4}, Rotation::Deg_0), 0);
    CHECK_NE(state.grid.placeFacility(*conveyor, conveyor->id, {5, 4}, Rotation::Deg_0), 0);
    CHECK_NE(state.grid.placeFacility(*storageOut, storageOut->id, {6, 4}, Rotation::Deg_0), 0);

    const ThroughputReport oneOutputReport = ThroughputEvaluator::evaluateThroughput(state);

    CHECK_NE(state.grid.placeFacility(*conveyor, conveyor->id, {4, 5}, Rotation::Deg_0), 0);
    const ThroughputReport twoOutputReport = ThroughputEvaluator::evaluateThroughput(state);

    const auto findSorterResult = [] (const ThroughputReport& report) {
        const auto it = std::find_if(
            report.facilityResults.begin(),
            report.facilityResults.end(),
            [] (const FacilityThroughputResult& result) { return result.definitionId == "sorter"; }
        );
        return it == report.facilityResults.end() ? 0.0 : it->throughputPerSecond;
    };

    CHECK(findSorterResult(oneOutputReport) > findSorterResult(twoOutputReport));
}

TEST_CASE("splitter averages throughput across connected outputs") {
    const auto buildState = [] (int outputBranchCount) {
        SimulationState state = createTestState();
        const FacilityDefinition* generator = state.catalog.findDefinition("generator");
        const FacilityDefinition* pole = state.catalog.findDefinition("power_pole");
        const FacilityDefinition* producer = state.catalog.findDefinition("producer");
        const FacilityDefinition* splitter = state.catalog.findDefinition("splitter");
        const FacilityDefinition* conveyor = state.catalog.findDefinition("conveyor");
        const FacilityDefinition* storageOut = state.catalog.findDefinition("storage_out");

        static_cast<void>(state.grid.placeFacility(*generator, generator->id, {0, 0}, Rotation::Deg_0));
        static_cast<void>(state.grid.placeFacility(*pole, pole->id, {2, 3}, Rotation::Deg_0));
        static_cast<void>(state.grid.placeFacility(*pole, pole->id, {7, 3}, Rotation::Deg_0));
        static_cast<void>(state.grid.placeFacility(*producer, producer->id, {1, 4}, Rotation::Deg_0));
        static_cast<void>(state.grid.placeFacility(*conveyor, conveyor->id, {2, 4}, Rotation::Deg_0));
        static_cast<void>(state.grid.placeFacility(*conveyor, conveyor->id, {3, 4}, Rotation::Deg_0));
        static_cast<void>(state.grid.placeFacility(*splitter, splitter->id, {4, 4}, Rotation::Deg_0));
        static_cast<void>(state.grid.placeFacility(*conveyor, conveyor->id, {5, 4}, Rotation::Deg_0));
        static_cast<void>(state.grid.placeFacility(*conveyor, conveyor->id, {6, 4}, Rotation::Deg_0));
        static_cast<void>(state.grid.placeFacility(*conveyor, conveyor->id, {7, 4}, Rotation::Deg_0));
        static_cast<void>(state.grid.placeFacility(*storageOut, storageOut->id, {8, 4}, Rotation::Deg_0));

        if (outputBranchCount >= 2) {
            static_cast<void>(state.grid.placeFacility(*conveyor, conveyor->id, {4, 3}, Rotation::Deg_0));
        }
        if (outputBranchCount >= 3) {
            static_cast<void>(state.grid.placeFacility(*conveyor, conveyor->id, {4, 5}, Rotation::Deg_0));
        }

        return state;
    };

    const ThroughputReport oneBranchReport = ThroughputEvaluator::evaluateThroughput(buildState(1));
    const ThroughputReport twoBranchReport = ThroughputEvaluator::evaluateThroughput(buildState(2));
    const ThroughputReport threeBranchReport = ThroughputEvaluator::evaluateThroughput(buildState(3));

    const auto findProducerThroughput = [] (const ThroughputReport& report) {
        const auto it = std::find_if(
            report.facilityResults.begin(),
            report.facilityResults.end(),
            [] (const FacilityThroughputResult& result) { return result.definitionId == "producer"; }
        );
        return it == report.facilityResults.end() ? 0.0 : it->throughputPerSecond;
    };

    const double oneBranchThroughput = findProducerThroughput(oneBranchReport);
    const double twoBranchThroughput = findProducerThroughput(twoBranchReport);
    const double threeBranchThroughput = findProducerThroughput(threeBranchReport);

    CHECK(oneBranchThroughput > 0.0);
    CHECK(twoBranchThroughput > 0.0);
    CHECK(threeBranchThroughput > 0.0);
    CHECK(twoBranchThroughput < oneBranchThroughput);
    CHECK(threeBranchThroughput < twoBranchThroughput);
    CHECK(threeBranchThroughput < oneBranchThroughput);
}

TEST_CASE("merger reduces per-input throughput when more inputs are connected") {
    const auto buildState = [] (bool threeBranches) {
        SimulationState state = createTestState();
        const FacilityDefinition* generator = state.catalog.findDefinition("generator");
        const FacilityDefinition* pole = state.catalog.findDefinition("power_pole");
        const FacilityDefinition* consumer = state.catalog.findDefinition("consumer");
        const FacilityDefinition* merger = state.catalog.findDefinition("merger");
        const FacilityDefinition* conveyor = state.catalog.findDefinition("conveyor");
        const FacilityDefinition* storageIn = state.catalog.findDefinition("storage_in");

        static_cast<void>(state.grid.placeFacility(*generator, generator->id, {0, 0}, Rotation::Deg_0));
        static_cast<void>(state.grid.placeFacility(*pole, pole->id, {2, 3}, Rotation::Deg_0));
        static_cast<void>(state.grid.placeFacility(*pole, pole->id, {7, 3}, Rotation::Deg_0));
        static_cast<void>(state.grid.placeFacility(*storageIn, storageIn->id, {1, 4}, Rotation::Deg_0));
        static_cast<void>(state.grid.placeFacility(*conveyor, conveyor->id, {2, 4}, Rotation::Deg_0));
        static_cast<void>(state.grid.placeFacility(*conveyor, conveyor->id, {3, 4}, Rotation::Deg_0));
        static_cast<void>(state.grid.placeFacility(*conveyor, conveyor->id, {4, 4}, Rotation::Deg_0));
        static_cast<void>(state.grid.placeFacility(*merger, merger->id, {5, 4}, Rotation::Deg_0));
        static_cast<void>(state.grid.placeFacility(*conveyor, conveyor->id, {6, 4}, Rotation::Deg_0));
        static_cast<void>(state.grid.placeFacility(*conveyor, conveyor->id, {7, 4}, Rotation::Deg_0));
        static_cast<void>(state.grid.placeFacility(*consumer, consumer->id, {8, 4}, Rotation::Deg_0));

        if (threeBranches) {
            static_cast<void>(state.grid.placeFacility(*conveyor, conveyor->id, {5, 3}, Rotation::Deg_0));
            static_cast<void>(state.grid.placeFacility(*conveyor, conveyor->id, {5, 5}, Rotation::Deg_0));
        }

        return state;
    };

    const ThroughputReport oneBranchReport = ThroughputEvaluator::evaluateThroughput(buildState(false));
    const ThroughputReport threeBranchReport = ThroughputEvaluator::evaluateThroughput(buildState(true));

    const auto findConsumerThroughput = [] (const ThroughputReport& report) {
        const auto it = std::find_if(
            report.facilityResults.begin(),
            report.facilityResults.end(),
            [] (const FacilityThroughputResult& result) { return result.definitionId == "consumer"; }
        );
        return it == report.facilityResults.end() ? 0.0 : it->throughputPerSecond;
    };

    const double oneBranchThroughput = findConsumerThroughput(oneBranchReport);
    const double threeBranchThroughput = findConsumerThroughput(threeBranchReport);

    CHECK(oneBranchThroughput > 0.0);
    CHECK(threeBranchThroughput > 0.0);
    CHECK(threeBranchThroughput < oneBranchThroughput);
}

TEST_CASE("wireless chest is treated as a warehouse output endpoint") {
    SimulationState state = createTestState();
    const FacilityDefinition* generator = state.catalog.findDefinition("generator");
    const FacilityDefinition* pole = state.catalog.findDefinition("power_pole");
    const FacilityDefinition* producer = state.catalog.findDefinition("producer");
    const FacilityDefinition* conveyor = state.catalog.findDefinition("conveyor");
    const FacilityDefinition* chest = state.catalog.findDefinition("chest");

    REQUIRE(generator != nullptr);
    REQUIRE(pole != nullptr);
    REQUIRE(producer != nullptr);
    REQUIRE(conveyor != nullptr);
    REQUIRE(chest != nullptr);

    CHECK_NE(state.grid.placeFacility(*generator, generator->id, {0, 0}, Rotation::Deg_0), 0);
    CHECK_NE(state.grid.placeFacility(*pole, pole->id, {3, 3}, Rotation::Deg_0), 0);
    CHECK_NE(state.grid.placeFacility(*producer, producer->id, {1, 4}, Rotation::Deg_0), 0);
    CHECK_NE(state.grid.placeFacility(*conveyor, conveyor->id, {2, 4}, Rotation::Deg_0), 0);
    const int chestId = state.grid.placeFacility(*chest, chest->id, {3, 4}, Rotation::Deg_0);
    REQUIRE(chestId != 0);

    FacilityInstance* chestInstance = state.grid.findFacility(chestId);
    REQUIRE(chestInstance != nullptr);
    chestInstance->storageMode = StorageMode::Wireless;
    chestInstance->inventorySlots = {{"ingot", 15}};

    const auto [inputInstanceIds, outputInstanceIds] = StorageSystem::collectEndpoints(state);
    CHECK(std::find(outputInstanceIds.begin(), outputInstanceIds.end(), chestId)
        != outputInstanceIds.end());

    const ThroughputReport report = ThroughputEvaluator::evaluateThroughput(state);
    const auto chestResult = std::find_if(
        report.facilityResults.begin(),
        report.facilityResults.end(),
        [chestId] (const FacilityThroughputResult& result) { return result.instanceId == chestId; }
    );

    REQUIRE(chestResult != report.facilityResults.end());
    CHECK(chestResult->throughputPerSecond > 0.0);
    CHECK(chestResult->bottleneckReason == "wireless_storage");
}

TEST_CASE("hub acts as an in-base warehouse endpoint without power") {
    SimulationState state = createTestState();
    const FacilityDefinition* hub = state.catalog.findDefinition("hub");
    REQUIRE(hub != nullptr);

    const int hubId = state.grid.placeFacility(*hub, hub->id, {5, 5}, Rotation::Deg_0);
    REQUIRE(hubId != 0);

    const auto [inputInstanceIds, outputInstanceIds] = StorageSystem::collectEndpoints(state);
    CHECK(std::find(inputInstanceIds.begin(), inputInstanceIds.end(), hubId)
        != inputInstanceIds.end());
    CHECK(std::find(outputInstanceIds.begin(), outputInstanceIds.end(), hubId)
        != outputInstanceIds.end());

    const PowerReport report = PowerSystem::evaluatePower(state);
    const auto hubState = std::find_if(
        report.facilityStates.begin(),
        report.facilityStates.end(),
        [hubId] (const FacilityPowerState& facilityState) { return facilityState.instanceId == hubId; }
    );

    REQUIRE(hubState != report.facilityStates.end());
    CHECK(hubState->powered);
}

TEST_CASE("json load save and result export remain consistent") {
    const std::filesystem::path tempRoot = std::filesystem::temp_directory_path() / "endfield_base_tests";
    const std::filesystem::path catalogPath = tempRoot / "facilities.json";
    const std::filesystem::path mapPath = tempRoot / "maps" / "sample_map.json";
    const std::filesystem::path savedPath = tempRoot / "maps" / "saved_map.json";
    const std::filesystem::path resultPath = tempRoot / "results" / "sample_result.json";

    writeFile(
        catalogPath,
        R"({
  "facilities": [
    {"id":"assembler","displayName":"Assembler","category":"machine","footprint":{"width":2,"height":2},"baseThroughput":6.0,"productionRate":5.0,"consumptionRate":3.0,"powerUsage":4.0,"powerGeneration":0.0,"powerRange":0,"maxInputs":1,"maxOutputs":1},
    {"id":"conveyor","displayName":"Conveyor","category":"conveyor","footprint":{"width":1,"height":1},"baseThroughput":8.0,"productionRate":0.0,"consumptionRate":0.0,"powerUsage":0.0,"powerGeneration":0.0,"powerRange":0,"maxInputs":1,"maxOutputs":1},
    {"id":"power_pole","displayName":"Power Pole","category":"power_pole","footprint":{"width":1,"height":1},"baseThroughput":0.0,"productionRate":0.0,"consumptionRate":0.0,"powerUsage":0.0,"powerGeneration":0.0,"powerRange":3,"maxInputs":0,"maxOutputs":0},
    {"id":"generator","displayName":"Generator","category":"generator","footprint":{"width":2,"height":2},"baseThroughput":0.0,"productionRate":0.0,"consumptionRate":0.0,"powerUsage":0.0,"powerGeneration":20.0,"powerRange":0,"maxInputs":0,"maxOutputs":0},
    {"id":"storage_in","displayName":"Input","category":"storage_in","footprint":{"width":1,"height":1},"baseThroughput":10.0,"productionRate":0.0,"consumptionRate":0.0,"powerUsage":0.0,"powerGeneration":0.0,"powerRange":0,"maxInputs":0,"maxOutputs":1},
    {"id":"storage_out","displayName":"Output","category":"storage_out","footprint":{"width":1,"height":1},"baseThroughput":10.0,"productionRate":0.0,"consumptionRate":0.0,"powerUsage":0.0,"powerGeneration":0.0,"powerRange":0,"maxInputs":1,"maxOutputs":0}
  ]
})"
    );

    writeFile(
        mapPath,
        R"({
  "facilityCatalog": "../facilities.json",
  "width": 12,
  "height": 8,
  "nextInstanceId": 6,
  "facilities": [
    {"instanceId": 1, "definitionId": "generator", "position": {"x": 0, "y": 0}, "rotation": "deg0"},
    {"instanceId": 2, "definitionId": "power_pole", "position": {"x": 3, "y": 1}, "rotation": "deg0"},
    {"instanceId": 3, "definitionId": "storage_in", "position": {"x": 4, "y": 4}, "rotation": "deg0"},
    {"instanceId": 4, "definitionId": "assembler", "position": {"x": 5, "y": 2}, "rotation": "deg0"},
    {"instanceId": 5, "definitionId": "storage_out", "position": {"x": 10, "y": 3}, "rotation": "deg0"}
  ]
})"
    );

    SimulationState state = loadMap(mapPath);
    saveMap(savedPath, state);

    const ResultReport report = buildResultReport(state);
    std::filesystem::create_directories(resultPath.parent_path());
    exportResultReport(resultPath, report);

    CHECK(std::filesystem::exists(savedPath));
    CHECK(std::filesystem::exists(resultPath));

    const auto resultJson = nlohmann::json::parse(std::ifstream(resultPath));
    CHECK(resultJson.contains("summary"));
    CHECK(resultJson.contains("facilityResults"));
    CHECK(resultJson.contains("powerStates"));
    CHECK(resultJson.contains("networkResults"));
}
