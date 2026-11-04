// Sleeping bag: server config ($profile:EZ_QOL/SleepingBagConfig.json) + registry data
// ($profile:EZ_QOL/SleepingBagRegistry.json). Vanilla only: no CF, no Expansion, no Terje/Zen.
class EZ_QOL_SleepingBagConfig
{
	protected static ref EZ_QOL_SleepingBagConfig s_Instance;
	static const string PATH = "$profile:EZ_QOL/SleepingBagConfig.json";

	int Enabled = 1;
	int MaxBagsPerPlayer = 1;            // only 1 is supported: a new bag moves the binding
	float HealthLossPerRespawn = 20;     // hitpoints taken from the bag per respawn on it
	float DecayPerHour = 0.5;            // hitpoints lost per real hour while deployed
	int AllowOthersLoot = 1;
	int AllowOthersPackOrDestroy = 1;
	int RepairEnabled = 1;
	int EnableFreshSpawn = 1;            // new character spawns at the bag
	int EnableAliveLoginSpawn = 1;       // living character is moved to the bag on login
	int ValidateBagExists = 1;
	float BagValidationRadius = 1.5;
	float AlreadyAtBagRadius = 5.0;
	float SpawnHeightOffset = 0.05;
	int EnableGenericPackActionHook = 1;
	int OwnerNameTags = 1;
	int RespawnCooldownMinutes = 30;
	float EnemyBlockRadius = 150;
	int AllowInPvEZones = 1;
	int AllowInSafeZones = 0;
	float SpawnHealthPercent = 100;

	static EZ_QOL_SleepingBagConfig Get()
	{
		if (!s_Instance)
			Load();
		return s_Instance;
	}

	static void Load()
	{
		EZ_QOL_Log.EnsureDirs();
		EZ_QOL_SleepingBagConfig cfg = new EZ_QOL_SleepingBagConfig();
		if (FileExist(PATH))
			JsonFileLoader<EZ_QOL_SleepingBagConfig>.JsonLoadFile(PATH, cfg);
		cfg.Sanitize();
		JsonFileLoader<EZ_QOL_SleepingBagConfig>.JsonSaveFile(PATH, cfg); // writes defaults / new keys
		s_Instance = cfg;
	}

	void Sanitize()
	{
		if (MaxBagsPerPlayer != 1)
			MaxBagsPerPlayer = 1;
		if (HealthLossPerRespawn < 0)
			HealthLossPerRespawn = 0;
		if (DecayPerHour < 0)
			DecayPerHour = 0;
		if (RespawnCooldownMinutes < 0)
			RespawnCooldownMinutes = 0;
		if (BagValidationRadius < 0.5)
			BagValidationRadius = 0.5;
		SpawnHealthPercent = Math.Clamp(SpawnHealthPercent, 1, 100);
	}
}

class EZ_QOL_SleepingBagRecord
{
	string OwnerUID;
	string OwnerName;
	string BagKey;        // "" = no bag bound (record kept for the cooldown)
	vector Pos;
	int LastRespawn;      // EZ_QOL_Time.Now() of the last respawn at a bag
}

class EZ_QOL_SleepingBagRegistryFile
{
	int Version = 1;
	ref array<ref EZ_QOL_SleepingBagRecord> Bags;

	void EZ_QOL_SleepingBagRegistryFile()
	{
		Bags = new array<ref EZ_QOL_SleepingBagRecord>;
	}
}

// Seconds since 1970-01-01 UTC from the system clock (survives restarts, unlike GetTime()).
class EZ_QOL_Time
{
	static int Now()
	{
		int y;
		int mo;
		int d;
		int h;
		int mi;
		int s;
		GetYearMonthDayUTC(y, mo, d);
		GetHourMinuteSecondUTC(h, mi, s);
		return DaysFromCivil(y, mo, d) * 86400 + h * 3600 + mi * 60 + s;
	}

	// Howard Hinnant's days_from_civil
	static int DaysFromCivil(int y, int m, int d)
	{
		if (m <= 2)
			y = y - 1;
		int era = y / 400;
		if (y < 0)
			era = (y - 399) / 400;
		int yoe = y - era * 400;
		int mp = m + 9;
		if (m > 2)
			mp = m - 3;
		int doy = (153 * mp + 2) / 5 + d - 1;
		int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
		return era * 146097 + doe - 719468;
	}
}
