// PvP gate for the attack drone. Nasdara is PvP everywhere; a strike is blocked only when the
// attacker or the target is in a Ninjin PvE (2) / safe (5) zone, an Expansion safezone or an
// Expansion trader (MARKET) zone (plus safezoneBufferM around Expansion zones).
// NO Ninjin symbols here: Ninjin's Zones.json is parsed as plain JSON. Player state from Ninjin's
// callbacks only arrives through the OPTIONAL compat file (optional/EZ_QOL_NinjinCompat), which
// calls EZ_QOL_PvPGate.SetNinjinPlayerBlocked(uid, bool).

// Zones.json layout per github.com/Ninjin89/ninjinspvppvewiki/wiki/Zones (shape 0 circle, 1 polygon).
class EZ_QOL_NinjinZone
{
	string name;
	int type;
	int priority;
	int shape;
	vector center;
	float radius;
	ref array<vector> vertices;
	float zoneMinHeight;
	float zoneMaxHeight;
	int ignoreHeightCheck;
}

class EZ_QOL_NinjinZonesFile
{
	ref array<ref EZ_QOL_NinjinZone> Zones;
}

class EZ_QOL_PvPGate
{
	protected static ref array<ref EZ_QOL_NinjinZone> s_Zones;
	protected static ref map<string, bool> s_NinjinBlockedUids;
	protected static bool s_CompatActive;

	static void Init()
	{
		EZ_QOL_AttackConfig cfg = EZ_QOL_AttackConfig.Get();
		s_Zones = new array<ref EZ_QOL_NinjinZone>;
		if (!s_NinjinBlockedUids)
			s_NinjinBlockedUids = new map<string, bool>;
		if (cfg.pvpSource == "zonesFile" || cfg.pvpSource == "ninjin")
			LoadZonesFile(cfg.ninjinZonesPath);
		int blocking = 0;
		for (int i = 0; i < s_Zones.Count(); i++)
		{
			if (cfg.IsBlockedZoneType(s_Zones[i].type))
				blocking++;
		}
		EZ_QOL_DroneLog.Info("PvPGate source=" + cfg.pvpSource + " zones=" + s_Zones.Count().ToString() + " blocking(PvE/safe)=" + blocking.ToString() + " ninjinCompat=" + s_CompatActive.ToString());
		if (cfg.pvpSource == "ninjin" && !s_CompatActive)
			EZ_QOL_DroneLog.Warn("pvpSource=ninjin but the optional Ninjin compat is not loaded; using Zones.json for players too");
	}

	protected static void LoadZonesFile(string path)
	{
		if (path == "" || !FileExist(path))
		{
			EZ_QOL_DroneLog.Warn("PvPGate: Ninjin zones file not found: " + path + " (only Expansion safezones/traders block)");
			return;
		}
		EZ_QOL_NinjinZonesFile f = new EZ_QOL_NinjinZonesFile();
		JsonFileLoader<EZ_QOL_NinjinZonesFile>.JsonLoadFile(path, f);
		if (!f.Zones)
		{
			EZ_QOL_DroneLog.Warn("PvPGate: no Zones array in " + path);
			return;
		}
		for (int i = 0; i < f.Zones.Count(); i++)
		{
			if (f.Zones[i])
				s_Zones.Insert(f.Zones[i]);
		}
	}

	// Called by the optional Ninjin compat (player state callbacks).
	static void SetNinjinPlayerBlocked(string uid, bool blocked)
	{
		if (!s_NinjinBlockedUids)
			s_NinjinBlockedUids = new map<string, bool>;
		s_CompatActive = true;
		if (blocked)
			s_NinjinBlockedUids.Set(uid, true);
		else
			s_NinjinBlockedUids.Remove(uid);
	}

	protected static bool PointInPolygon(vector p, array<vector> poly)
	{
		bool inside = false;
		int n = poly.Count();
		int j = n - 1;
		for (int i = 0; i < n; i++)
		{
			vector a = poly[i];
			vector b = poly[j];
			bool aAbove = a[2] > p[2];
			bool bAbove = b[2] > p[2];
			if (aAbove != bAbove)
			{
				float xCross = (b[0] - a[0]) * (p[2] - a[2]) / (b[2] - a[2]) + a[0];
				if (p[0] < xCross)
					inside = !inside;
			}
			j = i;
		}
		return inside;
	}

	protected static bool InZone(EZ_QOL_NinjinZone z, vector p)
	{
		if (z.ignoreHeightCheck == 0 && z.zoneMaxHeight > z.zoneMinHeight)
		{
			float h = p[1] - GetGame().SurfaceY(p[0], p[2]);
			if (h < z.zoneMinHeight || h > z.zoneMaxHeight)
				return false;
		}
		if (z.shape == 1)
		{
			if (!z.vertices || z.vertices.Count() < 3)
				return false;
			return PointInPolygon(p, z.vertices);
		}
		float dx = p[0] - z.center[0];
		float dz = p[2] - z.center[2];
		return dx * dx + dz * dz <= z.radius * z.radius;
	}

	// Highest-priority zone containing p decides; returns its type, or 0 when none.
	static int NinjinZoneTypeAt(vector p)
	{
		if (!s_Zones)
			return 0;
		int bestPrio = -1;
		int bestType = 0;
		for (int i = 0; i < s_Zones.Count(); i++)
		{
			EZ_QOL_NinjinZone z = s_Zones[i];
			if (z.priority > bestPrio && InZone(z, p))
			{
				bestPrio = z.priority;
				bestType = z.type;
			}
		}
		return bestType;
	}

	static bool InTraderZone(vector pos)
	{
	#ifdef EXPANSIONMODCORE
		return ExpansionZoneModule.IsInside(pos, ExpansionZoneType.MARKET);
	#else
		return false;
	#endif
	}

	// Expansion safezone or trader at pos, or within bufferM of one (8 samples on the ring).
	static bool NearExpansionZone(vector pos, float bufferM)
	{
		if (EZ_QOL_DroneZones.InSafezone(pos) || InTraderZone(pos))
			return true;
		if (bufferM <= 0)
			return false;
		for (int k = 0; k < 8; k++)
		{
			float ang = k * Math.PI2 / 8.0;
			vector s = Vector(pos[0] + Math.Cos(ang) * bufferM, pos[1], pos[2] + Math.Sin(ang) * bufferM);
			if (EZ_QOL_DroneZones.InSafezone(s) || InTraderZone(s))
				return true;
		}
		return false;
	}

	// "" = allowed. player may be null (vehicle / base / ground targets).
	static string BlockReason(vector pos, PlayerBase player, string who)
	{
		EZ_QOL_AttackConfig cfg = EZ_QOL_AttackConfig.Get();
		if (NearExpansionZone(pos, cfg.safezoneBufferM))
			return who + " is in or near a safezone or trader";
		if (cfg.pvpSource == "none")
			return "";
		if (player && player.GetIdentity() && s_CompatActive && cfg.pvpSource == "ninjin")
		{
			if (s_NinjinBlockedUids.Contains(player.GetIdentity().GetId()))
				return who + " is in a PvE or safe zone";
		}
		int t = NinjinZoneTypeAt(pos);
		if (t != 0 && cfg.IsBlockedZoneType(t))
			return who + " is in a PvE or safe zone";
		return "";
	}
}
