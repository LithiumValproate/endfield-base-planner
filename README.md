# Endfield Base Planner C#

This repository contains the C# port of the core planning logic from `endfield-base-planner`.

## Project layout

- `EndfieldBase.sln`: solution entrypoint
- `src/EndfieldBase.Core`: core domain and simulation logic
- `tests`: reserved for test projects
- `data`: reserved for sample data and fixtures

## Scope

The current port includes only the project core logic modules:

- facility/domain rules
- grid placement
- logistics graph
- path finding
- power evaluation
- storage endpoint discovery
- throughput evaluation

The current port intentionally excludes peripheral modules such as JSON IO, viewer, and export/report tooling.

## Build

```bash
dotnet build EndfieldBase.sln
```

## Run

```bash
dotnet run --project src/EndfieldBase.App
```

## Test

```bash
dotnet test EndfieldBase.sln
```

## Target framework

The projects target `net10.0` and are intended to be built with the `dotnet` SDK.
