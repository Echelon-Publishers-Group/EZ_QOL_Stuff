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
