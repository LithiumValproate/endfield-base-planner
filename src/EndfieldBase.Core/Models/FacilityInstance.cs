using System.Collections.ObjectModel;

namespace EndfieldBase.Core.Models;

public sealed class FacilityInstance
{
    public int InstanceId { get; set; }
    public string DefinitionId { get; set; } = string.Empty;
    public GridPoint Position { get; set; }
    public Rotation Rotation { get; set; } = Rotation.Deg0;
    public StorageMode StorageMode { get; set; } = StorageMode.Conveyor;
    public Collection<InventorySlot> InventorySlots { get; } = [];
    public int PassedItemCount { get; set; }
    public int? PassedItemLimit { get; set; }
}