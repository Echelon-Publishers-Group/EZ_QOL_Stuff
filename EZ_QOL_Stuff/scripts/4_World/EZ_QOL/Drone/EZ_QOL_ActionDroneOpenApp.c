// Opens the Drone Delivery app from a tablet in hands. Orders go through the app
// (generic EZ_QOL_Drone_PlaceOrder(categoryId)), so there is no per-category action any more.
// Until Chief's EZ_QOL_TabletShell is real, this opens the drone-only fallback menu
// EZ_QOL_MENU_DRONE (EZ_QOL_DroneAppMenu, 5_Mission). Once the shell exists Chief can
// drop this action and open the app via EZ_QOL_TabletShell.OpenApp("drone").
class EZ_QOL_ActionDroneOpenApp: ActionSingleUseBase
{
	void EZ_QOL_ActionDroneOpenApp()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_ITEM_ON;
		m_Text = "Open Drone Delivery";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINonRuined;
		m_ConditionTarget = new CCTNone;
	}

	override bool HasTarget() { return false; }

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		return item && item.IsInherited(EZ_QOL_Tablet);
	}

	override void OnExecuteClient(ActionData action_data)
	{
		if (GetGame().GetUIManager().GetMenu() == null)
			GetGame().GetUIManager().EnterScriptedMenu(EZ_QOL_MENU_DRONE, null);
	}
}

// Compatibility name: Chief's EZ_QOL_Tablet.SetActions() still adds EZ_QOL_ActionDroneOrderFood
// (the old first-slice food action). It now just opens the Drone app with all 4 categories.
// Chief can switch SetActions to EZ_QOL_ActionDroneOpenApp (or drop it once the shell opens the app).
class EZ_QOL_ActionDroneOrderFood: EZ_QOL_ActionDroneOpenApp {}

modded class ActionConstructor
{
	override void RegisterActions(TTypenameArray actions)
	{
		super.RegisterActions(actions);
		actions.Insert(EZ_QOL_ActionDroneOpenApp);
		actions.Insert(EZ_QOL_ActionDroneOrderFood);
	}
}
