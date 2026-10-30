// Smoke marker at the drop: a non-pickable M18 variant that is popped immediately on the server.
// config.cpp: class EZ_QOL_DroneMarker: M18SmokeGrenade_Red { scope=1; } (see DRONE_CONFIG_CPP_REQUESTS.md)
class EZ_QOL_DroneMarker: M18SmokeGrenade_Red
{
	void EZ_QOL_Activate()
	{
		if (!GetGame().IsServer())
			return;
		Unpin();
		ActivateImmediate(); // UNVERIFIED for smoke grenades; fallback: ActivateRandomTime() or OnActivatedByItem(this)
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(EZ_QOL_Cleanup, 120000, false);
	}

	void EZ_QOL_Cleanup()
	{
		if (GetGame() && !IsSetForDeletion())
			DeleteSafe();
	}

	override bool CanPutInCargo(EntityAI parent) { return false; }
	override bool CanPutIntoHands(EntityAI parent) { return false; }
}
