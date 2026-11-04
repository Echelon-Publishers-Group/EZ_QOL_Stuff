// Server singleton: RPCs, order checks, flight simulation, drop, crash.
class EZ_QOL_DroneOrder
{
	string uid;
	string playerName;
	PlayerIdentity identity;
	string categoryId;
	int price;
	vector target;          // ground target (player pos at order time)
	vector home;            // spawn / exit point
	float altitude;
	int state;
	float hoverLeft;
	int etaSec;
	EZ_QOL_DeliveryDrone drone;
	int lastStatusMs;
	int charged;            // amount actually taken (0 in free mode)
	float yaw;
}

class EZ_QOL_DroneManager
{
	protected static ref EZ_QOL_DroneManager s_Instance;
	protected ref array<ref EZ_QOL_DroneOrder> m_Orders;
	protected ref map<string, int> m_Cooldowns;   // key uid or uid|catId -> GetTime() ms when it expires
	protected ref EZ_QOL_DroneMoney m_Money;
	protected int m_LastTickMs;
	static const int TICK_MS = 100;

	static EZ_QOL_DroneManager Get()
	{
		if (!s_Instance)
			s_Instance = new EZ_QOL_DroneManager();
		return s_Instance;
	}

	void EZ_QOL_DroneManager()
	{
		m_Orders = new array<ref EZ_QOL_DroneOrder>;
		m_Cooldowns = new map<string, int>;
	}

	void Init()
	{
		EZ_QOL_DroneConfig.Load();
		m_Money = EZ_QOL_DroneMoney.Create();
		GetRPCManager().AddRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_DRONE_REQUESTCATALOG, this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_DRONE_PLACEORDER, this, SingleplayerExecutionType.Server);
		m_LastTickMs = GetGame().GetTime();
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Tick, TICK_MS, true);
		EZ_QOL_DroneLog.Info("DroneManager started (money adapter: " + m_Money.Type().ToString() + ")");
	}

	// ------------------------------------------------------------ helpers
	protected PlayerBase FindPlayer(PlayerIdentity sender)
	{
		if (!sender)
			return null;
		return PlayerBase.Cast(sender.GetPlayer());
	}

	protected int CooldownLeft(string key)
	{
		int until;
		if (!m_Cooldowns.Find(key, until))
			return 0;
		int left = (until - GetGame().GetTime()) / 1000;
		if (left < 0)
			return 0;
		return left;
	}

	protected int CategoryCooldownLeft(string uid, string catId)
	{
		int a = CooldownLeft(uid);
		int b = CooldownLeft(uid + "|" + catId);
		return Math.Max(a, b);
	}

	protected bool HasActiveOrder(string uid)
	{
		for (int i = 0; i < m_Orders.Count(); i++)
		{
			if (m_Orders[i].uid == uid)
				return true;
		}
		return false;
	}

	protected string BlockReason(vector pos)
	{
		EZ_QOL_DroneConfig cfg = EZ_QOL_DroneConfig.Get();
		if (cfg.blockInSafezone && EZ_QOL_DroneZones.InSafezone(pos))
			return "Drones can't deliver inside a safezone";
		if (cfg.blockInTerritory && EZ_QOL_DroneZones.InTerritory(pos))
			return "Drones can't deliver inside a base territory";
		return "";
	}

	static float WindSpeed()
	{
		return GetGame().GetWeather().GetWindSpeed();
	}

	protected float GroundY(vector p)
	{
		return GetGame().SurfaceY(p[0], p[2]);
	}

	// ------------------------------------------------------------ RPCs
	void EZ_QOL_Drone_RequestCatalog(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
			return;
		PlayerBase player = FindPlayer(sender);
		if (!player)
			return;
		SendCatalog(sender, player);
	}

	void SendCatalog(PlayerIdentity sender, PlayerBase player)
	{
		EZ_QOL_DroneConfig cfg = EZ_QOL_DroneConfig.Get();
		EZ_QOL_DroneCatalog cat = new EZ_QOL_DroneCatalog();
		cat.balance = m_Money.GetBalance(player);
		cat.windSpeed = WindSpeed();
		cat.windWarning = cat.windSpeed >= cfg.windWarnMs;
		cat.blockedReason = BlockReason(player.GetPosition());
		string uid = sender.GetId();
		for (int c = 0; c < cfg.categories.Count(); c++)
		{
			EZ_QOL_DroneConfigCategory src = cfg.categories[c];
			if (!src.enabled)
				continue;
			EZ_QOL_DroneCatalogCategory dst = new EZ_QOL_DroneCatalogCategory();
			dst.id = src.id;
			dst.name = src.name;
			dst.icon = src.icon;
			dst.price = src.price;
			dst.pickOne = src.pickOne;
			dst.cooldownLeft = CategoryCooldownLeft(uid, src.id);
			for (int i = 0; i < src.items.Count(); i++)
			{
				EZ_QOL_DroneCatalogItem it = new EZ_QOL_DroneCatalogItem();
				it.className = src.items[i].className;
				it.qty = src.items[i].qty;
				it.displayName = DisplayName(it.className);
				dst.items.Insert(it);
			}
			cat.categories.Insert(dst);
		}
		string json;
		JsonSerializer js = new JsonSerializer();
		js.WriteToString(cat, false, json);
		// One long string in a Param made the client VM fail in ctx.Read ("String CORRUPTED") and hang.
		// Send the JSON as short chunks instead; the client joins them.
		array<string> chunks = new array<string>;
		int total = json.Length();
		int pos = 0;
		while (pos < total)
		{
			int n = EZ_QOL_DRONE_CATALOG_CHUNK;
			if (pos + n > total)
				n = total - pos;
			chunks.Insert(json.Substring(pos, n));
			pos = pos + n;
		}
		EZ_QOL_DroneLog.Info("Catalog -> " + sender.GetName() + " balance=" + cat.balance.ToString() + " json=" + total.ToString() + "b chunks=" + chunks.Count().ToString());
		GetRPCManager().SendRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_DRONE_CATALOG, new Param1<ref array<string>>(chunks), true, sender);
	}

	protected string DisplayName(string cls)
	{
		string n;
		string roots[3] = {"CfgVehicles ", "CfgMagazines ", "CfgWeapons "};
		for (int r = 0; r < 3; r++)
		{
			if (GetGame().ConfigGetText(roots[r] + cls + " displayName", n) && n != "")
				return n;
		}
		return cls;
	}

	protected void SendResult(PlayerIdentity id, bool ok, string reason, int eta)
	{
		GetRPCManager().SendRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_DRONE_ORDERRESULT, new Param3<bool, string, int>(ok, reason, eta), true, id);
	}

	protected void SendStatus(EZ_QOL_DroneOrder o)
	{
		if (!o.identity)
			return;
		bool warn = WindSpeed() >= EZ_QOL_DroneConfig.Get().windWarnMs;
		GetRPCManager().SendRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_DRONE_STATUS, new Param3<int, int, bool>(o.state, o.etaSec, warn), true, o.identity);
	}

	void EZ_QOL_Drone_PlaceOrder(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
			return;
		Param1<string> data;
		if (!ctx.Read(data))
			return;
		PlayerBase player = FindPlayer(sender);
		if (!player || !player.IsAlive())
			return;

		string catId = data.param1;
		string uid = sender.GetId();
		string who = sender.GetName() + " (" + uid + ")";
		EZ_QOL_DroneConfig cfg = EZ_QOL_DroneConfig.Get();
		EZ_QOL_DroneConfigCategory cat = cfg.FindCategory(catId);

		string refuse = "";
		if (!cfg.enabled)
			refuse = "Drone delivery is disabled";
		else if (!cat || !cat.enabled)
			refuse = "Unknown or unavailable category";
		else if (HasActiveOrder(uid))
			refuse = "You already have a drone in the air";
		else if (CategoryCooldownLeft(uid, catId) > 0)
		{
			int cdLeft = CategoryCooldownLeft(uid, catId);
			refuse = "Cooldown: " + cdLeft.ToString() + "s";
		}
		else if (WindSpeed() >= cfg.windCancelMs)
			refuse = "Grounded by wind";
		else
			refuse = BlockReason(player.GetPosition());

		if (refuse == "" && !HoldsTablet(player))
			refuse = "You need a tablet";

		if (refuse == "")
		{
			// revalidate classes at order time
			int valid = 0;
			for (int i = 0; i < cat.items.Count(); i++)
			{
				if (EZ_QOL_DroneConfig.ClassExists(cat.items[i].className))
					valid++;
			}
			if (valid == 0)
				refuse = "Nothing to deliver in this category";
		}

		int charged = 0;
		if (refuse == "" && !m_Money.IsFreeMode())
		{
			if (m_Money.GetBalance(player) < cat.price)
				refuse = "Not enough money";
			else if (!m_Money.Charge(player, cat.price))
				refuse = "Payment failed";
			else
				charged = cat.price;
		}

		if (refuse != "")
		{
			EZ_QOL_DroneLog.Info("Order refused " + who + " cat=" + catId + ": " + refuse);
			SendResult(sender, false, refuse, 0);
			return;
		}

		EZ_QOL_DroneOrder o = StartOrder(sender, player, cat);
		if (!o)
		{
			bool refunded = m_Money.Refund(player, charged);
			EZ_QOL_DroneLog.Err("Drone spawn failed for " + who + " cat=" + catId + "; refund " + charged + " ok=" + refunded.ToString());
			if (!refunded)
				EZ_QOL_DroneLog.Err("REFUND OWED " + who + " amount=" + charged);
			SendResult(sender, false, "Drone failed to launch (refunded)", 0);
			return;
		}
		o.charged = charged;
		m_Cooldowns.Set(uid, GetGame().GetTime() + cfg.globalCooldownSec * 1000);
		m_Cooldowns.Set(uid + "|" + catId, GetGame().GetTime() + cat.cooldownSec * 1000);
		EZ_QOL_DroneLog.Info("Order OK " + who + " cat=" + catId + " price=" + cat.price + " target=" + o.target.ToString() + " eta=" + o.etaSec + "s wind=" + WindSpeed());
		SendResult(sender, true, "", o.etaSec);
		SendStatus(o);
	}

	protected bool HoldsTablet(PlayerBase player)
	{
		if (!EZ_QOL_DroneConfig.Get().requireTablet)
			return true;
		EntityAI inHands = player.GetHumanInventory().GetEntityInHands();
		if (inHands && inHands.IsInherited(EZ_QOL_Tablet))
			return true;
		array<EntityAI> items = new array<EntityAI>;
		player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
		for (int i = 0; i < items.Count(); i++)
		{
			if (items[i] && items[i].IsInherited(EZ_QOL_Tablet))
				return true;
		}
		return false;
	}

	// ------------------------------------------------------------ flight
	protected EZ_QOL_DroneOrder StartOrder(PlayerIdentity sender, PlayerBase player, EZ_QOL_DroneConfigCategory cat)
	{
		EZ_QOL_DroneConfig cfg = EZ_QOL_DroneConfig.Get();
		EZ_QOL_DroneOrder o = new EZ_QOL_DroneOrder();
		o.uid = sender.GetId();
		o.playerName = sender.GetName();
		o.identity = sender;
		o.categoryId = cat.id;
		o.price = cat.price;
		o.target = player.GetPosition();
		o.altitude = Math.RandomFloatInclusive(cfg.altitudeMin, cfg.altitudeMax);

		float ang = Math.RandomFloat(0, Math.PI2);
		o.home = o.target + Vector(Math.Cos(ang) * cfg.spawnDistance, 0, Math.Sin(ang) * cfg.spawnDistance);
		vector ezHome = o.home;
		float ezHomeY = GroundY(ezHome) + o.altitude;
		o.home = Vector(ezHome[0], ezHomeY, ezHome[2]);
		vector toTarget = o.target - o.home;
		toTarget[1] = 0;
		vector yprStart = toTarget.Normalized().VectorToAngles();
		o.yaw = yprStart[0];

		Object obj = GetGame().CreateObjectEx("EZ_QOL_DeliveryDrone", o.home, ECE_NONE, RF_DEFAULT);
		if (!Class.CastTo(o.drone, obj))
		{
			if (obj)
				GetGame().ObjectDelete(obj);
			return null;
		}
		o.drone.EZ_QOL_SetRotors(true);
		o.drone.EZ_QOL_SetCargoAttached(true);
		o.state = EZ_QOL_DRONE_STATE_INBOUND;
		o.hoverLeft = cfg.hoverTime;
		o.etaSec = EstimateEta(o.home, o.target);
		m_Orders.Insert(o);
		return o;
	}

	// Ground speed shrinks with wind (headwind worst case, simple model).
	protected float GroundSpeed()
	{
		EZ_QOL_DroneConfig cfg = EZ_QOL_DroneConfig.Get();
		float s = cfg.speed - WindSpeed() * cfg.windDriftFactor * 2.0;
		if (s < cfg.speed * 0.3)
			s = cfg.speed * 0.3;
		return s;
	}

	protected int EstimateEta(vector from, vector to)
	{
		float d = vector.Distance(Vector(from[0], 0, from[2]), Vector(to[0], 0, to[2]));
		return Math.Ceil(d / GroundSpeed()) + EZ_QOL_DroneConfig.Get().hoverTime;
	}

	void Tick()
	{
		int now = GetGame().GetTime();
		float dt = (now - m_LastTickMs) / 1000.0;
		m_LastTickMs = now;
		if (dt <= 0 || dt > 1.0)
			dt = TICK_MS / 1000.0;

		for (int i = m_Orders.Count() - 1; i >= 0; i--)
		{
			EZ_QOL_DroneOrder o = m_Orders[i];
			if (!o.drone || o.drone.IsSetForDeletion())
			{
				EZ_QOL_DroneLog.Warn("Drone for " + o.playerName + " vanished, order dropped");
				m_Orders.Remove(i);
				continue;
			}
			bool done = StepOrder(o, dt, now);
			if (done)
				m_Orders.Remove(i);
		}
	}

	// returns true when the order is finished
	protected bool StepOrder(EZ_QOL_DroneOrder o, float dt, int now)
	{
		EZ_QOL_DroneConfig cfg = EZ_QOL_DroneConfig.Get();
		vector pos = o.drone.GetPosition();
		vector prevPos = pos;
		vector wind = GetGame().GetWeather().GetWind();
		float windSpeed = wind.Length();

		if (o.state == EZ_QOL_DRONE_STATE_INBOUND || o.state == EZ_QOL_DRONE_STATE_OUTBOUND)
		{
			// crash roll
			if (o.state == EZ_QOL_DRONE_STATE_INBOUND && Math.RandomFloat01() < cfg.crashChancePerMs * windSpeed * dt)
			{
				Crash(o);
				return true;
			}

			vector dest = o.target;
			if (o.state == EZ_QOL_DRONE_STATE_OUTBOUND)
				dest = o.home;
			vector flat = Vector(dest[0] - pos[0], 0, dest[2] - pos[2]);
			float dist = flat.Length();
			float step = GroundSpeed() * dt;

			if (o.state == EZ_QOL_DRONE_STATE_INBOUND && dist <= Math.Max(step, 1.5))
			{
				o.state = EZ_QOL_DRONE_STATE_HOVER;
				o.etaSec = Math.Ceil(o.hoverLeft);
				SendStatus(o);
				pos = Vector(dest[0], pos[1], dest[2]);
			}
			else if (o.state == EZ_QOL_DRONE_STATE_OUTBOUND && dist <= Math.Max(step, 2.0))
			{
				EZ_QOL_DroneLog.Info("Drone for " + o.playerName + " left the area, despawned");
				o.drone.EZ_QOL_SetRotors(false);
				GetGame().ObjectDelete(o.drone);
				return true;
			}
			else
			{
				vector dir = flat.Normalized();
				pos = pos + dir * step;
				// wind drift (perpendicular push, corrected next tick by steering toward dest)
				pos = pos + Vector(wind[0], 0, wind[2]) * cfg.windDriftFactor * dt;
				vector ypr = dir.VectorToAngles();
				o.yaw = ypr[0];
				if (o.state == EZ_QOL_DRONE_STATE_INBOUND)
					o.etaSec = Math.Ceil(dist / GroundSpeed()) + Math.Ceil(o.hoverLeft);
			}
		}
		else if (o.state == EZ_QOL_DRONE_STATE_HOVER)
		{
			o.hoverLeft -= dt;
			o.etaSec = Math.Max(0, Math.Ceil(o.hoverLeft));
			if (o.hoverLeft <= 0)
			{
				DropCrate(o, false);
				o.state = EZ_QOL_DRONE_STATE_OUTBOUND;
				o.etaSec = 0;
				SendStatus(o);
			}
		}

		// terrain following: SurfaceY + altitude (hover descends to 15 m for the drop)
		float alt = o.altitude;
		if (o.state == EZ_QOL_DRONE_STATE_HOVER)
			alt = 15;
		float wantY = GroundY(pos) + alt;
		float curY = pos[1];
		float climb = 8.0 * dt;
		if (wantY > curY)
			pos[1] = Math.Min(wantY, curY + climb);
		else
			pos[1] = Math.Max(wantY, curY - climb);
		vector vel = (pos - prevPos) * (1.0 / dt);
		if (o.state == EZ_QOL_DRONE_STATE_HOVER)
			vel = "0 0 0";
		o.drone.EZ_QOL_ServerMove(pos, vel, o.yaw);

		if (now - o.lastStatusMs > 2000)
		{
			o.lastStatusMs = now;
			SendStatus(o);
		}
		return false;
	}

	protected void Crash(EZ_QOL_DroneOrder o)
	{
		EZ_QOL_DroneConfig cfg = EZ_QOL_DroneConfig.Get();
		vector ezPos = o.drone.GetPosition();
		EZ_QOL_DroneLog.Warn("Drone for " + o.playerName + " crashed at " + ezPos.ToString() + " wind=" + WindSpeed());
		o.state = EZ_QOL_DRONE_STATE_CRASHED;
		o.etaSec = 0;
		if (cfg.crateOnCrash)
			DropCrate(o, true);
		if (cfg.refundOnCrash && o.charged > 0)
		{
			PlayerBase owner = null;
			if (o.identity)
				owner = PlayerBase.Cast(o.identity.GetPlayer());
			bool ok = m_Money.Refund(owner, o.charged);
			EZ_QOL_DroneLog.Info("Crash refund " + o.playerName + " amount=" + o.charged + " ok=" + ok.ToString());
			if (!ok)
				EZ_QOL_DroneLog.Err("REFUND OWED " + o.playerName + " (" + o.uid + ") amount=" + o.charged);
		}
		SendStatus(o);
		o.drone.EZ_QOL_SetRotors(false);
		GetGame().ObjectDelete(o.drone);
	}

	protected void DropCrate(EZ_QOL_DroneOrder o, bool crashed)
	{
		EZ_QOL_DroneConfig cfg = EZ_QOL_DroneConfig.Get();
		EZ_QOL_DroneConfigCategory cat = cfg.FindCategory(o.categoryId);
		vector ezPos = o.drone.EZ_QOL_GetCargoDropWorld();
		float ezGroundY = GroundY(ezPos);
		ezPos[1] = ezGroundY;
		o.drone.EZ_QOL_SetCargoAttached(false);

		EZ_QOL_DroneCrate crate;
		Object obj = GetGame().CreateObjectEx("EZ_QOL_DroneCrate", ezPos, ECE_PLACE_ON_SURFACE | ECE_CREATEPHYSICS, RF_DEFAULT);
		if (!Class.CastTo(crate, obj))
		{
			EZ_QOL_DroneLog.Err("Crate spawn failed at " + ezPos.ToString());
			return;
		}

		int spawned = 0;
		if (cat)
		{
			if (cat.pickOne)
			{
				EZ_QOL_DroneConfigItem pick = PickWeighted(cat.items);
				if (pick)
					spawned += FillItem(crate, pick.className, pick.qty);
			}
			else
			{
				for (int i = 0; i < cat.items.Count(); i++)
					spawned += FillItem(crate, cat.items[i].className, cat.items[i].qty);
			}
		}

		if (!crashed)
		{
			Object smoke = GetGame().CreateObjectEx("EZ_QOL_DroneMarker", ezPos + Vector(0.8, 0, 0.8), ECE_PLACE_ON_SURFACE, RF_DEFAULT);
			EZ_QOL_DroneMarker marker;
			if (Class.CastTo(marker, smoke))
				marker.EZ_QOL_Activate();
		}
		EZ_QOL_DroneLog.Info("Crate for " + o.playerName + " at " + ezPos.ToString() + " items=" + spawned + " crashed=" + crashed.ToString());
	}

	protected int FillItem(EZ_QOL_DroneCrate crate, string cls, int qty)
	{
		if (!EZ_QOL_DroneConfig.ClassExists(cls))
		{
			EZ_QOL_DroneLog.Warn("Order-time: class '" + cls + "' missing, skipped");
			return 0;
		}
		int n = 0;
		for (int q = 0; q < qty; q++)
		{
			EntityAI e = crate.GetInventory().CreateInInventory(cls);
			if (!e)
			{
				// no room: put it on the ground next to the crate
				e = EntityAI.Cast(GetGame().CreateObjectEx(cls, crate.GetPosition() + Vector(0.6, 0, 0), ECE_PLACE_ON_SURFACE, RF_DEFAULT));
				if (e)
					EZ_QOL_DroneLog.Warn("Crate full, '" + cls + "' placed on ground");
			}
			if (e)
				n++;
		}
		return n;
	}

	protected EZ_QOL_DroneConfigItem PickWeighted(array<ref EZ_QOL_DroneConfigItem> items)
	{
		float total = 0;
		int i;
		for (i = 0; i < items.Count(); i++)
			total += Math.Max(0, items[i].weight);
		if (total <= 0)
			return null;
		float r = Math.RandomFloat(0, total);
		for (i = 0; i < items.Count(); i++)
		{
			r -= Math.Max(0, items[i].weight);
			if (r <= 0)
				return items[i];
		}
		return items[items.Count() - 1];
	}
}
