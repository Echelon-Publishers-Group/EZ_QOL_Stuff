// $profile:EZ_QOL/GarageConfig.json loader (server). Clients get the placement subset via EZ_QOL_Garage_Config.
class EZ_QOL_Zone
{
	string Name;
	float X;
	float Z;
	float Radius;
}

class EZ_QOL_GarageClientConfig
{
	bool Enabled = true;
	float MaxSlopeDeg = 15;
	float MinDistanceBetweenGarages = 50;
	bool AllowInSafeZones = false;
	bool AllowInTraderZones = false;
	ref array<ref EZ_QOL_Zone> BlacklistZones;
	ref TStringArray IgnoreObstacleClasses;

	void EZ_QOL_GarageClientConfig()
	{
		BlacklistZones = new array<ref EZ_QOL_Zone>;
		IgnoreObstacleClasses = new TStringArray;
	}
}

class EZ_QOL_GarageConfig : EZ_QOL_GarageClientConfig
{
	static const int VERSION = 1;
	protected static ref EZ_QOL_GarageConfig s_Instance;

	int Version = VERSION;
	int OrphanDays = 30;          // holder not seen for this long -> Orphans/
	int OrphanPurgeDays = 30;     // orphan files deleted after this many more days
	float RpcCooldownSec = 2;
	float ParkRadius = 6;         // vehicle centre must be this close to vehicle_spot
	float UseDistance = 8;        // player to garage, for tablet RPCs
	ref array<float> MaxVehicleSize; // x (width), z (length), y (height) in m
	ref TStringArray VehicleBlacklist;
	bool RequireEngineOff = true;
	int StartupCheckDelaySec = 90; // wait for world vehicles to load before the dupe check
	bool Logging = true;

	void EZ_QOL_GarageConfig()
	{
		MaxVehicleSize = {4.4, 7.2, 3.5};
		VehicleBlacklist = new TStringArray;
	}

	static EZ_QOL_GarageConfig Get()
	{
		if (!s_Instance)
			s_Instance = new EZ_QOL_GarageConfig();
		return s_Instance;
	}

	static void Load()
	{
		EZ_QOL_Log.EnsureDirs();
		EZ_QOL_GarageConfig cfg = new EZ_QOL_GarageConfig();
		if (FileExist(EZ_QOL_GARAGE_CONFIG))
		{
			JsonFileLoader<EZ_QOL_GarageConfig>.JsonLoadFile(EZ_QOL_GARAGE_CONFIG, cfg);
			if (cfg.Version < VERSION)
			{
				cfg.Version = VERSION;
				JsonFileLoader<EZ_QOL_GarageConfig>.JsonSaveFile(EZ_QOL_GARAGE_CONFIG, cfg);
			}
		}
		else
		{
			cfg.IgnoreObstacleClasses.Insert("BushSoft");
			JsonFileLoader<EZ_QOL_GarageConfig>.JsonSaveFile(EZ_QOL_GARAGE_CONFIG, cfg);
			EZ_QOL_Log.Info("Wrote default " + EZ_QOL_GARAGE_CONFIG);
		}
		if (!cfg.MaxVehicleSize || cfg.MaxVehicleSize.Count() < 3)
			cfg.MaxVehicleSize = {4.4, 7.2, 3.5};
		s_Instance = cfg;
	}

	string ClientJson()
	{
		EZ_QOL_GarageClientConfig c = new EZ_QOL_GarageClientConfig();
		c.Enabled = Enabled;
		c.MaxSlopeDeg = MaxSlopeDeg;
		c.MinDistanceBetweenGarages = MinDistanceBetweenGarages;
		c.AllowInSafeZones = AllowInSafeZones;
		c.AllowInTraderZones = AllowInTraderZones;
		c.BlacklistZones = BlacklistZones;
		c.IgnoreObstacleClasses = IgnoreObstacleClasses;
		string json;
		JsonSerializer js = new JsonSerializer();
		js.WriteToString(c, false, json);
		return json;
	}

	// client side
	static void ApplyClientJson(string json)
	{
		EZ_QOL_GarageClientConfig c = new EZ_QOL_GarageClientConfig();
		string err;
		JsonSerializer js = new JsonSerializer();
		if (!js.ReadFromString(c, json, err))
		{
			Print("[EZ_QOL] bad client config: " + err);
			return;
		}
		EZ_QOL_GarageConfig cfg = Get();
		cfg.Enabled = c.Enabled;
		cfg.MaxSlopeDeg = c.MaxSlopeDeg;
		cfg.MinDistanceBetweenGarages = c.MinDistanceBetweenGarages;
		cfg.AllowInSafeZones = c.AllowInSafeZones;
		cfg.AllowInTraderZones = c.AllowInTraderZones;
		cfg.BlacklistZones = c.BlacklistZones;
		cfg.IgnoreObstacleClasses = c.IgnoreObstacleClasses;
	}
}
