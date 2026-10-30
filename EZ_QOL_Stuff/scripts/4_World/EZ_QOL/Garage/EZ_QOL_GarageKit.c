// Packed garage. Stealable: no owner lock. Carries only the storage id + vehicle type (ledger is truth).
class EZ_QOL_GarageKit : ItemBase
{
	protected string m_EZ_StorageId;
	protected string m_EZ_VehType;
	protected bool m_EZ_HasVehicle;

	void EZ_QOL_GarageKit()
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

	override bool IsDeployable() { return true; }
	override bool CanBePlaced(Man player, vector position)
	{
		// client hologram uses the synced config; server re-validates in EZ_QOL_GarageModule.Deploy
		vector p, o; string r, d;
		return EZ_QOL_GaragePlacement.Validate(PlayerBase.Cast(player), position, GetOrientation(), p, o, r, d, this);
	}

	override void OnPlacementComplete(Man player, vector position = "0 0 0", vector orientation = "0 0 0")
	{
		// Do not call super: we replace the kit with an EZ_QOL_Garage instead of dropping it in place.
		if (!GetGame().IsServer()) return;
		PlayerBase pb = PlayerBase.Cast(player);
		string reason = EZ_QOL_GarageModule.Get().Deploy(this, pb, position, orientation);
		EZ_QOL_GarageModule.SendResult(pb, reason);
	}

	override string GetDeploySoundset() { return "putDown_FenceKit_SoundSet"; }

	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionTogglePlaceObject);
		AddAction(EZ_QOL_ActionDeployGarageKit);
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
		// No file move here: deletion at shutdown is ambiguous. The lastSeen sweep handles lost kits.
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

	override bool DescriptionOverride(out string output)
	{
		if (m_EZ_HasVehicle) output = "#STR_EZ_QOL_KIT_HAS_VEHICLE";
		else output = "#STR_EZ_QOL_KIT_EMPTY";
		return true;
	}
}

// Hologram projection class (config: scope 1, same model).
class EZ_QOL_GarageHologram : ItemBase {}
