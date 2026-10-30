<p align="center">
  <img src="EZ_QOL_Stuff/logo.png" alt="EZ QOL Stuff logo" width="256">
</p>

# EZ_QOL_Stuff

**Quality-of-life additions for DayZ community servers: drone deliveries and a portable garage kit.**
Version **0.2** · by **Echelon Publishers Group**

<p align="center">
  <img src="EZ_QOL_Stuff/workshop_preview.jpg" alt="EZ QOL Stuff preview" width="512">
</p>

Steam Workshop: https://steamcommunity.com/sharedfiles/filedetails/?id=3830414136
Running live on the **EchelonZ** community server (https://echelonz.org).

---

## Features
1. **Drone delivery service.** Order from the in-game tablet in four categories:
   - **Surgery & medical** (Terje Medicine items)
   - **Ammo drops**
   - **Surprise delivery**
   - **Food & water**

   A drone flies the crate in and drops it. **The crate despawns once it's emptied.**
2. **Portable garage kit.** Built on DayZ Expansion's storage API:
   - **Pack a vehicle with its cargo inside**, carry the kit, and **replant it somewhere else**.
   - **Lock it with a code lock.**
   - **It can be stolen while packed.** Guard your kit.
3. **Coexists with Virtual Garage System (VGS).** It uses its own RPC names (`EZ_QOL_Garage_*`) and its own storage folder (`$profile:EZ_QOL/Garage`), and never reads VGS data.

## Requirements and dependencies
From `config.cpp` `requiredAddons[]`:
| Addon | Provided by |
|---|---|
| `DZ_Data`, `DZ_Scripts`, `DZ_Gear_Camping`, `DZ_Weapons_Explosives` | The base game (the last one is for the M18 smoke used by the drone marker) |
| `JM_CF_Scripts` | **Community Framework (CF)** |
| `DayZExpansion_Core_Scripts` | **DayZ Expansion** Core |
| `DayZExpansion_Vehicles_Scripts` | **DayZ Expansion** Vehicles |
| `DayZExpansion_BaseBuilding_Scripts` | **DayZ Expansion** BaseBuilding (code lock) |
| `DayZExpansion_Market_Scripts` | **DayZ Expansion** Market (drone payments) |

Medical deliveries use **Terje Medicine** item classes (e.g. `TerjeSurgicalKit`). Install TerjeMedicine, or edit the medical category in the config.

## Install (server owners)
1. Subscribe to the Workshop item, or copy `@EZ_QOL_Stuff` into the server folder.
2. Load order: `-mod=@CF;@DayZ-Expansion-Core;@DayZ-Expansion-Vehicles;@DayZ-Expansion-BaseBuilding;@DayZ-Expansion-Market;[@TerjeMedicine;]@EZ_QOL_Stuff` (match your mod folder names).
3. Copy the mod's `keys/*.bikey` (shipped with the Workshop download) into your server's `keys/` folder.
4. Merge `EZ_QOL_Stuff/types_drone_snippet.xml` into your mission's `db/types.xml`.
5. Start the server once. Then edit the generated config under `$profile:EZ_QOL/`, using `EZ_QOL_Stuff/DroneDelivery.sample.json` as a reference (zones, prices, categories, altitude).
6. Restart and test: open the tablet, order a drone, deploy a garage kit.

## Building from source
- Pack `EZ_QOL_Stuff/` with Addon Builder or pboProject and **sign it with your own key**. Keys and signed or packed files are never committed.
- The models are MLOD (unbinarized); see `docs/MODEL_NOTES.md`.

## Compatibility
- Works alongside **Virtual Garage System**. There's no shared data, and the hologram and placement hooks apply only to `EZ_QOL_GarageKit`.
- Requires the Expansion modules listed above. Other garage or storage mods that override the same vanilla actions are untested.
- Built for DayZ private community servers (modded).

## Credits
- **"Delivery drone Gameready (Free)"** by **Egor Likhachev**, CC BY 4.0, https://sketchfab.com/3d-models/delivery-drone-gameready-free-c22b1060860e47b4adbc1e9007b70b18 (modified: converted for DayZ, split meshes, retextured)
- **"Military Rugged PDA / Tactical Tablet"** by **Snupp**, CC BY 4.0, https://sketchfab.com/3d-models/military-rugged-pda-tactical-tablet-24b5e19dcb9e4970b5c211787c7f3fd0 (modified as above)
- **"Cloth Hangar"** by **CommonSpence**, CC BY 4.0, https://sketchfab.com/3d-models/cloth-hangar-dd53c69aa57b4c048d32363149b9966c (the base of the garage model; rescaled, LODs, collision and memory points added)
- Full notices are in `EZ_QOL_Stuff/CREDITS.txt`. CC BY 4.0: https://creativecommons.org/licenses/by/4.0/
- Built for DayZ. DayZ is a trademark of Bohemia Interactive a.s.; this project isn't affiliated with or endorsed by Bohemia Interactive. Mod content follows Bohemia's DayZ modding licence terms.

## License

Original work is licensed under [CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/): share and adapt it with credit to Echelon Publishers Group, non-commercially, under the same license. The credited 3D models stay under CC BY 4.0. See [LICENSE](LICENSE).
