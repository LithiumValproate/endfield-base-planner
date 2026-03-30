#include <algorithm>
#include <filesystem>
#include <optional>
#include <ranges>
#include <sstream>
#include <stdexcept>

#include <raylib.h>

#include "endfield_base/json_io.h"

using namespace endfield_base;

namespace {
constexpr int Window_Width = 1400;
constexpr int Window_Height = 900;
constexpr int Cell_Size = 40;
constexpr int Sidebar_Width = 360;

Color facilityColor(FacilityCategory category) {
    switch (category) {
    case FacilityCategory::Machine: return ORANGE;
    case FacilityCategory::Conveyor: return SKYBLUE;
    case FacilityCategory::Splitter: return BLUE;
    case FacilityCategory::Merger: return DARKBLUE;
    case FacilityCategory::Bridge: return VIOLET;
    case FacilityCategory::Power_pole: return YELLOW;
    case FacilityCategory::Generator: return GREEN;
    case FacilityCategory::Storage_in: return LIME;
    case FacilityCategory::Storage_out: return RED;
    }

    return LIGHTGRAY;
}

GridPoint screenToGrid(Vector2 position) {
    return {
        static_cast<int>(position.x) / Cell_Size,
        static_cast<int>(position.y) / Cell_Size,
    };
}

std::optional<int> facilityAtPoint(const SimulationState& state, const GridPoint& point) {
    const std::vector<int> ids = state.grid.getFacilityIdsAt(point);
    if (ids.empty()) {
        return std::nullopt;
    }
    return ids.back();
}

Rotation nextRotation(Rotation rotation) {
    switch (rotation) {
    case Rotation::Deg_0: return Rotation::Deg_90;
    case Rotation::Deg_90: return Rotation::Deg_180;
    case Rotation::Deg_180: return Rotation::Deg_270;
    case Rotation::Deg_270: return Rotation::Deg_0;
    }

    return Rotation::Deg_0;
}

void drawFacility(const SimulationState& state, const FacilityInstance& instance, bool selected) {
    const FacilityDefinition* definition = state.catalog.findDefinition(instance.definitionId);
    if (definition == nullptr) {
        return;
    }

    const GridSize footprint = rotateFootprint(definition->footprint, instance.rotation);
    const Rectangle rectangle{
        static_cast<float>(instance.position.x * Cell_Size),
        static_cast<float>(instance.position.y * Cell_Size),
        static_cast<float>(footprint.width * Cell_Size),
        static_cast<float>(footprint.height * Cell_Size),
    };
    DrawRectangleRec(rectangle, facilityColor(definition->category));
    DrawRectangleLinesEx(rectangle, selected ? 4.0F : 2.0F, selected ? BLACK : DARKGRAY);
    DrawText(
        definition->displayName.c_str(),
        static_cast<int>(rectangle.x) + 4,
        static_cast<int>(rectangle.y) + 4,
        14,
        BLACK
    );
}

void drawPowerCoverage(const SimulationState& state, int selectedInstanceId) {
    const FacilityInstance* instance = state.grid.findFacility(selectedInstanceId);
    if (instance == nullptr) {
        return;
    }

    const FacilityDefinition* definition = state.catalog.findDefinition(instance->definitionId);
    if (definition == nullptr || !isPowerTransmitter(definition->category)) {
        return;
    }

    const auto occupiedCells = state.grid.getOccupiedCells(*definition, *instance);
    for (const GridPoint& cell : occupiedCells) {
        for (int y = std::max(0, cell.y - definition->powerRange); y <= std::min(state.grid.getHeight() - 1,
                                                                       cell.y + definition->powerRange); ++y) {
            for (int x = std::max(0, cell.x - definition->powerRange); x <= std::min(state.grid.getWidth() - 1,
                                                                           cell.x + definition->powerRange); ++x) {
                if (std::abs(x - cell.x) + std::abs(y - cell.y) <= definition->powerRange) {
                    DrawRectangle(x * Cell_Size, y * Cell_Size, Cell_Size, Cell_Size, Fade(YELLOW, 0.12F));
                }
            }
        }
    }
}
}  // namespace

int main(int argc, char** argv) {
    try {
        std::filesystem::path mapPath = argc > 1
                                            ? std::filesystem::path(argv[1])
                                            : std::filesystem::path("data/maps/sample_map.json");

        SimulationState state = loadMap(mapPath);
        const auto& definitions = state.catalog.getDefinitions();
        if (definitions.empty()) {
            throw std::runtime_error("No facilities found in catalog");
        }

        std::size_t selectedDefinitionIndex = 0;
        Rotation currentRotation = Rotation::Deg_0;
        int selectedInstanceId = 0;

        InitWindow(Window_Width, Window_Height, "Endfield Base Simulator");
        SetTargetFPS(60);

        while (!WindowShouldClose()) {
            const Vector2 mousePosition = GetMousePosition();
            const GridPoint hoveredCell = screenToGrid(mousePosition);
            const bool mouseInsideGrid = hoveredCell.x >= 0
                                         && hoveredCell.y >= 0
                                         && hoveredCell.x < state.grid.getWidth()
                                         && hoveredCell.y < state.grid.getHeight();

            if (IsKeyPressed(KEY_TAB)) {
                selectedDefinitionIndex = (selectedDefinitionIndex + 1) % definitions.size();
            }
            if (IsKeyPressed(KEY_R)) {
                currentRotation = nextRotation(currentRotation);
            }
            if (IsKeyPressed(KEY_S)) {
                saveMap(mapPath, state);
            }

            if (mouseInsideGrid && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                const FacilityDefinition& definition = definitions.at(selectedDefinitionIndex);
                std::string reason;
                const int instanceId = state.grid.placeFacility(
                    definition,
                    definition.id,
                    hoveredCell,
                    currentRotation,
                    std::nullopt,
                    &reason
                );
                if (instanceId != 0) {
                    selectedInstanceId = instanceId;
                }
            }

            if (mouseInsideGrid && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                const std::optional<int> instanceId = facilityAtPoint(state, hoveredCell);
                if (instanceId.has_value()) {
                    state.grid.removeFacility(*instanceId, state.catalog);
                    if (selectedInstanceId == *instanceId) {
                        selectedInstanceId = 0;
                    }
                }
            }

            if (mouseInsideGrid && IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
                const std::optional<int> instanceId = facilityAtPoint(state, hoveredCell);
                selectedInstanceId = instanceId.value_or(0);
            }

            const ResultReport report = buildResultReport(state);

            BeginDrawing();
            ClearBackground(RAYWHITE);

            drawPowerCoverage(state, selectedInstanceId);

            for (int y = 0; y < state.grid.getHeight(); ++y) {
                for (int x = 0; x < state.grid.getWidth(); ++x) {
                    DrawRectangleLines(x * Cell_Size, y * Cell_Size, Cell_Size, Cell_Size, LIGHTGRAY);
                }
            }

            for (const FacilityInstance& instance : state.grid.getFacilities()) {
                drawFacility(state, instance, instance.instanceId == selectedInstanceId);
            }

            DrawRectangle(Window_Width - Sidebar_Width, 0, Sidebar_Width, Window_Height, Fade(LIGHTGRAY, 0.4F));

            int cursorY = 16;
            const int sidebarX = Window_Width - Sidebar_Width + 16;
            DrawText("Controls", sidebarX, cursorY, 22, BLACK);
            cursorY += 32;
            DrawText("Tab: Next facility", sidebarX, cursorY, 18, BLACK);
            cursorY += 22;
            DrawText("R: Rotate", sidebarX, cursorY, 18, BLACK);
            cursorY += 22;
            DrawText("LMB: Place", sidebarX, cursorY, 18, BLACK);
            cursorY += 22;
            DrawText("RMB: Delete", sidebarX, cursorY, 18, BLACK);
            cursorY += 22;
            DrawText("MMB: Select", sidebarX, cursorY, 18, BLACK);
            cursorY += 22;
            DrawText("S: Save map", sidebarX, cursorY, 18, BLACK);
            cursorY += 40;

            const FacilityDefinition& selectedDefinition = definitions.at(selectedDefinitionIndex);
            DrawText("Placement", sidebarX, cursorY, 22, BLACK);
            cursorY += 32;
            DrawText(selectedDefinition.displayName.c_str(), sidebarX, cursorY, 20,
                     facilityColor(selectedDefinition.category));
            cursorY += 24;
            DrawText(toString(currentRotation).c_str(), sidebarX, cursorY, 18, BLACK);
            cursorY += 40;

            DrawText("Summary", sidebarX, cursorY, 22, BLACK);
            cursorY += 32;

            std::ostringstream summary;
            summary.precision(2);
            summary << std::fixed;
            summary << "Throughput: " << report.summary.totalThroughput;
            DrawText(summary.str().c_str(), sidebarX, cursorY, 18, BLACK);
            cursorY += 22;
            summary.str("");
            summary << "Production: " << report.summary.totalProduction;
            DrawText(summary.str().c_str(), sidebarX, cursorY, 18, BLACK);
            cursorY += 22;
            summary.str("");
            summary << "Consumption: " << report.summary.totalConsumption;
            DrawText(summary.str().c_str(), sidebarX, cursorY, 18, BLACK);
            cursorY += 22;
            summary.str("");
            summary << "Power: " << report.summary.totalServedPower << " / " << report.summary.totalGeneration;
            DrawText(summary.str().c_str(), sidebarX, cursorY, 18, BLACK);
            cursorY += 22;
            summary.str("");
            summary << "Space efficiency: " << report.layoutScore.spaceEfficiency;
            DrawText(summary.str().c_str(), sidebarX, cursorY, 18, BLACK);
            cursorY += 36;

            if (selectedInstanceId != 0) {
                const FacilityInstance* selectedInstance = state.grid.findFacility(selectedInstanceId);
                if (selectedInstance != nullptr) {
                    const FacilityDefinition* definition = state.catalog.findDefinition(selectedInstance->definitionId);
                    DrawText("Selected", sidebarX, cursorY, 22, BLACK);
                    cursorY += 32;
                    DrawText(definition->displayName.c_str(), sidebarX, cursorY, 20, BLACK);
                    cursorY += 24;
                    summary.str("");
                    summary << "Footprint: " << rotateFootprint(definition->footprint, selectedInstance->rotation).width
                        << "x" << rotateFootprint(definition->footprint, selectedInstance->rotation).height;
                    DrawText(summary.str().c_str(), sidebarX, cursorY, 18, BLACK);
                    cursorY += 22;
                    summary.str("");
                    summary << "Power usage: " << definition->powerUsage;
                    DrawText(summary.str().c_str(), sidebarX, cursorY, 18, BLACK);
                    cursorY += 22;
                    summary.str("");
                    summary << "Base throughput: " << definition->baseThroughput;
                    DrawText(summary.str().c_str(), sidebarX, cursorY, 18, BLACK);
                    cursorY += 22;

                    const auto resultIt = std::ranges::find_if(
                        report.facilityResults,
                        [selectedInstanceId] (const FacilityThroughputResult& result) {
                            return result.instanceId == selectedInstanceId;
                        }
                    );
                    if (resultIt != report.facilityResults.end()) {
                        summary.str("");
                        summary << "Current throughput: " << resultIt->throughputPerSecond;
                        DrawText(summary.str().c_str(), sidebarX, cursorY, 18, BLACK);
                        cursorY += 22;
                        summary.str("");
                        summary << "Utilization: " << resultIt->utilization;
                        DrawText(summary.str().c_str(), sidebarX, cursorY, 18, BLACK);
                        cursorY += 22;
                        DrawText(resultIt->bottleneckReason.c_str(), sidebarX, cursorY, 18, MAROON);
                    }
                }
            }

            EndDrawing();
        }

        CloseWindow();
        return 0;
    } catch (const std::exception& exception) {
        TraceLog(LOG_ERROR, "%s", exception.what());
        return 1;
    }
}
