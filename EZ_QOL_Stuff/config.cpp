// EZ_QOL_Stuff: garage (garage team) + drone delivery (ClearcREEk) + tablet shell.
// Coexistence: every class is EZ_QOL_ prefixed; no VGS (HideoutVirtualGarageSystem / GarageMod_*) class is referenced or overridden.
class CfgPatches
{
	class EZ_QOL_Stuff
	{
		units[] = {"EZ_QOL_GarageKit","EZ_QOL_Garage","EZ_QOL_Tablet","EZ_QOL_DroneCrate","EZ_QOL_AttackDrone","EZ_PineappleSeeds","EZ_PineappleSeedsPack","EZ_Plant_Pineapple","EZ_Pineapple","EZ_PineappleSlice","EZ_SleepingBag_Packed","EZ_SleepingBag_Deployed"};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] =
		{
			"DZ_Data",
			"DZ_Scripts",
			"DZ_Gear_Camping",
			"DZ_Gear_Food",                  // Pumpkin / SlicedPumpkin (pineapple bases)
			"DZ_Gear_Cultivation",           // SeedBase / TomatoSeedsPack / PlantBase
			"DZ_Gear_Tools",                 // SewingKit (sleeping bag repair)  [addon name seen in Expansion configs]
			"DZ_Weapons_Explosives",        // M18 smoke (EZ_QOL_DroneMarker) + Plastic_Explosive_Ammo (attack drone); DZ\weapons\explosives  [addon name verified]
			"JM_CF_Scripts",
			"DayZExpansion_Core_Scripts",
			"DayZExpansion_Vehicles_Scripts",
			"DayZExpansion_BaseBuilding_Scripts",
			"DayZExpansion_Market_Scripts"     // drone payments (EXPANSIONMODMARKET)
		};
	};
};

class CfgMods
{
	class EZ_QOL_Stuff
	{
		dir = "EZ_QOL_Stuff";
		name = "EZ_QOL_Stuff";
		author = "Echelon Publishers Group";
		inputs = "EZ_QOL_Stuff/data/inputs.xml";
		type = "mod";
		dependencies[] = {"Game","World","Mission"};
		class defs
		{
			class gameScriptModule    { value = ""; files[] = {"EZ_QOL_Stuff/scripts/3_Game"}; };
			class worldScriptModule   { value = ""; files[] = {"EZ_QOL_Stuff/scripts/4_World"}; };
			class missionScriptModule { value = ""; files[] = {"EZ_QOL_Stuff/scripts/5_Mission"}; };
		};
	};
};

class FruitStageTransitions;
class FoodAnimationSources;

class CfgVehicles
{
	class Inventory_Base;
	class HouseNoDestruct;
	class House;
	class Container_Base;
	class M18SmokeGrenade_Red;
	class SeedBase;
	class TomatoSeedsPack;
	class PlantBase;
	class Pumpkin;
	class SlicedPumpkin;

	// ---------------- garage (garage team)
	class EZ_QOL_GarageKit: Inventory_Base
	{
		scope = 2;
		displayName = "$STR_EZ_QOL_KIT_NAME";
		descriptionShort = "$STR_EZ_QOL_KIT_DESC";
		model = "EZ_QOL_Stuff\data\garage_kit\ez_qol_garagekit.p3d";
		weight = 15000;
		itemSize[] = {6, 4};
		itemBehaviour = 0;
		physLayer = "item_large";
		carveNavmesh = 1;
		canBeDigged = 0;
		heavyItem = 1;
		rotationFlags = 2;
		class DamageSystem
		{
			class GlobalHealth { class Health { hitpoints = 2000; }; };
		};
	};

	class EZ_QOL_Garage: Inventory_Base
	{
		scope = 2;
		displayName = "$STR_EZ_QOL_GARAGE_NAME";
		descriptionShort = "$STR_EZ_QOL_GARAGE_DESC";
		model = "EZ_QOL_Stuff\data\garage\ez_qol_garage.p3d";
		weight = 100000;
		itemSize[] = {20, 20};
		itemBehaviour = 0;
		physLayer = "item_large";
		carveNavmesh = 1;
		canBeDigged = 0;
		bounding = "BSphere";
		forceFarBubble = "true";
		attachments[] = {"Att_ExpansionCodeLock"};
		class GUIInventoryAttachmentsProps
		{
			class CodeLock
			{
				name = "$STR_EXPANSION_CODELOCK";   // Expansion stringtable key  [UNVERIFIED]
				description = "";
				attachmentSlots[] = {"Att_ExpansionCodeLock"};
				icon = "set:expansion_inventory image:codelock";   // [UNVERIFIED]
				view_index = 1;
			};
		};
		class DamageSystem
		{
			class GlobalHealth { class Health { hitpoints = 50000; }; };
		};
	};

	class EZ_QOL_GarageHologram: EZ_QOL_Garage
	{
		scope = 1;
		attachments[] = {};
		class GUIInventoryAttachmentsProps {};
	};

	// ---------------- tablet (shell: garage team)
	class EZ_QOL_Tablet: Inventory_Base
	{
		scope = 2;
		displayName = "$STR_EZ_QOL_TABLET";
		descriptionShort = "$STR_EZ_QOL_TABLET_DESC";
		model = "EZ_QOL_Stuff\data\tablet\ez_qol_tablet.p3d";
		itemSize[] = {2, 2};
		weight = 600;
		hiddenSelections[] = {"screen"};
		hiddenSelectionsTextures[] = {"EZ_QOL_Stuff\data\tablet\textures\ez_qol_tablet_screen_co.paa"};
		hiddenSelectionsMaterials[] = {"EZ_QOL_Stuff\data\tablet\ez_qol_tablet_screen.rvmat"};
	};

	// ---------------- drone (ClearcREEk, merged from DRONE_CONFIG_CPP_REQUESTS.md)
	class EZ_QOL_DeliveryDrone: HouseNoDestruct
	{
		scope = 1;
		model = "EZ_QOL_Stuff\data\drone\ez_qol_drone.p3d";
		class AnimationSources
		{
			class rotor          { source = "user"; animPeriod = 0.1;  initPhase = 0; };
			class cargo_attached { source = "user"; animPeriod = 0.01; initPhase = 0; }; // 0 = cargo visible, 1 = hidden (model.cfg hideValue 0.5)
		};
	};

	// ---------------- attack drone (ATTACKDRONE_CONFIG_CPP_REQUESTS.md). Damageable base (House), NOT EZ_QOL_DeliveryDrone (HouseNoDestruct).
	class EZ_QOL_AttackDrone: House
	{
		scope = 1;
		model = "EZ_QOL_Stuff\data\drone\ez_qol_attackdrone.p3d";
		hiddenSelections[] = {"camo"};
		hiddenSelectionsTextures[] = {"EZ_QOL_Stuff\data\drone\textures\ez_qol_attackdrone_co.paa"};
		class AnimationSources
		{
			class rotor          { source = "user"; animPeriod = 0.1;  initPhase = 0; };
			class cargo_attached { source = "user"; animPeriod = 0.01; initPhase = 1; }; // cargo hidden
		};
		class DamageSystem
		{
			class GlobalHealth { class Health { hitpoints = 60; }; }; // must match droneHealth in AttackDrone.json
		};
	};

	class EZ_QOL_DroneCrate: Container_Base
	{
		scope = 2;
		displayName = "Drone Delivery Crate";
		descriptionShort = "Supply crate dropped by a delivery drone.";
		model = "EZ_QOL_Stuff\data\drone\ez_qol_drone.p3d"; // PLACEHOLDER until a crate model exists
		weight = 5000;
		itemSize[] = {10, 10};
		itemBehaviour = 0;
		physLayer = "item_large";
		carveNavmesh = 0;
		canBeDigged = 0;
		class Cargo { itemsCargoSize[] = {10, 8}; openable = 0; allowOwnedCargoManipulation = 1; };
	};

	class EZ_QOL_DroneMarker: M18SmokeGrenade_Red
	{
		scope = 1;
		displayName = "Drone Marker Smoke";
	};

	// ---------------- pineapple farming (moved from EZLoderScreen; item classnames unchanged)
	class EZ_PineappleSeeds: SeedBase {
		scope = 2;
		displayName = "Pineapple Seeds";
		descriptionShort = "A handful of pineapple seeds. Plant them in a garden plot or greenhouse.";
		model = "\dz\gear\cultivation\pepper_seeds.p3d";
		weight = 0;
		weightPerQuantityUnit = 1;
		itemSize[] = {1, 1};
		varQuantityInit = 10;
		varQuantityMin = 0;
		varQuantityMax = 10;
		canBeSplit = 1;
		class Horticulture {
			PlantType = "EZ_Plant_Pineapple";
		};
	};
	class EZ_PineappleSeedsPack: TomatoSeedsPack {
		scope = 2;
		displayName = "Pineapple Seeds Pack";
		descriptionShort = "A pack of pineapple seeds. Open it to get the seeds.";
		model = "\dz\gear\cultivation\tomato_seeds_pack.p3d";
		weight = 3;
		itemSize[] = {1, 1};
		class Horticulture {
			ContainsSeedsType = "EZ_PineappleSeeds";
			ContainsSeedsQuantity = 10;
		};
	};
	class EZ_Plant_Pineapple: PlantBase {
		scope = 2;
		displayName = "Pineapple plant";
		model = "\EZ_QOL_Stuff\data\pineapples\ez_pineapple_plant.p3d";
		class Horticulture {
			GrowthStagesCount = 6;
			CropsCount = 2;
			CropsType = "EZ_Pineapple";
		};
	};
	class EZ_Pineapple: Pumpkin {
		scope = 2;
		displayName = "Pineapple";
		descriptionShort = "A ripe, juicy pineapple. Sweet, filling and full of water.";
		model = "\EZ_QOL_Stuff\data\pineapples\ez_pineapple.p3d";
		rotationFlags = 34;
		weight = 0;
		weightPerQuantityUnit = 1;
		itemSize[] = {2, 3};
		stackedUnit = "g";
		quantityBar = 1;
		varQuantityInit = 250;
		varQuantityMin = 0;
		varQuantityMax = 250;
		absorbency = 0.2;
		inventorySlot[] = {};
		containsSeedsType = "EZ_PineappleSeeds";
		containsSeedsQuantity = "2";
		class DamageSystem {
			class GlobalHealth {
				class Health {
					hitpoints = 100;
					healthLevels[] = {1, "EZ_QOL_Stuff\data\pineapples\ez_pineapple.rvmat", 0.7, "EZ_QOL_Stuff\data\pineapples\ez_pineapple.rvmat", 0.5, "EZ_QOL_Stuff\data\pineapples\ez_pineapple.rvmat", 0.3, "EZ_QOL_Stuff\data\pineapples\ez_pineapple.rvmat", 0, "EZ_QOL_Stuff\data\pineapples\ez_pineapple.rvmat"};
				};
			};
		};
		class AnimationSources: FoodAnimationSources {};
		class Food {
			class FoodStages {
				class Raw {
					visual_properties[] = {-1, -1, -1};
					nutrition_properties[] = {2.5, 60, 85, 1, 0};
					cooking_properties[] = {0, 0};
				};
				class Rotten {
					visual_properties[] = {-1, -1, -1};
					nutrition_properties[] = {2, 30, 45, 1, 0, 16};
					cooking_properties[] = {0, 0};
				};
				class Baked {
					visual_properties[] = {-1, -1, -1};
					nutrition_properties[] = {1.75, 90, 50, 1, 0};
					cooking_properties[] = {70, 35};
				};
				class Boiled {
					visual_properties[] = {-1, -1, -1};
					nutrition_properties[] = {1.5, 70, 85, 1, 0};
					cooking_properties[] = {70, 45};
				};
				class Dried {
					visual_properties[] = {-1, -1, -1};
					nutrition_properties[] = {0.75, 110, 5, 1, 0};
					cooking_properties[] = {70, 30, 80};
				};
				class Burned {
					visual_properties[] = {-1, -1, -1};
					nutrition_properties[] = {2, 20, 5, 1, 0, 16};
					cooking_properties[] = {100, 30};
				};
			};
			class FoodStageTransitions: FruitStageTransitions {};
		};
		class NoiseImpact {
			strength = 600;
			type = "sound";
		};
		class AnimEvents {
			class SoundWeapon {
				class openTunaCan {
					soundSet = "openTunaCan_SoundSet";
					id = 204;
				};
				class pickUpItem {
					soundSet = "pickUpPotato_SoundSet";
					id = 797;
				};
				class Eating_TakeFood {
					soundSet = "Eating_TakeFood_Soundset";
					id = 889;
				};
				class Eating_BoxOpen {
					soundSet = "Eating_BoxOpen_Soundset";
					id = 893;
				};
				class Eating_BoxShake {
					soundSet = "Eating_BoxShake_Soundset";
					id = 894;
				};
				class Eating_BoxEnd {
					soundSet = "Eating_BoxEnd_Soundset";
					id = 895;
				};
				class drop {
					soundset = "apple_drop_SoundSet";
					id = 898;
				};
			};
		};
	};
	class EZ_PineappleSlice: SlicedPumpkin {
		scope = 2;
		displayName = "Pineapple Slice";
		descriptionShort = "A quarter of a pineapple. Sweet and juicy, can be eaten raw or cooked.";
		model = "\EZ_QOL_Stuff\data\pineapples\ez_pineapple_slice.p3d";
		hiddenSelections[] = {};
		hiddenSelectionsTextures[] = {};
		varQuantityInit = 63;
		varQuantityMin = 0;
		varQuantityMax = 63;
		class Food {
			class FoodStages {
				class Raw {
					visual_properties[] = {0, 0, 0};
					nutrition_properties[] = {2.5, 60, 85, 1, 0};
					cooking_properties[] = {0, 0};
				};
				class Rotten {
					visual_properties[] = {-1, -1, 5};
					nutrition_properties[] = {2, 30, 45, 1, 0, 16};
					cooking_properties[] = {0, 0};
				};
				class Baked {
					visual_properties[] = {0, 1, 1};
					nutrition_properties[] = {1.75, 90, 50, 1, 0};
					cooking_properties[] = {70, 35};
				};
				class Boiled {
					visual_properties[] = {0, 2, 2};
					nutrition_properties[] = {1.5, 70, 85, 1, 0};
					cooking_properties[] = {70, 45};
				};
				class Dried {
					visual_properties[] = {0, 1, 3};
					nutrition_properties[] = {0.75, 110, 5, 1, 0};
					cooking_properties[] = {70, 30, 80};
				};
				class Burned {
					visual_properties[] = {0, 1, 4};
					nutrition_properties[] = {2, 20, 5, 1, 0, 16};
					cooking_properties[] = {100, 30};
				};
			};
			class FoodStageTransitions: FruitStageTransitions {};
		};
	};

	// ---------------- sleeping bag (placeholder vanilla models until the SLEEP BAG p3d is done; see SLEEPINGBAG_P3D_REQUESTS.md)
	class EZ_SleepingBag_Packed: Inventory_Base
	{
		scope = 2;
		displayName = "$STR_EZ_QOL_BAG_PACKED";
		descriptionShort = "$STR_EZ_QOL_BAG_PACKED_DESC";
		model = "\DZ\gear\camping\camo_net_p.p3d";          // PLACEHOLDER
		projectionTypename = "EZ_SleepingBag_PackedPlacing";
		weight = 2500;
		itemSize[] = {3, 4};
		itemBehaviour = 0;
		rotationFlags = 2;
		class DamageSystem
		{
			class GlobalHealth { class Health { hitpoints = 100; }; };
		};
	};
	class EZ_SleepingBag_PackedPlacing: Inventory_Base
	{
		scope = 1;
		model = "\DZ\gear\camping\underground_stash.p3d";   // PLACEHOLDER, same as deployed
		storageCategory = 10;
		hiddenSelections[] = {};
	};
	class EZ_SleepingBag_Deployed: Container_Base
	{
		scope = 2;
		displayName = "$STR_EZ_QOL_BAG_NAME";
		descriptionShort = "$STR_EZ_QOL_BAG_DESC";
		model = "\DZ\gear\camping\underground_stash.p3d";   // PLACEHOLDER
		weight = 2500;
		itemSize[] = {10, 10};
		itemBehaviour = 0;
		physLayer = "item_large";
		carveNavmesh = 0;
		canBeDigged = 0;
		class Cargo
		{
			itemsCargoSize[] = {10, 10};
			openable = 0;
			allowOwnedCargoManipulation = 1;
		};
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 100;
				};
			};
		};
	};
};

class CfgSoundShaders
{
	class EZ_QOL_DroneLoop_SoundShader
	{
		samples[] = {{"EZ_QOL_Stuff\data\sounds\drone_loop", 1}};   // data/sounds/drone_loop.ogg: FILE STILL NEEDS CREATING
		volume = 0.8;
		range = 300;
		limitation = 0;
	};
	class EZ_QOL_DroneTakeoff_SoundShader
	{
		samples[] = {{"EZ_QOL_Stuff\data\sounds\drone_takeoff", 1}}; // data/sounds/drone_takeoff.ogg: FILE STILL NEEDS CREATING
		volume = 1;
		range = 300;
	};
};

class CfgSoundSets
{
	class EZ_QOL_DroneLoop_SoundSet
	{
		soundShaders[] = {"EZ_QOL_DroneLoop_SoundShader"};
		volumeFactor = 1;
		frequencyFactor = 1;
		spatial = 1;
		loop = 1;
		sound3DProcessingType = "infantryFootsteps3DProcessingType";
		distanceFilter = "defaultDistanceFilter";
		volumeCurve = "InverseSquare2Curve";
	};
	class EZ_QOL_DroneTakeoff_SoundSet: EZ_QOL_DroneLoop_SoundSet
	{
		soundShaders[] = {"EZ_QOL_DroneTakeoff_SoundShader"};
		loop = 0;
	};
};
