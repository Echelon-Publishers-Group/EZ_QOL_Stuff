// File name sorts after EZ_QOL_DeliveryDrone.c on purpose (base class first).
// Attack drone: delivery drone model, cargo box hidden, C4 proxy (Object Builder, Ewan), olive/black
// retexture via hiddenSelections "camo". Needs a damageable config (CfgVehicles EZ_QOL_AttackDrone:
// DamageSystem GlobalHealth hitpoints 60, see ATTACKDRONE_CONFIG_CPP_REQUESTS.md) to be shot down.
class EZ_QOL_AttackDrone: EZ_QOL_DeliveryDrone
{
	protected bool m_EZ_ShotDown;
	protected string m_EZ_LastShooterUid;

	void EZ_QOL_AttackDrone()
	{
		m_EZ_CargoAttached = false; // no cargo box on the attack drone
	}

	override void EEInit()
	{
		super.EEInit();
		EZ_QOL_ApplyCargoPhase();
	}

	bool EZ_QOL_IsShotDown()
	{
		return m_EZ_ShotDown || IsDamageDestroyed();
	}

	string EZ_QOL_GetLastShooterUid()
	{
		return m_EZ_LastShooterUid;
	}

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
		if (!GetGame().IsServer() || !source)
			return;
		Man shooter = source.GetHierarchyRootPlayer();
		if (shooter && shooter.GetIdentity())
			m_EZ_LastShooterUid = shooter.GetIdentity().GetId();
	}

	override void EEKilled(Object killer)
	{
		super.EEKilled(killer);
		m_EZ_ShotDown = true; // the strike manager sees this on its next tick and detonates/aborts
	}

	// Server: detonate here (vanilla C4 explosion, server-side damage).
	void EZ_QOL_Detonate(string ammoType)
	{
		if (!GetGame().IsServer())
			return;
		EZ_QOL_SetRotors(false);
		Explode(DamageType.EXPLOSION, ammoType);
	}
}
