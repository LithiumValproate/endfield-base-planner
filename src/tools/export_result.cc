#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "endfield_base/json_io.h"

using namespace endfield_base;

int main(int argc, char** argv) {
    try {
        if (argc < 3) {
            std::cerr << "Usage: endfield_base_export <map.json> <result.json>\n";
            return 1;
        }

        const std::filesystem::path mapPath = argv[1];
        const std::filesystem::path resultPath = argv[2];

        const SimulationState state = loadMap(mapPath);
        const ResultReport report = buildResultReport(state);
        exportResultReport(resultPath, report);

        std::cout << "Exported result report to " << resultPath << '\n';
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }
}
