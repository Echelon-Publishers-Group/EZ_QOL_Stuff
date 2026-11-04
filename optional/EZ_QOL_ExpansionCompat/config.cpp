// OPTIONAL. Not part of the default EZ_QOL_Stuff PBO. Pack as its own PBO (e.g. EZ_QOL_ExpansionCompat.pbo)
// only on servers that run DayZ Expansion Spawn Selection. Lists the player's sleeping bag in
// Expansion's spawn menu (next to territories) and switches off the vanilla bag position override.
class CfgPatches
{
	class EZ_QOL_ExpansionCompat
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"EZ_QOL_Stuff", "DayZExpansion_SpawnSelection_Scripts"};
	};
};

class CfgMods
{
	class EZ_QOL_ExpansionCompat
	{
		dir = "EZ_QOL_ExpansionCompat";
		name = "EZ QOL Expansion compat";
		author = "Echelon Publishers Group";
		type = "mod";
		dependencies[] = {"Mission"};
		class defs
		{
			class missionScriptModule
			{
				value = "";
				files[] = {"EZ_QOL_ExpansionCompat/scripts/5_Mission"};
			};
		};
	};
};
