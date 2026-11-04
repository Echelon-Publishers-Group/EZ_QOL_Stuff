// Money adapter. Swap EZ_QOL_DroneMoney.Create() for a non-Expansion server.
class EZ_QOL_DroneMoney
{
	static EZ_QOL_DroneMoney Create()
	{
	#ifdef EXPANSIONMODMARKET
		return new EZ_QOL_DroneMoneyExpansion();
	#else
		return new EZ_QOL_DroneMoney();
	#endif
	}

	bool IsAvailable() { return false; }
	int GetBalance(PlayerBase player) { return -1; }
	// Must only return true if the full amount was removed.
	bool Charge(PlayerBase player, int amount) { return amount <= 0; }
	// Give money back. Returns true if it was paid out.
	bool Refund(PlayerBase player, int amount) { return amount <= 0; }
	// Free orders when no money system exists (price is ignored).
	bool IsFreeMode() { return true; }
}

#ifdef EXPANSIONMODMARKET
// Same calls Expansion's own ExpansionGarageModule uses (verified in DayZ-Expansion-Scripts):
//   m_MarketModule.GetPlayerWorth(player, monies, NULL, true);  m_MarketModule.RemoveMoney(price, player)
class EZ_QOL_DroneMoneyExpansion: EZ_QOL_DroneMoney
{
	protected ExpansionMarketModule GetMarket()
	{
		ExpansionMarketModule m;
		Class.CastTo(m, CF_ModuleCoreManager.Get(ExpansionMarketModule));
		return m;
	}

	override bool IsAvailable() { return GetMarket() != null; }
	override bool IsFreeMode() { return false; }

	override int GetBalance(PlayerBase player)
	{
		ExpansionMarketModule m = GetMarket();
		if (!m || !player)
			return -1;
		array<int> monies = new array<int>;
		return m.GetPlayerWorth(player, monies, NULL, true);
	}

	override bool Charge(PlayerBase player, int amount)
	{
		if (amount <= 0)
			return true;
		ExpansionMarketModule m = GetMarket();
		if (!m || !player)
			return false;
		if (GetBalance(player) < amount)
			return false;
		// Expansion API: reserve exact coins, RemoveMoney(player) deletes reserved, refund change (ATM deposit pattern).
		array<int> monies = new array<int>;
		if (!m.FindMoneyAndCountTypes(player, amount, monies, true, NULL, NULL, true))
		{
			m.UnlockMoney(player);
			return false;
		}
		EntityAI parent = player;
		int removed = m.RemoveMoney(player);
		if (removed - amount > 0)
		{
			m.SpawnMoney(player, parent, removed - amount, true, NULL, NULL, true);
			m.CheckSpawn(player, parent);
		}
		return removed >= amount;
	}

	// ExpansionMarketModule.SpawnMoney(player, inout parent, amount, useExistingStacks, marketItem, trader, isATM)
	// + CheckSpawn(player, parent): signatures read in ExpansionMarketModule.c (experimental branch).
	override bool Refund(PlayerBase player, int amount)
	{
		if (amount <= 0)
			return true;
		ExpansionMarketModule m = GetMarket();
		if (!m || !player || !player.IsAlive())
			return false;
		EntityAI parent = player;
		array<ItemBase> spawned = m.SpawnMoney(player, parent, amount, true, NULL, NULL, true);
		m.CheckSpawn(player, parent);
		return spawned != null;
	}
}
#endif
