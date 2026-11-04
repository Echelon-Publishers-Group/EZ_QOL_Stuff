// Packed bag: inventory item, deployed with the vanilla hologram flow (projectionTypename in config).
class EZ_SleepingBag_Packed : ItemBase
{
	override bool IsDeployable() { return true; }
	// true => ActionDeployObject.OnEndServer deletes the packed item after placement (KitBase pattern)
	override bool IsBasebuildingKit() { return true; }
	override string GetDeploySoundset() { return "placeTent_SoundSet"; }

	override bool CanBePlaced(Man player, vector position)
	{
		if (!super.CanBePlaced(player, position))
			return false;
		if (GetGame().IsServer())
		{
			if (EZ_QOL_SleepingBagRegistry.Get().ZoneBlock(position) != "")
				return false;
		}
		return true;
	}

	override void OnPlacementComplete(Man player, vector position = "0 0 0", vector orientation = "0 0 0")
	{
		super.OnPlacementComplete(player, position, orientation);
		if (!GetGame().IsServer())
			return;
		EZ_SleepingBag_Deployed bag = EZ_SleepingBag_Deployed.Cast(GetGame().CreateObjectEx("EZ_SleepingBag_Deployed", position, ECE_PLACE_ON_SURFACE));
		if (!bag)
			return;
		bag.SetPosition(position);
		bag.SetOrientation(orientation);
		bag.SetHealth01("", "", GetHealth01("", ""));
		PlayerBase pb = PlayerBase.Cast(player);
		if (pb && pb.GetIdentity())
		{
			bag.EZ_QOL_SetOwner(pb.GetIdentity().GetId(), pb.GetIdentity().GetName());
			EZ_QOL_SleepingBagRegistry.Notify(pb, "Sleeping bag placed: you will respawn here");
		}
		HideAllSelections(); // deleted by the deploy action when it ends
	}

	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionTogglePlaceObject);
		AddAction(EZ_QOL_ActionDeploySleepingBag);
	}
}

// Hologram projection (config scope 1, deployed model).
class EZ_SleepingBag_PackedPlacing : ItemBase {}

// Deployed bag: 10x10 cargo, persistent, bound to one owner (server registry).
class EZ_SleepingBag_Deployed : Container_Base
{
	static ref array<EZ_SleepingBag_Deployed> s_EZ_All = new array<EZ_SleepingBag_Deployed>;
	static const int STORE_VERSION = 1;

	protected string m_EZ_OwnerUID;
	protected string m_EZ_OwnerName;
	protected string m_EZ_BagKey;
	protected int m_EZ_OwnerHash;   // netsynced so a client can tell "yours" (strings cannot be synced)

	void EZ_SleepingBag_Deployed()
	{
		RegisterNetSyncVariableInt("m_EZ_OwnerHash");
	}

	string EZ_QOL_GetOwnerUID() { return m_EZ_OwnerUID; }
	string EZ_QOL_GetOwnerName() { return m_EZ_OwnerName; }
	string EZ_QOL_GetBagKey() { return m_EZ_BagKey; }
	bool EZ_QOL_HasOwner() { return m_EZ_OwnerHash != 0; }

	bool EZ_QOL_IsOwner(PlayerBase player)
	{
		if (!player || !player.GetIdentity() || m_EZ_OwnerHash == 0)
			return false;
		return player.GetIdentity().GetId().Hash() == m_EZ_OwnerHash;
	}

	// server
	void EZ_QOL_SetOwner(string uid, string ownerName)
	{
		if (m_EZ_BagKey == "")
			m_EZ_BagKey = EZ_QOL_SleepingBagRegistry.Get().NewKey();
		m_EZ_OwnerUID = uid;
		m_EZ_OwnerName = ownerName;
		m_EZ_OwnerHash = uid.Hash();
		if (m_EZ_OwnerHash == 0)
			m_EZ_OwnerHash = 1;
		SetSynchDirty();
		EZ_QOL_SleepingBagRegistry.Get().Bind(uid, ownerName, this);
	}

	// server. unbind=true also clears the registry entry (pack, ruin); false when the registry moved it.
	void EZ_QOL_ClearOwner(bool unbind)
	{
		if (unbind && m_EZ_OwnerUID != "")
			EZ_QOL_SleepingBagRegistry.Get().Unbind(m_EZ_OwnerUID, m_EZ_BagKey);
		m_EZ_OwnerUID = "";
		m_EZ_OwnerName = "";
		m_EZ_OwnerHash = 0;
		SetSynchDirty();
	}

	void EZ_QOL_ApplyDamage(float hp)
	{
		if (hp <= 0 || IsRuined())
			return;
		float newHp = GetHealth("", "") - hp;
		if (newHp < 0)
			newHp = 0;
		SetHealth("", "", newHp);
	}

	bool EZ_QOL_CargoEmpty()
	{
		if (!GetInventory())
			return true;
		CargoBase cargo = GetInventory().GetCargo();
		if (cargo && cargo.GetItemCount() > 0)
			return false;
		return GetInventory().AttachmentCount() == 0;
	}

	// Pack: back to a packed item at the same health, binding cleared. Cargo must be empty.
	bool EZ_QOL_Pack(PlayerBase player)
	{
		if (!GetGame().IsServer() || !EZ_QOL_CargoEmpty() || IsRuined())
			return false;
		vector pos = GetPosition();
		float health01 = GetHealth01("", "");
		EZ_QOL_ClearOwner(true);
		EntityAI packed;
		if (player)
			packed = player.GetHumanInventory().CreateInHands("EZ_SleepingBag_Packed");
		if (!packed)
			packed = EntityAI.Cast(GetGame().CreateObjectEx("EZ_SleepingBag_Packed", pos, ECE_PLACE_ON_SURFACE));
		if (packed)
			packed.SetHealth01("", "", health01);
		DeleteSafe();
		return true;
	}

	override bool IsTakeable() { return false; }
	override bool CanPutInCargo(EntityAI parent) { return false; }
	override bool CanPutIntoHands(EntityAI parent) { return false; }

	override bool CanReceiveItemIntoCargo(EntityAI item)
	{
		if (IsRuined())
			return false;
		return super.CanReceiveItemIntoCargo(item);
	}

	override string GetDisplayName()
	{
		string n = super.GetDisplayName();
		if (!GetGame().IsDedicatedServer() && m_EZ_OwnerHash != 0)
		{
			PlayerBase me = PlayerBase.Cast(GetGame().GetPlayer());
			if (EZ_QOL_IsOwner(me))
				n = n + " (" + Widget.TranslateString("#STR_EZ_QOL_BAG_YOURS") + ")";
		}
		return n;
	}

	override void EEInit()
	{
		super.EEInit();
		if (GetGame().IsServer())
		{
			s_EZ_All.Insert(this);
			if (m_EZ_OwnerUID != "")
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(EZ_QOL_CheckBinding, 100, false);
		}
	}

	protected void EZ_QOL_CheckBinding()
	{
		if (m_EZ_OwnerUID == "")
			return;
		if (!EZ_QOL_SleepingBagRegistry.Get().OnBagLoaded(this))
			EZ_QOL_ClearOwner(false);
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);
		if (GetGame().IsServer())
		{
			int idx = s_EZ_All.Find(this);
			if (idx > -1)
				s_EZ_All.Remove(idx);
			if (m_EZ_BagKey != "")
				EZ_QOL_SleepingBagRegistry.Get().OnBagGone(this);
		}
	}

	override void EEHealthLevelChanged(int oldLevel, int newLevel, string zone)
	{
		super.EEHealthLevelChanged(oldLevel, newLevel, zone);
		if (GetGame().IsServer() && zone == "" && newLevel == GameConstants.STATE_RUINED)
			EZ_QOL_ClearOwner(true);
	}

	override void EEKilled(Object killer)
	{
		super.EEKilled(killer);
		if (GetGame().IsServer())
			EZ_QOL_ClearOwner(true);
	}

	override void OnStoreSave(ParamsWriteContext ctx)
	{
		super.OnStoreSave(ctx);
		ctx.Write(STORE_VERSION);
		ctx.Write(m_EZ_OwnerUID);
		ctx.Write(m_EZ_OwnerName);
		ctx.Write(m_EZ_BagKey);
	}

	override bool OnStoreLoad(ParamsReadContext ctx, int version)
	{
		if (!super.OnStoreLoad(ctx, version))
			return false;
		int v;
		if (!ctx.Read(v))
			return false;
		if (!ctx.Read(m_EZ_OwnerUID))
			return false;
		if (!ctx.Read(m_EZ_OwnerName))
			return false;
		if (!ctx.Read(m_EZ_BagKey))
			return false;
		m_EZ_OwnerHash = 0;
		if (m_EZ_OwnerUID != "")
		{
			m_EZ_OwnerHash = m_EZ_OwnerUID.Hash();
			if (m_EZ_OwnerHash == 0)
				m_EZ_OwnerHash = 1;
		}
		return true;
	}

	override void SetActions()
	{
		super.SetActions();
		AddAction(EZ_QOL_ActionPackSleepingBag);
		AddAction(EZ_QOL_ActionClaimSleepingBag);
	}
}
