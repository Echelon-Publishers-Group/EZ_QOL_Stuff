# EZ_QOL_Stuff – P3D models (Object Builder stage done on Linux)

## What was done
- `data/drone/ez_qol_drone.p3d` and `data/tablet/ez_qol_tablet.p3d`: **MLOD (unbinarized) P3Ds**, written with the P3D data module from the Arma 3 Object Builder Blender add-on v2.5.1 (driven by `source_tools/build.py`) and re-read by both `verify.py` and the add-on's own importer.
- LODs per model: Resolution 1 (LOD 0, full), 2 (LOD 1, decimated), 4 (LOD 2, decimated more), View Geometry, Geometry (convex `ComponentXX`, mass, `autocenter=0`), Fire Geometry, Memory.
- Named selections and memory points follow OBJECT_BUILDER_STEPS.md (plus `camo` = body+cargo / body; `rotor_N_axis` = begin+end vertices in one selection, as the model.cfg needs).
- Drone: origin moved to the bounding-box centre (offset 0, -0.102, +0.0078 m); every LOD and memory point moved with it. `light_pos` is on the round emissive lamp lens on the -Y end of the hull; `light_dir` is 10 cm out along its facing (-Y).
  NOTE: that lamp faces -Y, the side the doc called "rear". Check which way is the front in game.
- Tablet: scaled uniformly ×2.594 to 25.0 × 17.5 × 1.7 cm and centred on its bounding box. `screen` (2 tris) is its own selection, with its own texture and rvmat. `invview` is 35 cm in front of the screen. Mass is 1.0 kg (the doc's 0.6 kg was for the small source model).
- `data/model.cfg`: drone skeleton, rotor_1..4 rotation animations and cargo hide (from the doc), plus the tablet's `screen` section.
- Textures: the .paa files from the drop (DXT PAA, already converted) plus simple Super-shader rvmats.

## Left for the maintainer
- **Not binarized.** No Wine or DayZ Tools on the box. Addon Builder / pboProject will binarize these MLODs when you pack them (or ship them as MLOD).
- Open each model once in Object Builder: Tools ▸ Check, and look at normals and smoothing (sharp edges are not exported, so smoothing comes from the stored vertex normals).
- Do an in-game check: scale, rotor spin direction, light position and direction, tablet screen hiddenSelection swap, and geometry and collision. Fix the drone's front if needed.
- Write the config.cpp (AnimationSources rotor/cargo_attached, hiddenSelections) and sign the PBO yourself.

## v1.0 beta additions (moved from EZLoderScreen + new)
- **Pineapple farming** (`data/pineapples/`, `scripts/4_World/EZ_QOL/Farming/`). The item classnames are unchanged (EZ_PineappleSeeds, EZ_PineappleSeedsPack, EZ_Plant_Pineapple, EZ_Pineapple, EZ_PineappleSlice), so existing types.xml entries and stored items keep working. EZLoderScreen's "sticky hands" effect is no longer triggered by slicing.
- **Vehicle flip**: a hold-F action on overturned, empty, engine-off vehicles, plus the F6 keybind (`UAEZQOLFlip`).
- **Client settings, earplugs and keybinds**: input group "EZ_QOL" with UAEZQOLFlip (F6), UAEZQOLEarplugs (End) and UAEZQOLSettings (Home). The settings menu covers: earplugs enabled, earplug levels, earplug HUD, and flip confirm. Saved to `$profile:EZ_QOL_client.json` on the client. An "EZ QOL settings" button sits in the in-game menu.
- **Sleeping bag** (EZ_SleepingBag_Packed -> EZ_SleepingBag_Deployed):
  - Placing a bag binds it to you (max 1).
  - You respawn at your bag as a fresh spawn. You can optionally be moved to it on an alive login.
  - Rules and defaults are in `$profile:EZ_QOL/SleepingBagConfig.json`, written on first start; see `SleepingBagConfig.sample.json`. The registry is `$profile:EZ_QOL/SleepingBagRegistry.json`.
  - Pack, claim and repair (sewing kit) actions.
  - `types_sleepingbag_snippet.xml` has the CE entries. The models are vanilla placeholders; see SLEEPINGBAG_P3D_REQUESTS.md.
  - The Expansion spawn-select bridge is optional: it is a separate addon in `optional/EZ_QOL_ExpansionCompat` and is not part of this PBO.
