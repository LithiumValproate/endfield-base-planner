using EndfieldBase.Core.Grid;
using EndfieldBase.Core.Models;

SimulationState state = new()
    {
        Grid = new GridMap(8, 6),
    };

Console.WriteLine("Endfield Base Planner C#");
Console.WriteLine($"Grid: {state.Grid.GetWidth()} x {state.Grid.GetHeight()}");
Console.WriteLine($"Catalog definitions: {state.Catalog.GetDefinitions().Count}");