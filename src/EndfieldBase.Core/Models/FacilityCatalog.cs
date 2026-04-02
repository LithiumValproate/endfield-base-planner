namespace EndfieldBase.Core.Models;

public sealed class FacilityCatalog
{
    private readonly List<FacilityDefinition> _definitions = [];
    private readonly Dictionary<string, int> _indexById = new(StringComparer.Ordinal);

    public void AddDefinition(FacilityDefinition definition) {
        if (definition.Role == FacilityRole.Generic)
            definition.Role = FacilityRules.DefaultFacilityRole(definition.Category);

        if (definition.PowerFacilityType == PowerFacilityType.None)
            definition.PowerFacilityType = FacilityRules.DefaultPowerFacilityType(definition.Category);

        if (definition.ItemStackLimit <= 0 && definition.InventorySlotCount > 0) definition.ItemStackLimit = 50;

        if (string.IsNullOrWhiteSpace(definition.Id))
            throw new InvalidOperationException("Facility definition id is required.");

        _indexById[definition.Id] = _definitions.Count;
        _definitions.Add(definition);
    }

    public FacilityDefinition? FindDefinition(string id) {
        return _indexById.TryGetValue(id, out var index) ? _definitions[index] : null;
    }

    public IReadOnlyList<FacilityDefinition> GetDefinitions() {
        return _definitions;
    }

    public bool Empty() {
        return _definitions.Count == 0;
    }
}