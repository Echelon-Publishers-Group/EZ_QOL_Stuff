// $profile:EZ_QOL/DroneDelivery.json (written with defaults when missing)
// NOTE: "class" is a reserved word in Enforce Script, so the JSON key for an item class is "className".

class EZ_QOL_DroneConfigItem
{
	string className;
	int qty;
	float weight;

	void EZ_QOL_DroneConfigItem(string c = "", int q = 1, float w = 1)
	{
		className = c;
		qty = q;
		weight = w;
	}
}

class EZ_QOL_DroneConfigCategory
{
	string id;
	string name;
	string icon;
	int price;
	int cooldownSec;
	bool pickOne;      // true = surprise: pick ONE item by weight
	bool enabled;      // first slice: only "food" enabled by default
	ref array<ref EZ_QOL_DroneConfigItem> items;

	void EZ_QOL_DroneConfigCategory()
	{
		items = new array<ref EZ_QOL_DroneConfigItem>;
		enabled = true;
	}

	void Add(string c, int q, float w = 1)
	{
		items.Insert(new EZ_QOL_DroneConfigItem(c, q, w));
	}
}

class EZ_QOL_DroneConfig
{
	static const string DIR  = "$profile:EZ_QOL";
	static const string PATH = "$profile:EZ_QOL/DroneDelivery.json";
	protected static ref EZ_QOL_DroneConfig s_Instance;

	int version;
	bool enabled;
	float altitudeMin;
	float altitudeMax;
	float speed;               // m/s, still air
	float spawnDistance;       // m from target
	float hoverTime;           // s
	float windCancelMs;        // wind >= this refuses the order
	float windWarnMs;          // wind >= this shows the warning banner
	float windDriftFactor;     // drift m/s per m/s of wind
	float crashChancePerMs;    // per second, per m/s of wind
	bool refundOnCrash;        // refund the price if the drone crashes (launch failures are always refunded)
	bool crateOnCrash;
	int globalCooldownSec;     // per player, any category
	bool blockInSafezone;
	bool blockInTerritory;
	int CrateEmptyGraceSec;    // delete crate this long after it becomes empty
	int CrateMaxLifetimeSec;   // delete crate after this long regardless
	bool logging;
	bool requireTablet;        // player must hold or carry an EZ_QOL_Tablet
	ref array<ref EZ_QOL_DroneConfigCategory> categories;

	void EZ_QOL_DroneConfig()
	{
		categories = new array<ref EZ_QOL_DroneConfigCategory>;
	}

	static EZ_QOL_DroneConfig Get()
	{
		if (!s_Instance)
			Load();
		return s_Instance;
	}

	static void Load()
	{
		EZ_QOL_DroneConfig cfg = new EZ_QOL_DroneConfig();
		if (FileExist(PATH))
		{
			JsonFileLoader<EZ_QOL_DroneConfig>.JsonLoadFile(PATH, cfg);
			EZ_QOL_DroneLog.Info("Loaded " + PATH);
		}
		else
		{
			cfg.SetDefaults();
			if (!FileExist(DIR))
				MakeDirectory(DIR);
			JsonFileLoader<EZ_QOL_DroneConfig>.JsonSaveFile(PATH, cfg);
			EZ_QOL_DroneLog.Info("Wrote default " + PATH);
		}
		cfg.Sanitize();
		cfg.Validate();
		s_Instance = cfg;
		EZ_QOL_DroneLog.s_Enabled = cfg.logging;
	}

	static bool ClassExists(string cls)
	{
		if (cls == "")
			return false;
		if (GetGame().ConfigIsExisting("CfgVehicles " + cls))
			return true;
		if (GetGame().ConfigIsExisting("CfgMagazines " + cls))
			return true;
		if (GetGame().ConfigIsExisting("CfgWeapons " + cls))
			return true;
		return false;
	}

	void Sanitize()
	{
		if (altitudeMin <= 0) altitudeMin = 40;
		if (altitudeMax < altitudeMin) altitudeMax = altitudeMin;
		if (speed <= 0) speed = 15;
		if (spawnDistance <= 0) spawnDistance = 800;
		if (CrateEmptyGraceSec <= 0) CrateEmptyGraceSec = 30;
		if (CrateMaxLifetimeSec <= 0) CrateMaxLifetimeSec = 900;
		if (windCancelMs <= 0) windCancelMs = 18;
		if (windWarnMs <= 0) windWarnMs = windCancelMs * 0.66;
		// v0.11: the shipped category icons are .paa; migrate old profile JSONs that still point at the never-shipped .edds files.
		if (categories)
		{
			foreach (EZ_QOL_DroneConfigCategory sc : categories)
			{
				if (!sc)
					continue;
				string ic = sc.icon;
				if (ic.IndexOf("EZ_QOL_Stuff/gui/icons/") == 0 && ic.IndexOf(".edds") > 0)
				{
					ic.Replace(".edds", ".paa");
					sc.icon = ic;
				}
			}
		}
	}

	// Drops missing classes (warn) and disables categories left empty (warn).
	void Validate()
	{
		for (int c = 0; c < categories.Count(); c++)
		{
			EZ_QOL_DroneConfigCategory cat = categories[c];
			for (int i = cat.items.Count() - 1; i >= 0; i--)
			{
				if (!ClassExists(cat.items[i].className))
				{
					EZ_QOL_DroneLog.Warn("Category '" + cat.id + "': class '" + cat.items[i].className + "' not found, skipped");
					cat.items.Remove(i);
				}
			}
			if (cat.enabled && cat.items.Count() == 0)
			{
				EZ_QOL_DroneLog.Warn("Category '" + cat.id + "' has no valid items, hidden");
				cat.enabled = false;
			}
		}
	}

	EZ_QOL_DroneConfigCategory FindCategory(string id)
	{
		for (int c = 0; c < categories.Count(); c++)
		{
			if (categories[c].id == id)
				return categories[c];
		}
		return null;
	}

	void SetDefaults()
	{
		version = 1;
		enabled = true;
		altitudeMin = 40;
		altitudeMax = 60;
		speed = 15;
		spawnDistance = 800;
		hoverTime = 4;
		windCancelMs = 18;
		windWarnMs = 12;
		windDriftFactor = 0.15;
		crashChancePerMs = 0.00005;
		refundOnCrash = false;
		crateOnCrash = true;
		globalCooldownSec = 300;
		blockInSafezone = true;
		blockInTerritory = false;
		CrateEmptyGraceSec = 30;
		CrateMaxLifetimeSec = 900;
		logging = true;
		requireTablet = true;

		EZ_QOL_DroneConfigCategory med = new EZ_QOL_DroneConfigCategory();
		med.id = "medical"; med.name = "Medical / Surgery"; med.icon = "EZ_QOL_Stuff/gui/icons/medical.paa";
		med.price = 2500; med.cooldownSec = 900; med.enabled = true;
		// Terje Medicine (verified in TerjeBruoygard/TerjeMods TerjeMedicine/*/config.cpp)
		med.Add("TerjeSurgicalKit", 1);
		med.Add("TerjeBandageHemostatic", 2);
		med.Add("TerjeInjectorPromidol", 1);
		med.Add("TerjeAmpouleAmoxivan", 1);
		med.Add("TerjeSyringeNew", 1);
		med.Add("TerjePillsNimesulid", 1);
		med.Add("TerjeFirstAidKitIndividual_Green", 1);
		// vanilla fallbacks
		med.Add("BandageDressing", 4);
		med.Add("Morphine", 1);
		med.Add("Epinephrine", 1);
		med.Add("SalineBagIV", 1);
		med.Add("TetracyclineAntibiotics", 1);
		med.Add("PainkillerTablets", 1);
		med.Add("CharcoalTablets", 1);
		med.Add("SurgicalGloves_Blue", 1);
		categories.Insert(med);

		EZ_QOL_DroneConfigCategory ammo = new EZ_QOL_DroneConfigCategory();
		ammo.id = "ammo"; ammo.name = "Ammo Drop"; ammo.icon = "EZ_QOL_Stuff/gui/icons/ammo.paa";
		ammo.price = 3000; ammo.cooldownSec = 1200; ammo.enabled = true;
		ammo.Add("AmmoBox_556x45_20Rnd", 2);
		ammo.Add("AmmoBox_762x39_20Rnd", 2);
		ammo.Add("AmmoBox_9x19_25rnd", 2);
		ammo.Add("AmmoBox_12gaPellets_10Rnd", 1);
		ammo.Add("Ammo_308Win", 1);
		categories.Insert(ammo);

		EZ_QOL_DroneConfigCategory sur = new EZ_QOL_DroneConfigCategory();
		sur.id = "surprise"; sur.name = "Surprise"; sur.icon = "EZ_QOL_Stuff/gui/icons/surprise.paa";
		sur.price = 1500; sur.cooldownSec = 1800; sur.pickOne = true; sur.enabled = true;
		sur.Add("NVGoggles", 1, 1);
		sur.Add("M4A1", 1, 2);
		sur.Add("Binoculars", 1, 5);
		sur.Add("Rice", 2, 10);
		sur.Add("CanOpener", 1, 10);
		categories.Insert(sur);

		EZ_QOL_DroneConfigCategory food = new EZ_QOL_DroneConfigCategory();
		food.id = "food"; food.name = "Food & Water"; food.icon = "EZ_QOL_Stuff/gui/icons/food.paa";
		food.price = 800; food.cooldownSec = 600; food.enabled = true;
		food.Add("BakedBeansCan", 2);
		food.Add("SpaghettiCan", 1);
		food.Add("TacticalBaconCan", 1);
		food.Add("WaterBottle", 2);
		food.Add("Canteen", 1);
		food.Add("CanOpener", 1);
		categories.Insert(food);
	}
}
