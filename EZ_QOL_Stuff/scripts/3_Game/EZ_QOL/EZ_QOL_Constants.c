// EZ_QOL shared constants (garage team + ClearcREEk). Shared file: change only with both teams agreeing.
const string EZ_QOL_PROFILE_DIR      = "$profile:EZ_QOL";
const string EZ_QOL_GARAGE_DIR       = "$profile:EZ_QOL/Garage";
const string EZ_QOL_GARAGE_ORPHANS   = "$profile:EZ_QOL/Garage/Orphans";
const string EZ_QOL_GARAGE_LEDGER    = "$profile:EZ_QOL/Garage/ledger.json";
const string EZ_QOL_GARAGE_CONFIG    = "$profile:EZ_QOL/GarageConfig.json";

const int EZ_QOL_MENU_TABLET = 47310; // UIScriptedMenu id for EZ_QOL_TabletShell

// Garage model (data/garage): 4.6 x 7.4 x 4.659 m, interior clearance 3.547 m
const float EZ_QOL_GARAGE_WIDTH  = 4.6;
const float EZ_QOL_GARAGE_LENGTH = 7.4;
const float EZ_QOL_GARAGE_HEIGHT = 4.659;

// Storage state kept in the ledger (source of truth); holders only carry the id.
enum EZ_QOL_GarageState
{
	EMPTY = 0,
	PENDING,     // park started, file not confirmed yet
	STORED,      // vehicle is in <id>.bin
	RETRIEVING,  // restore started, .bak exists
	ORPHANED     // holder lost, file moved to Orphans/
}

// Reason keys (stringtable STR_EZ_QOL_*). "" = OK.
class EZ_QOL_Reason
{
	static const string OK            = "";
	static const string DISABLED      = "STR_EZ_QOL_DISABLED";
	static const string SAFEZONE      = "STR_EZ_QOL_PLACE_SAFEZONE";
	static const string TRADER        = "STR_EZ_QOL_PLACE_TRADER";
	static const string SLOPE         = "STR_EZ_QOL_PLACE_SLOPE";
	static const string BLOCKED       = "STR_EZ_QOL_PLACE_BLOCKED";
	static const string TOO_CLOSE     = "STR_EZ_QOL_PLACE_TOO_CLOSE";
	static const string BLACKLIST     = "STR_EZ_QOL_PLACE_BLACKLIST";
	static const string WATER         = "STR_EZ_QOL_PLACE_WATER";
	static const string LOCKED        = "STR_EZ_QOL_LOCKED";
	static const string HAS_LOCK      = "STR_EZ_QOL_PACK_HAS_LOCK";
	static const string NOT_EMPTY     = "STR_EZ_QOL_NOT_EMPTY";
	static const string EMPTY         = "STR_EZ_QOL_EMPTY";
	static const string NO_VEHICLE    = "STR_EZ_QOL_NO_VEHICLE";
	static const string ENGINE_ON     = "STR_EZ_QOL_ENGINE_ON";
	static const string CREW          = "STR_EZ_QOL_CREW";
	static const string TOO_BIG       = "STR_EZ_QOL_TOO_BIG";
	static const string VEH_BLACKLIST = "STR_EZ_QOL_VEH_BLACKLIST";
	static const string SPOT_BLOCKED  = "STR_EZ_QOL_SPOT_BLOCKED";
	static const string BUSY          = "STR_EZ_QOL_BUSY";
	static const string RATE          = "STR_EZ_QOL_RATE";
	static const string FAILED        = "STR_EZ_QOL_FAILED";
	static const string PARKED        = "STR_EZ_QOL_PARKED";
	static const string RETRIEVED     = "STR_EZ_QOL_RETRIEVED";
	static const string PACKED        = "STR_EZ_QOL_PACKED";
	static const string PLANTED       = "STR_EZ_QOL_PLANTED";
}
