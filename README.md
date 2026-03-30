# Endfield Base Simulator

Endfield Base Simulator is a small C++ project for experimenting with grid-based base layouts, logistics flow, power coverage, and stable throughput evaluation.

## Project Focus

This project is a base layout and production analysis simulator, not an animation-heavy game shell.

The current product direction has three primary goals:

- Build a reliable rules system for placing facilities on a 2D orthogonal grid, including footprint handling, rotation, conveyors, splitters, bridges, power coverage, and storage connectivity.
- Evaluate layouts with stable-state analysis, including per-second throughput, production and consumption balance, transport loss from path length, power coverage, and space efficiency.
- Keep clear extension points for future work such as path search, layout scoring, and automatic optimization, while first making the rules system and visual editor dependable.

In short, the project should prioritize a trustworthy simulation core and layout evaluation workflow before expanding into advanced automation features.

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

## Near-Term Priorities

The expected implementation order is:

1. Continue refining facility and logistics rules.
2. Complete the storage system and external IO model.
3. Turn the visual editor into a practical analysis tool.
4. Expand the evaluation model beyond total throughput.
5. Reserve interfaces for advanced optimization features.

More specifically, the current priority areas are:

- Refine core rules such as splitter port-by-port round robin behavior, merger rules, explicit IO directions for each facility, storage and box interaction, and servo rate limiting and counters.
- Complete the storage model, including central hubs, peripheral access lines, access ports, base-edge connections, peripheral IO rules, and clear UI presentation of these relationships.
- Improve the visual editor so it supports practical analysis workflows: placement, rotation, deletion, selection with parameter inspection, power coverage highlighting, logistics path highlighting, bottleneck hints, and result panels.
- Extend evaluation beyond total throughput to include local bottlenecks, link congestion, power generation and consumption balance, space efficiency, and facility utilization.
- Leave room for later research directions such as path search, layout scoring, auto-suggestions, and auto-optimization, without treating them as the current mainline scope.

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
