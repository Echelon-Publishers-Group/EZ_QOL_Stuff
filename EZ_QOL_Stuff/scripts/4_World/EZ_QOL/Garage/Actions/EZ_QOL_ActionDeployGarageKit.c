// Vanilla deploy flow restricted to our kit; the kit's OnPlacementComplete swaps in the garage server-side.
class EZ_QOL_ActionDeployGarageKit : ActionDeployObject
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (!item || !item.IsInherited(EZ_QOL_GarageKit)) return false;
		return super.ActionCondition(player, target, item);
	}
}
