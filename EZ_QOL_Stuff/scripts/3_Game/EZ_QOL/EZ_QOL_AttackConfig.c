// $profile:EZ_QOL/AttackDrone.json (written with defaults when missing). Server only.
// Base damage is always on (no raid-hours option, by design).
class EZ_QOL_AttackConfig
{
	static const string DIR  = "$profile:EZ_QOL";
	static const string PATH = "$profile:EZ_QOL/AttackDrone.json";
	protected static ref EZ_QOL_AttackConfig s_Instance;

	int version;
	bool enabled;
	int price;
	int cooldownSec;          // per player
	int serverCooldownSec;    // global, between launches
	int maxActivePerPlayer;
	int maxActiveServer;
	float armWindowSec;       // pre-lock window after Arm
	float lockAcquireSec;
	float graceSec;
	float graceRefillPerSec;
	float coneDeg;
	float groundPointToleranceM;
	float maxRangeM;
	float minRangeM;
	string ammoType;
	bool allowPlayerTargets;
	bool allowVehicleTargets;
	bool allowBaseTargets;
	bool allowGroundTargets;
	string pvpSource;         // "zonesFile" (default), "ninjin" (needs optional compat), "none"
	string ninjinZonesPath;
	ref array<int> blockZoneTypes;
	float safezoneBufferM;
	float droneHealth;        // must match config.cpp DamageSystem hitpoints
	bool warnTarget;
	float warnLeadSec;
	float warnRadiusM;        // players this close to the target point also get the warning
	float windCancelMs;
	bool groundInSandstorm;   // sandstorm = wind >= windCancelMs (same proxy as deliveries)
	float spawnDistanceMin;
	float spawnDistanceMax;
	float cruiseAltitude;
	float speed;              // m/s cruise
	float diveStartM;         // horizontal distance where the dive starts
	float diveSpeedMult;
	float detonateDistanceM;
	float timeoutSec;
	int abortRefundPct;
	bool logging;
	bool requireTablet;

	void EZ_QOL_AttackConfig()
	{
		blockZoneTypes = new array<int>;
	}

	static EZ_QOL_AttackConfig Get()
	{
		if (!s_Instance)
			Load();
		return s_Instance;
	}

	static void Load()
	{
		EZ_QOL_AttackConfig cfg = new EZ_QOL_AttackConfig();
		cfg.SetDefaults();
		if (FileExist(PATH))
		{
			JsonFileLoader<EZ_QOL_AttackConfig>.JsonLoadFile(PATH, cfg);
			EZ_QOL_DroneLog.Info("Loaded " + PATH);
		}
		else
		{
			if (!FileExist(DIR))
				MakeDirectory(DIR);
			JsonFileLoader<EZ_QOL_AttackConfig>.JsonSaveFile(PATH, cfg);
			EZ_QOL_DroneLog.Info("Wrote default " + PATH);
		}
		cfg.Sanitize();
		s_Instance = cfg;
	}

	void Sanitize()
	{
		if (price < 0) price = 0;
		if (maxActivePerPlayer < 1) maxActivePerPlayer = 1;
		if (maxActiveServer < 1) maxActiveServer = 1;
		if (armWindowSec <= 0) armWindowSec = 10;
		if (lockAcquireSec <= 0) lockAcquireSec = 3;
		if (graceSec < 0) graceSec = 0;
		if (graceRefillPerSec < 0) graceRefillPerSec = 0;
		if (coneDeg <= 0) coneDeg = 2.5;
		if (groundPointToleranceM <= 0) groundPointToleranceM = 3;
		if (maxRangeM <= 0) maxRangeM = 400;
		if (minRangeM < 0) minRangeM = 0;
		if (ammoType == "") ammoType = "Plastic_Explosive_Ammo";
		if (pvpSource == "") pvpSource = "zonesFile";
		if (!blockZoneTypes) blockZoneTypes = new array<int>;
		if (blockZoneTypes.Count() == 0)
		{
			blockZoneTypes.Insert(2);
			blockZoneTypes.Insert(5);
		}
		if (droneHealth <= 0) droneHealth = 60;
		if (warnLeadSec < 0) warnLeadSec = 0;
		if (windCancelMs <= 0) windCancelMs = 18;
		if (spawnDistanceMin <= 0) spawnDistanceMin = 600;
		if (spawnDistanceMax < spawnDistanceMin) spawnDistanceMax = spawnDistanceMin;
		if (cruiseAltitude <= 0) cruiseAltitude = 50;
		if (speed <= 0) speed = 22;
		if (diveStartM <= 0) diveStartM = 120;
		if (diveSpeedMult <= 0) diveSpeedMult = 1.5;
		if (detonateDistanceM <= 0) detonateDistanceM = 2;
		if (timeoutSec <= 0) timeoutSec = 90;
		if (abortRefundPct < 0) abortRefundPct = 0;
		if (abortRefundPct > 100) abortRefundPct = 100;
	}

	bool IsBlockedZoneType(int t)
	{
		return blockZoneTypes.Find(t) >= 0;
	}

	void SetDefaults()
	{
		version = 1;
		enabled = true;
		price = 150000;
		cooldownSec = 3600;
		serverCooldownSec = 300;
		maxActivePerPlayer = 1;
		maxActiveServer = 2;
		armWindowSec = 10;
		lockAcquireSec = 3;
		graceSec = 1.5;
		graceRefillPerSec = 0.5;
		coneDeg = 2.5;
		groundPointToleranceM = 3;
		maxRangeM = 400;
		minRangeM = 40;
		ammoType = "Plastic_Explosive_Ammo";
		allowPlayerTargets = true;
		allowVehicleTargets = true;
		allowBaseTargets = true;
		allowGroundTargets = true;
		pvpSource = "zonesFile";
		ninjinZonesPath = "$profile:NinjinsPvPPvE/Config/Zones.json";
		blockZoneTypes = new array<int>;
		blockZoneTypes.Insert(2);
		blockZoneTypes.Insert(5);
		safezoneBufferM = 150;
		droneHealth = 60;
		warnTarget = true;
		warnLeadSec = 8;
		warnRadiusM = 15;
		windCancelMs = 18;
		groundInSandstorm = true;
		spawnDistanceMin = 600;
		spawnDistanceMax = 900;
		cruiseAltitude = 50;
		speed = 22;
		diveStartM = 120;
		diveSpeedMult = 1.5;
		detonateDistanceM = 2;
		timeoutSec = 90;
		abortRefundPct = 50;
		logging = true;
		requireTablet = true;
	}
}
