class EZ_QOL_ActionPackGarageCB : ActionContinuousBaseCB
{
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new CAContinuousTime(8);
	}
}

class EZ_QOL_ActionPackGarage : ActionContinuousBase
{
	void EZ_QOL_ActionPackGarage()
	{
		m_CallbackClass = EZ_QOL_ActionPackGarageCB;
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_INTERACT;
		m_FullBody = true;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
		m_Text = "#STR_EZ_QOL_ACT_PACK";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINone;
		m_ConditionTarget = new CCTCursor(UAMaxDistances.DEFAULT);
	}

	override bool HasProgress() { return true; }

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		EZ_QOL_Garage g = EZ_QOL_Garage.Cast(target.GetObject());
		if (!g) return false;
		return !g.ExpansionHasCodeLock(); // pack only without a lock (STR_EZ_QOL_PACK_HAS_LOCK server-side too)
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		EZ_QOL_Garage g = EZ_QOL_Garage.Cast(action_data.m_Target.GetObject());
		EZ_QOL_GarageModule.SendResult(action_data.m_Player, EZ_QOL_GarageModule.Get().Pack(g, action_data.m_Player));
	}
}
