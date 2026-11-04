// Adds the sleeping bag as a spawn point in Expansion's spawn selection menu.
// Expansion only calls GetTerritoryList when SpawnOnTerritory is enabled in its SpawnSettings.json.
modded class ExpansionRespawnHandlerModule
{
	void ExpansionRespawnHandlerModule()
	{
		EZ_QOL_SleepingBagRegistry.s_ExternalSpawnSelect = true;
	}

	override array<ref ExpansionSpawnLocation> GetTerritoryList(string playerUID)
	{
		array<ref ExpansionSpawnLocation> list = super.GetTerritoryList(playerUID);
		if (!list)
			list = new array<ref ExpansionSpawnLocation>;
		if (!GetGame().IsServer())
			return list;
		vector bagPos;
		EZ_SleepingBag_Deployed bag;
		string reason = EZ_QOL_SleepingBagRegistry.Get().GetSpawn(playerUID, bagPos, bag);
		if (reason != "")
			return list;
		array<vector> positions = new array<vector>;
		positions.Insert(bagPos);
		ExpansionSpawnLocation loc = new ExpansionSpawnLocation();
		loc.SetLocation("Sleeping bag", positions);
		list.Insert(loc);
		return list;
	}
}
