# Endfield Base Simulator

Endfield Base Simulator is a small C++ project for experimenting with grid-based base layouts, logistics flow, power coverage, and stable throughput evaluation.

The repository currently includes:

- a core simulation library
- a JSON import and export pipeline
- a CLI exporter for result reports
- an optional viewer and map editor when `raylib` is available
- validation tests for core systems

## Features

- Grid-based facility placement
- Logistics path evaluation
- Power coverage and generation checks
- Stable-state throughput reporting
- JSON-based facility definitions, map saves, and result exports

## Project Structure

- `include/endfield_base`: public headers
- `src`: core implementation, tools, and viewer
- `tests`: validation tests
- `data`: sample maps and result files
- `third_party`: bundled third-party sources when needed

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

`nlohmann_json` is required. The viewer target is built only when `raylib` is available on the local system.

## Targets

- `endfield_base_core`: core simulation library
- `endfield_base_export`: export a result JSON from a map JSON
- `endfield_base_viewer`: optional grid editor and report viewer
- `endfield_base_tests`: core validation tests

## Usage

Export a result report:

```bash
./build/endfield_base_export data/maps/sample_map.json data/results/sample_result.json
```

Run the viewer when `raylib` is installed:

```bash
./build/endfield_base_viewer data/maps/sample_map.json
```

## Modeling Notes

- All values are evaluated as stable-state per-second numbers.
- Path length is folded into throughput using a conservative transport penalty.
- Conveyor-related facilities and generators do not require power-pole coverage.
- Bridges use a separate traversal layer so they can cross conveyor cells without merging with the lower path.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.
