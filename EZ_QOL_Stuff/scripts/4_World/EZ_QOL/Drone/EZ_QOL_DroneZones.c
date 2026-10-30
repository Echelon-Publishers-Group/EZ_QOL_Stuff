// Safezone / territory checks, isolated so the Expansion dependency stays in one place.
class EZ_QOL_DroneZones
{
	static bool InSafezone(vector pos)
	{
	// EXPANSIONMODCORE is not seen in Expansion sources; Market implies Core, so accept either define.
	#ifdef EXPANSIONMODCORE
		return ExpansionZoneModule.IsInsideSafeZone(pos);
	#else
		#ifdef EXPANSIONMODMARKET
		return ExpansionZoneModule.IsInsideSafeZone(pos);
		#else
		return false;
		#endif
	#endif
	}

	static bool InTerritory(vector pos)
	{
	#ifdef EXPANSIONMODBASEBUILDING
		ExpansionTerritoryModule t;
		if (Class.CastTo(t, CF_ModuleCoreManager.Get(ExpansionTerritoryModule)))
			return t.IsInTerritory(pos);
	#endif
		return false;
	}
}
