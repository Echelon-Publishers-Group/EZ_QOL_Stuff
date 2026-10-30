// Planted garage. Takes an Expansion code lock (slot Att_ExpansionCodeLock), auto-wired by ItemBase.ExpansionFindCodeLockSlot.
class EZ_QOL_Garage : ItemBase
{
	protected string m_EZ_StorageId;
	protected string m_EZ_VehType;
	protected bool m_EZ_HasVehicle;

	void EZ_QOL_Garage()
	{
		RegisterNetSyncVariableBool("m_EZ_HasVehicle");
	}

	string EZ_QOL_GetStorageId() { return m_EZ_StorageId; }
	string EZ_QOL_GetVehType() { return m_EZ_VehType; }
	bool EZ_QOL_HasVehicle() { return m_EZ_HasVehicle; }

	void EZ_QOL_SetStorage(string id, string vehType)
	{
		m_EZ_StorageId = id;
		m_EZ_VehType = vehType;
		m_EZ_HasVehicle = vehType != "";
		SetSynchDirty();
	}

	void EZ_QOL_ClearStorage() { EZ_QOL_SetStorage("", ""); }

	vector EZ_QOL_GetVehicleSpot()
	{
		if (MemoryPointExists("vehicle_spot"))
			return ModelToWorld(GetMemoryPointPos("vehicle_spot"));
		return GetPosition();
	}

	vector EZ_QOL_GetVehicleOrientation()
	{
		if (MemoryPointExists("vehicle_spot") && MemoryPointExists("vehicle_dir"))
		{
			vector dir = ModelToWorld(GetMemoryPointPos("vehicle_dir")) - EZ_QOL_GetVehicleSpot();
			dir[1] = 0;
			return Vector(dir.Normalized().VectorToAngles()[0], 0, 0);
		}
		return GetOrientation();
	}

	// Code lock: unlocked or known user. UNVERIFIED: IsKnownUser semantics vs. code entry, check in game.
	bool EZ_QOL_CanPlayerUse(PlayerBase p)
	{
		if (!ExpansionIsLocked()) return true;
		return IsKnownUser(p);
	}

	// Expansion's attach action needs IsOpen(); a garage has no door state, so it reads as open while unlocked.
	override bool IsOpen() { return !ExpansionIsLocked(); }
	override bool CanPutInCargo(EntityAI parent) { return false; }
	override bool CanPutIntoHands(EntityAI parent) { return false; }
	override bool IsInventoryVisible() { return true; }

	override void SetActions()
	{
		super.SetActions();
		AddAction(EZ_QOL_ActionParkVehicle);
		AddAction(EZ_QOL_ActionRetrieveVehicle);
		AddAction(EZ_QOL_ActionPackGarage);
	}

	override void EEInit()
	{
		super.EEInit();
		if (GetGame().IsServer() && m_EZ_StorageId != "")
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(EZ_Register, 100, false);
	}

	protected void EZ_Register()
	{
		EZ_QOL_GarageModule m = EZ_QOL_GarageModule.Get();
		if (m && !m.RegisterHolder(m_EZ_StorageId, this))
			EZ_QOL_ClearStorage();
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);
		if (GetGame().IsServer() && m_EZ_StorageId != "" && EZ_QOL_GarageModule.Get())
			EZ_QOL_GarageModule.Get().UnregisterHolder(m_EZ_StorageId, this);
	}

	override void EEKilled(Object killer)
	{
		super.EEKilled(killer);
		if (GetGame().IsServer() && m_EZ_StorageId != "" && EZ_QOL_GarageModule.Get())
			EZ_QOL_GarageModule.Get().OnHolderRuined(m_EZ_StorageId);
	}

	override void OnStoreSave(ParamsWriteContext ctx)
	{
		super.OnStoreSave(ctx);
		ctx.Write(m_EZ_StorageId);
		ctx.Write(m_EZ_VehType);
	}

	override bool OnStoreLoad(ParamsReadContext ctx, int version)
	{
		if (!super.OnStoreLoad(ctx, version)) return false;
		if (!ctx.Read(m_EZ_StorageId)) return false;
		if (!ctx.Read(m_EZ_VehType)) return false;
		m_EZ_HasVehicle = m_EZ_VehType != "";
		return true;
	}
}
