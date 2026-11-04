// EZ_QOL drone RPC names (CF RPCManager, namespace "EZ_QOL"). Can be merged into Chief's EZ_QOL_RPC.c later.
const string EZ_QOL_DRONE_RPC_NS = "EZ_QOL";
// client -> server
const string EZ_QOL_RPC_DRONE_REQUESTCATALOG = "EZ_QOL_Drone_RequestCatalog"; // no params (Param1<int> dummy)
const string EZ_QOL_RPC_DRONE_PLACEORDER     = "EZ_QOL_Drone_PlaceOrder";     // Param1<string categoryId>
// server -> client
const string EZ_QOL_RPC_DRONE_CATALOG        = "EZ_QOL_Drone_Catalog2";       // Param1<array<string>> json of EZ_QOL_DroneCatalog split in chunks
const int EZ_QOL_DRONE_CATALOG_CHUNK = 256;
const string EZ_QOL_RPC_DRONE_ORDERRESULT    = "EZ_QOL_Drone_OrderResult";    // Param3<bool ok, string reason, int etaSec>
const string EZ_QOL_RPC_DRONE_STATUS         = "EZ_QOL_Drone_Status";         // Param3<int state, int etaSec, bool windWarning>

// Drone-only fallback menu id (Chief uses 47310 for EZ_QOL_TabletShell)
const int EZ_QOL_MENU_DRONE = 47311;

// cargo_attached animation phase. model.cfg cargo_hide hides the selection when phase >= hideValue 0.5,
// so "attached/visible" must be 0. See DRONE_CONFIG_CPP_REQUESTS.md (initPhase).
const float EZ_QOL_DRONE_CARGO_PHASE_ATTACHED = 0.0;
const float EZ_QOL_DRONE_CARGO_PHASE_DROPPED  = 1.0;

// int values sent in EZ_QOL_Drone_Status
const int EZ_QOL_DRONE_STATE_IDLE      = 0;
const int EZ_QOL_DRONE_STATE_INBOUND   = 1;
const int EZ_QOL_DRONE_STATE_HOVER     = 2;
const int EZ_QOL_DRONE_STATE_OUTBOUND  = 3;
const int EZ_QOL_DRONE_STATE_DELIVERED = 4;
const int EZ_QOL_DRONE_STATE_CRASHED   = 5;

class EZ_QOL_DroneCatalogItem
{
	string className;
	string displayName;
	int qty;
}

class EZ_QOL_DroneCatalogCategory
{
	string id;
	string name;
	string icon;
	int price;
	int cooldownLeft;
	bool pickOne;
	ref array<ref EZ_QOL_DroneCatalogItem> items;

	void EZ_QOL_DroneCatalogCategory()
	{
		items = new array<ref EZ_QOL_DroneCatalogItem>;
	}
}

class EZ_QOL_DroneCatalog
{
	int balance;          // -1 = unknown
	bool windWarning;
	float windSpeed;
	string blockedReason; // "" = allowed here
	ref array<ref EZ_QOL_DroneCatalogCategory> categories;

	void EZ_QOL_DroneCatalog()
	{
		categories = new array<ref EZ_QOL_DroneCatalogCategory>;
	}
}

string EZ_QOL_DroneStateText(int state)
{
	switch (state)
	{
		case EZ_QOL_DRONE_STATE_INBOUND:   return "Drone inbound";
		case EZ_QOL_DRONE_STATE_HOVER:     return "Drone hovering, dropping crate";
		case EZ_QOL_DRONE_STATE_OUTBOUND:  return "Crate dropped, drone returning";
		case EZ_QOL_DRONE_STATE_DELIVERED: return "Delivered";
		case EZ_QOL_DRONE_STATE_CRASHED:   return "Drone crashed! Crate at crash site";
	}
	return "Idle";
}

// ---------------------------------------------------------------- attack drone (lock-on C4 strike)
// Own tablet app + fallback menu id (47310 = tablet shell, 47311 = drone delivery)
const int EZ_QOL_MENU_ATTACKDRONE = 47312;
// client -> server
const string EZ_QOL_RPC_STRIKE_INFO      = "EZ_QOL_Strike_Info";      // Param1<int> dummy: ask for price/cooldown/balance
const string EZ_QOL_RPC_STRIKE_ARM       = "EZ_QOL_Strike_Arm";       // Param1<int> dummy
const string EZ_QOL_RPC_STRIKE_CANDIDATE = "EZ_QOL_Strike_Candidate"; // Param4<int low, int high, vector groundPt, bool hasObject> hint only
const string EZ_QOL_RPC_STRIKE_CANCEL    = "EZ_QOL_Strike_Cancel";    // Param1<int> dummy
// server -> client
const string EZ_QOL_RPC_STRIKE_INFOREPLY = "EZ_QOL_Strike_InfoReply"; // Param5<int price, int cooldownLeft, int balance, string blockedReason, bool enabled>
const string EZ_QOL_RPC_STRIKE_STATE     = "EZ_QOL_Strike_State";     // Param5<int state, float lockPct, float graceLeft, int etaSec, string reason>
const string EZ_QOL_RPC_STRIKE_WARN      = "EZ_QOL_Strike_Warn";      // Param2<vector dronePos, int etaSec>

const int EZ_QOL_STRIKE_IDLE      = 0;
const int EZ_QOL_STRIKE_ARMED     = 1; // pre-lock window, aim at a target
const int EZ_QOL_STRIKE_ACQUIRING = 2; // lock building
const int EZ_QOL_STRIKE_INFLIGHT  = 3; // launched, hold the lock
const int EZ_QOL_STRIKE_DIVE      = 4;
const int EZ_QOL_STRIKE_HIT       = 5;
const int EZ_QOL_STRIKE_ABORTED   = 6;

string EZ_QOL_StrikeStateText(int state)
{
	switch (state)
	{
		case EZ_QOL_STRIKE_ARMED:     return "Armed: raise a weapon and aim at the target";
		case EZ_QOL_STRIKE_ACQUIRING: return "Acquiring lock - hold steady";
		case EZ_QOL_STRIKE_INFLIGHT:  return "Drone inbound - hold the lock";
		case EZ_QOL_STRIKE_DIVE:      return "Drone diving - hold the lock";
		case EZ_QOL_STRIKE_HIT:       return "Impact";
		case EZ_QOL_STRIKE_ABORTED:   return "Strike aborted";
	}
	return "Idle";
}
