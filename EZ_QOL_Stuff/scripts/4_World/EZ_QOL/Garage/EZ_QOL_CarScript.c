// Dupe check hook. Deliberately does NOT add OnStoreSave data to CarScript: appending storage would shift
// other mods' reads (VGS, Expansion) on existing cars. Instead the module records vehicle persistent IDs
// in its ledger and matches them here. Super-calling override only; no actions added to CarScript.
modded class CarScript
{
	string EZ_QOL_GetPersistentKey()
	{
		int b1, b2, b3, b4;
		GetPersistentID(b1, b2, b3, b4);
		return b1.ToString() + "_" + b2.ToString() + "_" + b3.ToString() + "_" + b4.ToString();
	}

	override void AfterStoreLoad()
	{
		super.AfterStoreLoad();
		if (GetGame().IsServer())
			EZ_QOL_GarageModule.OnVehicleLoaded(this);
	}
}
