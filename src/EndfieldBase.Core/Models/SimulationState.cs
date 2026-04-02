using EndfieldBase.Core.Grid;

namespace EndfieldBase.Core.Models;

public sealed class SimulationState
{
    public FacilityCatalog Catalog { get; set; } = new();
    public GridMap Grid { get; set; } = new();
    public string FacilityCatalogPath { get; set; } = string.Empty;
    public string? LoadedFrom { get; set; }
}