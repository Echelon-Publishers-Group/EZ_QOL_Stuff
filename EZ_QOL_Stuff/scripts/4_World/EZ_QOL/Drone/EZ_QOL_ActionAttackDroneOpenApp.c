// Opens the Attack Drone app standalone (menu EZ_QOL_MENU_ATTACKDRONE, 3_Game) from a tablet in hands.
// Normally opened from the tablet home screen (EZ_QOL_TabletShell.RegisterApp in 5_Mission).
class EZ_QOL_ActionAttackDroneOpenApp: ActionSingleUseBase
{
	void EZ_QOL_ActionAttackDroneOpenApp()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_ITEM_ON;
		m_Text = "#STR_EZ_QOL_ACT_OPEN_ATTACKDRONE";
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
			GetGame().GetUIManager().EnterScriptedMenu(EZ_QOL_MENU_ATTACKDRONE, null);
	}
}

modded class ActionConstructor
{
	override void RegisterActions(TTypenameArray actions)
	{
		super.RegisterActions(actions);
		actions.Insert(EZ_QOL_ActionAttackDroneOpenApp);
	}
}

// Not added to EZ_QOL_Tablet.SetActions on purpose: EZ_QOL_Tablet.c is Ewan's V0.9.1 file and a
// 'modded class EZ_QOL_Tablet' in this folder could load before the class itself (compile risk).
// The app is opened from its tablet home-screen icon. To also get a direct action, add
//     AddAction(EZ_QOL_ActionAttackDroneOpenApp);
// to EZ_QOL_Tablet.SetActions().
