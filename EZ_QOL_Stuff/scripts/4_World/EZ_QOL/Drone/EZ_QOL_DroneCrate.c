// Delivery crate. Server checks every 5 s: empty for CrateEmptyGraceSec -> delete; older than CrateMaxLifetimeSec -> delete.
class EZ_QOL_DroneCrate: Container_Base
{
	protected int m_EZ_SpawnTimeMs;
	protected int m_EZ_EmptySinceMs = -1;

	override void EEInit()
	{
		super.EEInit();
		if (GetGame().IsServer())
		{
			m_EZ_SpawnTimeMs = GetGame().GetTime();
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(EZ_QOL_CheckDespawn, 5000, true);
		}
	}

	override void EEDelete(EntityAI parent)
	{
		if (GetGame())
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(EZ_QOL_CheckDespawn);
		super.EEDelete(parent);
	}

	bool EZ_QOL_IsEmpty()
	{
		CargoBase cargo = GetInventory().GetCargo();
		if (cargo && cargo.GetItemCount() > 0)
			return false;
		if (GetInventory().AttachmentCount() > 0)
			return false;
		return true;
	}

	void EZ_QOL_CheckDespawn()
	{
		if (IsSetForDeletion())
			return;
		EZ_QOL_DroneConfig cfg = EZ_QOL_DroneConfig.Get();
		int now = GetGame().GetTime();

		if (now - m_EZ_SpawnTimeMs >= cfg.CrateMaxLifetimeSec * 1000)
		{
			EZ_QOL_DroneLog.Info("Crate " + GetPosition().ToString() + " reached max lifetime, deleted");
			EZ_QOL_Remove();
			return;
		}

		if (EZ_QOL_IsEmpty())
		{
			if (m_EZ_EmptySinceMs < 0)
				m_EZ_EmptySinceMs = now;
			else if (now - m_EZ_EmptySinceMs >= cfg.CrateEmptyGraceSec * 1000)
			{
				EZ_QOL_DroneLog.Info("Crate " + GetPosition().ToString() + " empty for " + cfg.CrateEmptyGraceSec + "s, deleted");
				EZ_QOL_Remove();
			}
		}
		else
		{
			m_EZ_EmptySinceMs = -1;
		}
	}

	protected void EZ_QOL_Remove()
	{
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(EZ_QOL_CheckDespawn);
		DeleteSafe();
	}

	// Not carryable, not storable
	override bool CanPutInCargo(EntityAI parent) { return false; }
	override bool CanPutIntoHands(EntityAI parent) { return false; }
	override bool IsTakeable() { return false; }
	// Don't let the crate get into persistence for long (timers restart on load, so a restart grants a fresh lifetime).
}
