# Changelog

## [0.2]: 2026-10 (Steam Workshop release)
### Added
- Drone delivery service with four categories: surgery & medical (Terje Medicine), ammo drops, surprise delivery, food & water.
- Delivery crates despawn once emptied.
- In-game tablet with drone and garage apps.
- Portable garage kit using Expansion's storage API: pack a vehicle with its cargo, replant it elsewhere, code-lock it. Packed kits can be stolen.
- Drone payments via Expansion Market.
- Coexistence with Virtual Garage System: separate RPC names and storage folder.
- Sample configs: `DroneDelivery.sample.json`, `types_drone_snippet.xml`.
### Known / to verify
- Verified `DZ_Weapons_Explosives` as the correct base-game addon for the M18 smoke marker (it is the CfgPatches of the vanilla weapons/explosives config).
- Models ship as MLOD; check the drone's front orientation in game.
