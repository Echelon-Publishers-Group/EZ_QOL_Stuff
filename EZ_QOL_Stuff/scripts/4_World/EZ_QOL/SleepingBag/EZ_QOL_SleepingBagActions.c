// Sleeping bag actions. Registered in Garage/Actions/EZ_QOL_ActionConstructor.c.

// Deploy: vanilla hologram flow, restricted to the packed bag.
class EZ_QOL_ActionDeploySleepingBag : ActionDeployObject
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (!item || !item.IsInherited(EZ_SleepingBag_Packed))
			return false;
		return super.ActionCondition(player, target, item);
	}
}

// Pack (hold F, empty hands). Cargo must be empty.
class EZ_QOL_ActionPackSleepingBagCB : ActionContinuousBaseCB
{
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new CAContinuousTime(UATimeSpent.DEFAULT_DEPLOY);
	}
}

class EZ_QOL_ActionPackSleepingBag : ActionContinuousBase
{
	void EZ_QOL_ActionPackSleepingBag()
	{
		m_CallbackClass = EZ_QOL_ActionPackSleepingBagCB;
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_INTERACT;
		m_FullBody = true;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_CROUCH | DayZPlayerConstants.STANCEMASK_ERECT;
		m_Text = "#STR_EZ_QOL_BAG_PACK";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINone;
		m_ConditionTarget = new CCTObject(UAMaxDistances.DEFAULT);
	}

	override typename GetInputType() { return ContinuousInteractActionInput; }
	override bool HasProgress() { return true; }
	override bool HasTarget() { return true; }

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		EZ_SleepingBag_Deployed bag = EZ_SleepingBag_Deployed.Cast(target.GetObject());
		if (!bag || bag.IsRuined())
			return false;
		return bag.EZ_QOL_CargoEmpty();
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		EZ_SleepingBag_Deployed bag = EZ_SleepingBag_Deployed.Cast(action_data.m_Target.GetObject());
		if (!bag)
			return;
		PlayerBase pb = action_data.m_Player;
		if (!EZ_QOL_SleepingBagConfig.Get().AllowOthersPackOrDestroy && bag.EZ_QOL_HasOwner() && !bag.EZ_QOL_IsOwner(pb))
		{
			EZ_QOL_SleepingBagRegistry.Notify(pb, "Only the owner can pack this sleeping bag");
			return;
		}
		if (!bag.EZ_QOL_CargoEmpty())
		{
			EZ_QOL_SleepingBagRegistry.Notify(pb, "Empty the sleeping bag first");
			return;
		}
		bag.EZ_QOL_Pack(pb);
	}
}

// Claim: bind someone else's (or an unowned) bag to me. Moves my binding.
class EZ_QOL_ActionClaimSleepingBag : ActionInteractBase
{
	void EZ_QOL_ActionClaimSleepingBag()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_CROUCH | DayZPlayerConstants.STANCEMASK_ERECT;
		m_Text = "#STR_EZ_QOL_BAG_CLAIM";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINone;
		m_ConditionTarget = new CCTObject(UAMaxDistances.DEFAULT);
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		EZ_SleepingBag_Deployed bag = EZ_SleepingBag_Deployed.Cast(target.GetObject());
		if (!bag || bag.IsRuined())
			return false;
		return !bag.EZ_QOL_IsOwner(player);
	}

	override void OnExecuteServer(ActionData action_data)
	{
		EZ_SleepingBag_Deployed bag = EZ_SleepingBag_Deployed.Cast(action_data.m_Target.GetObject());
		PlayerBase pb = action_data.m_Player;
		if (!bag || bag.IsRuined() || !pb || !pb.GetIdentity())
			return;
		string reason = EZ_QOL_SleepingBagRegistry.Get().ZoneBlock(bag.GetPosition());
		if (reason != "")
		{
			EZ_QOL_SleepingBagRegistry.Notify(pb, reason);
			return;
		}
		if (bag.EZ_QOL_HasOwner())
			bag.EZ_QOL_ClearOwner(true);
		bag.EZ_QOL_SetOwner(pb.GetIdentity().GetId(), pb.GetIdentity().GetName());
		EZ_QOL_SleepingBagRegistry.Notify(pb, "Sleeping bag claimed: you will respawn here");
	}
}

// Repair: SewingKit in hands, one health level per completed action (never from ruined).
class EZ_QOL_ActionRepairSleepingBagCB : ActionContinuousBaseCB
{
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new CAContinuousTime(UATimeSpent.BASEBUILDING_REPAIR_FAST);
	}
}

class EZ_QOL_ActionRepairSleepingBag : ActionContinuousBase
{
	void EZ_QOL_ActionRepairSleepingBag()
	{
		m_CallbackClass = EZ_QOL_ActionRepairSleepingBagCB;
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_INTERACT;
		m_FullBody = true;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_CROUCH | DayZPlayerConstants.STANCEMASK_ERECT;
		m_SpecialtyWeight = UASoftSkillsWeight.PRECISE_LOW;
		m_Text = "#repair";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINonRuined;
		m_ConditionTarget = new CCTObject(UAMaxDistances.DEFAULT);
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (!item || !item.IsInherited(SewingKit))
			return false;
		EZ_SleepingBag_Deployed bag = EZ_SleepingBag_Deployed.Cast(target.GetObject());
		if (!bag || bag.IsRuined())
			return false;
		if (GetGame().IsServer() && !EZ_QOL_SleepingBagConfig.Get().RepairEnabled)
			return false;
		return bag.GetHealthLevel("") > GameConstants.STATE_PRISTINE;
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		EZ_SleepingBag_Deployed bag = EZ_SleepingBag_Deployed.Cast(action_data.m_Target.GetObject());
		ItemBase kit = action_data.m_MainItem;
		if (!bag || bag.IsRuined() || !kit)
			return;
		if (!EZ_QOL_SleepingBagConfig.Get().RepairEnabled)
			return;
		bag.AddHealthLevel(-1, "");
		if (kit.HasQuantity())
			kit.AddQuantity(-10);
		else
			kit.DecreaseHealth("", "", 5);
	}
}

modded class SewingKit
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(EZ_QOL_ActionRepairSleepingBag);
	}
}
