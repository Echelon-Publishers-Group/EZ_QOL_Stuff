class EZ_QOL_ActionOpenTablet : ActionSingleUseBase
{
	void EZ_QOL_ActionOpenTablet()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_ITEM_ON;
		m_Text = "#STR_EZ_QOL_ACT_OPEN_TABLET";
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
		// the menu class lives in 5_Mission; open by id (MissionGameplay.CreateScriptedMenu handles it)
		if (!GetGame().GetUIManager().GetMenu())
			GetGame().GetUIManager().EnterScriptedMenu(EZ_QOL_MENU_TABLET, null);
	}
}
