EZ_QOL_ExpansionCompat (optional, not compiled by default)
- Needs: EZ_QOL_Stuff + DayZ Expansion Spawn Selection, with SpawnOnTerritory = 1 in Expansion's SpawnSettings.json.
- Effect: the sleeping bag shows up as a spawn point in Expansion's menu; the vanilla OnClientNewEvent /
  alive-login bag override in EZ_QOL_Stuff then steps aside (EZ_QOL_SleepingBagRegistry.s_ExternalSpawnSelect).
- Known gap: picking the bag in Expansion's menu does not start the EZ_QOL cooldown or apply
  HealthLossPerRespawn (Expansion places the player itself). Use Expansion's own spawn cooldown for now.
- Warning: with this PBO loaded but SpawnOnTerritory = 0, the bag is never offered (vanilla path is off).
- Untested in game.
