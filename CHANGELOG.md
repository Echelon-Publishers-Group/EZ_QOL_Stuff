# Changelog

## V1.0 beta
By Robert J. Morris (LordSausageFinger), Echelon Publishers Group.
Steam Workshop: https://steamcommunity.com/sharedfiles/filedetails/?id=3830414136
### Added
- Pineapple farming, moved from EZLoderScreen. The seeds, plant, fruit and slices keep their classnames.
- Vehicle flip: hold F on an overturned, empty, engine-off vehicle, or press F6 (with an optional confirm press).
- Earplugs with volume levels, an on/off toggle and a HUD indicator. New client settings menu (Home key, plus a button in the Esc menu) and an "EZ_QOL" keybind group: Flip F6, Earplugs End, Settings Home.
- Sleeping bag: place it to set your respawn point, one per survivor. Pack, claim and sewing-kit repair actions. Respawn cooldown, damage per respawn, decay, and zone and enemy-proximity checks, configurable in `$profile:EZ_QOL/SleepingBagConfig.json`. Vanilla placeholder models for now.
- Four drone delivery category icons (gui/icons/*.paa).
- Optional, separate add-on `optional/EZ_QOL_ExpansionCompat` (not part of the mod) bridging the sleeping bag to Expansion's spawn select.

## V0.10 (Strike Package)
### Added
- Attack drone: a lock-on strike bought from the tablet against player, vehicle, base or ground targets, with target warnings, wind/sandstorm cancel, cooldowns and a partial refund on abort (`AttackDrone.sample.json`).
- PvE/safe-zone gate: strikes are blocked in Ninjin PvP/PvE PvE and safe zones (via Ninjin's `Zones.json`), Expansion safezones and a configurable buffer.

## V0.9.1
### Changed
- Tablet UI re-laid out and centred on screen.

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
