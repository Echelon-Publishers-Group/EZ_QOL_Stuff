class EZ_QOL_Tablet : ItemBase
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(EZ_QOL_ActionOpenTablet);
		AddAction(EZ_QOL_ActionDroneOpenApp); // ClearcREEk: drone-only fallback menu (47311)
		AddAction(EZ_QOL_ActionAttackDroneOpenApp); // attack drone app direct action
	}
}
