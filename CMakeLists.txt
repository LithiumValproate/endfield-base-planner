cmake_minimum_required(VERSION 3.24)

project(endfield_base VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)

find_package(nlohmann_json CONFIG REQUIRED)

add_library(endfield_base_core
    src/facility.cc
    src/grid_map.cc
    src/json_io.cc
    src/logistics_graph.cc
    src/path_finder.cc
    src/power_system.cc
    src/result_report.cc
    src/storage_system.cc
    src/throughput_evaluator.cc
)

target_include_directories(endfield_base_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(endfield_base_core
    PUBLIC
        nlohmann_json::nlohmann_json
)

add_executable(endfield_base_export src/tools/export_result.cc)
target_link_libraries(endfield_base_export PRIVATE endfield_base_core)

find_path(RAYLIB_INCLUDE_DIR raylib.h)
find_library(RAYLIB_LIBRARY raylib)

if(RAYLIB_INCLUDE_DIR AND RAYLIB_LIBRARY)
    add_executable(endfield_base_viewer src/viewer/main.cc)
    target_include_directories(endfield_base_viewer PRIVATE ${RAYLIB_INCLUDE_DIR})
    target_link_libraries(endfield_base_viewer PRIVATE endfield_base_core ${RAYLIB_LIBRARY})
    target_compile_definitions(endfield_base_viewer PRIVATE RAYLIB_AVAILABLE=1)
else()
    message(STATUS "raylib not found; skipping viewer target")
endif()

if(BUILD_TESTING)
    add_executable(endfield_base_tests tests/endfield_base_tests.cc)
    target_include_directories(endfield_base_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    )
    target_link_libraries(endfield_base_tests PRIVATE endfield_base_core)
    add_test(NAME endfield_base_tests COMMAND endfield_base_tests)
endif()
