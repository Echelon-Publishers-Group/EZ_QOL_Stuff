# Sleeping bag model requests (for Ewan)

Base asset: "SLEEP BAG" by Igor Rodrigues, CC BY 4.0
https://sketchfab.com/3d-models/sleep-bag-ccd1976fba4445cdb4d6769dbf4db653 (credit is in CREDITS.txt)

v0.11 uses vanilla placeholders (config.cpp):
- EZ_SleepingBag_Packed -> \DZ\gear\camping\camo_net_p.p3d
- EZ_SleepingBag_PackedPlacing and EZ_SleepingBag_Deployed -> \DZ\gear\camping\underground_stash.p3d

Needed files (suggested folder EZ_QOL_Stuff\data\sleepingbag\):
1. ez_sleepingbag_packed.p3d: the rolled bag, about 0.6 x 0.25 x 0.25 m. Item LODs, Geometry (mass about 2.5 kg), View/Fire geometry, Memory with `invView`. It has to fit itemSize 3x4.
2. ez_sleepingbag.p3d: the bag lying flat, about 2.0 x 0.8 x 0.15 m. Its Geometry needs a component the player can target (actions use the cursor on the object: Pack, Claim, Repair). Keep it walkable, because carveNavmesh is 0. Fire Geometry and View Geometry too.
3. ez_sleepingbag_placing.p3d: optional. It can be the same mesh as 2 with a hologram-friendly material; a separate file is only needed if you want one.
4. Textures: _co, _nohq, _smdi, and an .rvmat with 5 health levels (pristine to ruined), the way the pineapple rvmats do it.
5. Then switch the three `model =` lines in config.cpp, and add hiddenSelections only if you add camo selections.

Notes:
- No proxies, attachments or animation sources are needed. Cargo is 10x10 from config.
- Keep the deployed bag's origin at its bottom centre. The deploy uses ECE_PLACE_ON_SURFACE, and the respawn spot is the bag position + SpawnHeightOffset (0.05 m).
