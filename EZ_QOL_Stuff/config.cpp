// EZ_QOL_Stuff: garage (garage team) + drone delivery (ClearcREEk) + tablet shell.
// Coexistence: every class is EZ_QOL_ prefixed; no VGS (HideoutVirtualGarageSystem / GarageMod_*) class is referenced or overridden.
class CfgPatches
{
	class EZ_QOL_Stuff
	{
		units[] = {"EZ_QOL_GarageKit","EZ_QOL_Garage","EZ_QOL_Tablet","EZ_QOL_DroneCrate"};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] =
		{
			"DZ_Data",
			"DZ_Scripts",
			"DZ_Gear_Camping",
			"DZ_Weapons_Explosives",        // M18 smoke for EZ_QOL_DroneMarker
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
		author = "EchelonZ";
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

class CfgVehicles
{
	class Inventory_Base;
	class HouseNoDestruct;
	class Container_Base;
	class M18SmokeGrenade_Red;

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
