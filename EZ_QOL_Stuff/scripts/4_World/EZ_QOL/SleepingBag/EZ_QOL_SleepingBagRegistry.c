// Server-side owner -> bag binding. One bag per player: binding a new bag clears the old one.
// The file is the truth for "which bag key is current"; a bag that loads with a stale key drops its owner.
class EZ_QOL_SleepingBagRegistry
{
	static const string PATH = "$profile:EZ_QOL/SleepingBagRegistry.json";
	protected static ref EZ_QOL_SleepingBagRegistry s_Instance;
	// Set by optional/EZ_QOL_ExpansionCompat when Expansion's spawn menu lists the bag; the vanilla
	// OnClientNewEvent position override then steps aside.
	static bool s_ExternalSpawnSelect;

	protected ref EZ_QOL_SleepingBagRegistryFile m_File;
	protected ref map<string, ref EZ_QOL_SleepingBagRecord> m_ByUid;
	protected ref map<string, EZ_SleepingBag_Deployed> m_Live; // bagKey -> loaded bag
	protected int m_KeyCounter;

	static EZ_QOL_SleepingBagRegistry Get()
	{
		if (!s_Instance)
		{
			s_Instance = new EZ_QOL_SleepingBagRegistry();
			s_Instance.LoadFile();
		}
		return s_Instance;
	}

	void EZ_QOL_SleepingBagRegistry()
	{
		m_File = new EZ_QOL_SleepingBagRegistryFile();
		m_ByUid = new map<string, ref EZ_QOL_SleepingBagRecord>;
		m_Live = new map<string, EZ_SleepingBag_Deployed>;
	}

	protected void LoadFile()
	{
		EZ_QOL_Log.EnsureDirs();
		if (FileExist(PATH))
			JsonFileLoader<EZ_QOL_SleepingBagRegistryFile>.JsonLoadFile(PATH, m_File);
		if (!m_File.Bags)
			m_File.Bags = new array<ref EZ_QOL_SleepingBagRecord>;
		for (int i = 0; i < m_File.Bags.Count(); i++)
		{
			EZ_QOL_SleepingBagRecord r = m_File.Bags[i];
			if (r && r.OwnerUID != "")
				m_ByUid.Set(r.OwnerUID, r);
		}
		// decay tick every 10 min (server only; Get() is only called server-side)
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DecayTick, 600000, true);
	}

	void Save()
	{
		m_File.Bags.Clear();
		for (int i = 0; i < m_ByUid.Count(); i++)
			m_File.Bags.Insert(m_ByUid.GetElement(i));
		JsonFileLoader<EZ_QOL_SleepingBagRegistryFile>.JsonSaveFile(PATH, m_File);
	}

	string NewKey()
	{
		m_KeyCounter++;
		return EZ_QOL_Time.Now().ToString() + "-" + Math.RandomInt(100000, 999999).ToString() + "-" + m_KeyCounter.ToString();
	}

	EZ_QOL_SleepingBagRecord GetRecord(string uid)
	{
		EZ_QOL_SleepingBagRecord r;
		m_ByUid.Find(uid, r);
		return r;
	}

	// Binds uid to bag; any older bag of that uid loses its owner.
	void Bind(string uid, string ownerName, EZ_SleepingBag_Deployed bag)
	{
		if (uid == "" || !bag)
			return;
		EZ_QOL_SleepingBagRecord r = GetRecord(uid);
		if (!r)
		{
			r = new EZ_QOL_SleepingBagRecord();
			r.OwnerUID = uid;
			m_ByUid.Set(uid, r);
		}
		if (r.BagKey != "" && r.BagKey != bag.EZ_QOL_GetBagKey())
		{
			EZ_SleepingBag_Deployed old;
			if (m_Live.Find(r.BagKey, old) && old)
				old.EZ_QOL_ClearOwner(false);
			m_Live.Remove(r.BagKey);
		}
		r.OwnerName = ownerName;
		r.BagKey = bag.EZ_QOL_GetBagKey();
		r.Pos = bag.GetPosition();
		m_Live.Set(r.BagKey, bag);
		Save();
		EZ_QOL_Log.Info("SleepingBag bound uid=" + uid + " key=" + r.BagKey + " pos=" + r.Pos.ToString());
	}

	// Clears the binding only if it still points at this bag key.
	void Unbind(string uid, string bagKey)
	{
		m_Live.Remove(bagKey);
		EZ_QOL_SleepingBagRecord r = GetRecord(uid);
		if (!r || r.BagKey != bagKey)
			return;
		r.BagKey = "";
		Save();
		EZ_QOL_Log.Info("SleepingBag unbound uid=" + uid + " key=" + bagKey);
	}

	// Called from the bag after load/init. False = stale binding, bag must drop its owner.
	bool OnBagLoaded(EZ_SleepingBag_Deployed bag)
	{
		EZ_QOL_SleepingBagRecord r = GetRecord(bag.EZ_QOL_GetOwnerUID());
		if (!r || r.BagKey != bag.EZ_QOL_GetBagKey())
			return false;
		r.Pos = bag.GetPosition();
		m_Live.Set(r.BagKey, bag);
		return true;
	}

	void OnBagGone(EZ_SleepingBag_Deployed bag)
	{
		// object deleted (shutdown, cleanup, pack): forget the live ref only; pack/ruin unbind explicitly
		m_Live.Remove(bag.EZ_QOL_GetBagKey());
	}

	EZ_SleepingBag_Deployed FindBag(EZ_QOL_SleepingBagRecord r)
	{
		EZ_SleepingBag_Deployed bag;
		if (m_Live.Find(r.BagKey, bag) && bag)
			return bag;
		if (!EZ_QOL_SleepingBagConfig.Get().ValidateBagExists)
			return null;
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition(r.Pos, EZ_QOL_SleepingBagConfig.Get().BagValidationRadius, objs, null);
		for (int i = 0; i < objs.Count(); i++)
		{
			EZ_SleepingBag_Deployed b = EZ_SleepingBag_Deployed.Cast(objs[i]);
			if (b && b.EZ_QOL_GetBagKey() == r.BagKey)
			{
				m_Live.Set(r.BagKey, b);
				return b;
			}
		}
		return null;
	}

	int CooldownLeft(EZ_QOL_SleepingBagRecord r)
	{
		int left = r.LastRespawn + EZ_QOL_SleepingBagConfig.Get().RespawnCooldownMinutes * 60 - EZ_QOL_Time.Now();
		if (left < 0)
			left = 0;
		return left;
	}

	// "" = OK and outPos is set. Otherwise a reason (logged / sent to the player).
	string GetSpawn(string uid, out vector outPos, out EZ_SleepingBag_Deployed outBag)
	{
		EZ_QOL_SleepingBagConfig cfg = EZ_QOL_SleepingBagConfig.Get();
		if (!cfg.Enabled)
			return "disabled";
		EZ_QOL_SleepingBagRecord r = GetRecord(uid);
		if (!r || r.BagKey == "")
			return "no bag";
		int left = CooldownLeft(r);
		if (left > 0)
			return "Sleeping bag cooldown: " + Math.Ceil(left / 60.0).ToString() + " min left";
		EZ_SleepingBag_Deployed bag = FindBag(r);
		if (!bag)
		{
			if (cfg.ValidateBagExists)
			{
				r.BagKey = "";
				Save();
				return "Your sleeping bag is gone";
			}
			outPos = r.Pos;
		}
		else
		{
			if (bag.IsRuined())
			{
				bag.EZ_QOL_ClearOwner(true);
				return "Your sleeping bag is ruined";
			}
			outPos = bag.GetPosition();
		}
		string zone = ZoneBlock(outPos);
		if (zone != "")
			return zone;
		if (EnemyNear(outPos, uid))
			return "Sleeping bag blocked: another player is nearby";
		outBag = bag;
		float newY = outPos[1] + cfg.SpawnHeightOffset;
		outPos[1] = newY;
		return "";
	}

	string ZoneBlock(vector pos)
	{
		EZ_QOL_SleepingBagConfig cfg = EZ_QOL_SleepingBagConfig.Get();
		int t = EZ_QOL_PvPGate.NinjinZoneTypeAt(pos);
		if (!cfg.AllowInSafeZones)
		{
			if (EZ_QOL_DroneZones.InSafezone(pos) || t == 5)
				return "Sleeping bags do not work in safe zones";
		}
		if (!cfg.AllowInPvEZones && t == 2)
			return "Sleeping bags do not work in PvE zones";
		return "";
	}

	// No party/group system in vanilla: any other living player counts as an enemy.
	bool EnemyNear(vector pos, string uid)
	{
		float radius = EZ_QOL_SleepingBagConfig.Get().EnemyBlockRadius;
		if (radius <= 0)
			return false;
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		for (int i = 0; i < players.Count(); i++)
		{
			Man m = players[i];
			if (!m || !m.IsAlive() || !m.GetIdentity())
				continue;
			if (m.GetIdentity().GetId() == uid)
				continue;
			if (vector.Distance(m.GetPosition(), pos) <= radius)
				return true;
		}
		return false;
	}

	// Respawn happened at the bag: start cooldown and wear the bag.
	void OnSpawned(string uid, EZ_SleepingBag_Deployed bag)
	{
		EZ_QOL_SleepingBagRecord r = GetRecord(uid);
		if (!r)
			return;
		r.LastRespawn = EZ_QOL_Time.Now();
		Save();
		if (bag)
			bag.EZ_QOL_ApplyDamage(EZ_QOL_SleepingBagConfig.Get().HealthLossPerRespawn);
	}

	protected void DecayTick()
	{
		float dmg = EZ_QOL_SleepingBagConfig.Get().DecayPerHour / 6.0;
		if (dmg <= 0)
			return;
		// copy first: damage can ruin a bag, which edits the lists
		array<EZ_SleepingBag_Deployed> bags = new array<EZ_SleepingBag_Deployed>;
		bags.Copy(EZ_SleepingBag_Deployed.s_EZ_All);
		for (int j = 0; j < bags.Count(); j++)
		{
			if (bags[j] && !bags[j].IsRuined())
				bags[j].EZ_QOL_ApplyDamage(dmg);
		}
	}

	static void Notify(PlayerBase player, string msg)
	{
		if (!player || !player.GetIdentity() || msg == "")
			return;
		GetGame().RPCSingleParam(player, EZ_QOL_RPC_BAG_NOTIFY, new Param1<string>(msg), true, player.GetIdentity());
	}
}
