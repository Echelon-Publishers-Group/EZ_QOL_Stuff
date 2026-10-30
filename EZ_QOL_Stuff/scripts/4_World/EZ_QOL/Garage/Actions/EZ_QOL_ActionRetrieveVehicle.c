class EZ_QOL_ActionRetrieveVehicle : ActionInteractBase
{
	void EZ_QOL_ActionRetrieveVehicle()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
		m_Text = "#STR_EZ_QOL_ACT_RETRIEVE";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINone;
		m_ConditionTarget = new CCTCursor(UAMaxDistances.DEFAULT);
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		EZ_QOL_Garage g = EZ_QOL_Garage.Cast(target.GetObject());
		return g && g.EZ_QOL_HasVehicle();
	}

	override void OnExecuteServer(ActionData action_data)
	{
		EZ_QOL_Garage g = EZ_QOL_Garage.Cast(action_data.m_Target.GetObject());
		EZ_QOL_GarageModule.SendResult(action_data.m_Player, EZ_QOL_GarageModule.Get().Retrieve(g, action_data.m_Player));
	}
}
