// Server side of the attack drone (lock-on C4 strike).
// The client only sends a HINT (what it thinks it is aiming at). Every 100 ms the server rebuilds the
// aim ray itself (head bone + vanilla aiming angles, same maths as Weapon_Base) and raycasts it; the
// client's claims are never trusted for hits, damage or targets.
class EZ_QOL_Strike
{
	string uid;
	string name;
	int state;
	float armLeft;
	float lockHeld;
	float grace;
	// candidate hint from the client
	bool hintHasObject;
	int hintLow;
	int hintHigh;
	vector hintPoint;
	int hintMs;
	// locked target
	int targetKind;          // 0 ground, 1 player, 2 vehicle, 3 base
	int targetLow;
	int targetHigh;
	vector targetPos;
	string targetUid;
	string targetDesc;
	// flight
	EZ_QOL_AttackDrone drone;
	vector dronePos;
	float yaw;
	float flight;
	float leaveLeft;
	int charged;
	bool warned;
	float stateSendAcc;
	float gateCheckAcc;
	string lastReason;
}

class EZ_QOL_StrikeManager
{
	protected static ref EZ_QOL_StrikeManager s_Instance;
	protected ref array<ref EZ_QOL_Strike> m_Strikes;
	protected ref map<string, int> m_Cooldowns; // uid -> GetTime() ms when it expires
	protected ref EZ_QOL_DroneMoney m_Money;
	protected int m_ServerNextMs;
	static const int TICK_MS = 100;
	static const int KIND_GROUND = 0;
	static const int KIND_PLAYER = 1;
	static const int KIND_VEHICLE = 2;
	static const int KIND_BASE = 3;

	static EZ_QOL_StrikeManager Get()
	{
		if (!s_Instance)
			s_Instance = new EZ_QOL_StrikeManager();
		return s_Instance;
	}

	void EZ_QOL_StrikeManager()
	{
		m_Strikes = new array<ref EZ_QOL_Strike>;
		m_Cooldowns = new map<string, int>;
	}

	void Init()
	{
		EZ_QOL_AttackConfig.Load();
		EZ_QOL_PvPGate.Init();
		m_Money = EZ_QOL_DroneMoney.Create();
		GetRPCManager().AddRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_STRIKE_INFO, this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_STRIKE_ARM, this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_STRIKE_CANDIDATE, this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_STRIKE_CANCEL, this, SingleplayerExecutionType.Server);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Tick, TICK_MS, true);
		EZ_QOL_AttackConfig cfg = EZ_QOL_AttackConfig.Get();
		Log("StrikeManager started enabled=" + cfg.enabled.ToString() + " price=" + cfg.price.ToString() + " ammo=" + cfg.ammoType);
	}

	protected void Log(string msg)
	{
		EZ_QOL_AttackConfig cfg = EZ_QOL_AttackConfig.Get();
		if (cfg && !cfg.logging)
			return;
		EZ_QOL_DroneLog.Info("[AttackDrone] " + msg);
	}

	// ------------------------------------------------------------ helpers
	protected PlayerBase FindPlayer(PlayerIdentity sender)
	{
		if (!sender)
			return null;
		return PlayerBase.Cast(sender.GetPlayer());
	}

	protected PlayerBase FindPlayerByUid(string uid)
	{
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		for (int i = 0; i < players.Count(); i++)
		{
			Man m = players[i];
			if (m && m.GetIdentity() && m.GetIdentity().GetId() == uid)
				return PlayerBase.Cast(m);
		}
		return null;
	}

	protected EZ_QOL_Strike FindStrike(string uid)
	{
		for (int i = 0; i < m_Strikes.Count(); i++)
		{
			if (m_Strikes[i].uid == uid)
				return m_Strikes[i];
		}
		return null;
	}

	protected int CountActive(string uid)
	{
		int n = 0;
		for (int i = 0; i < m_Strikes.Count(); i++)
		{
			EZ_QOL_Strike s = m_Strikes[i];
			if (s.state != EZ_QOL_STRIKE_INFLIGHT && s.state != EZ_QOL_STRIKE_DIVE)
				continue;
			if (uid == "" || s.uid == uid)
				n++;
		}
		return n;
	}

	protected int CooldownLeft(string uid)
	{
		int until;
		int best = 0;
		if (m_Cooldowns.Find(uid, until))
			best = (until - GetGame().GetTime()) / 1000;
		int srv = (m_ServerNextMs - GetGame().GetTime()) / 1000;
		if (srv > best)
			best = srv;
		if (best < 0)
			return 0;
		return best;
	}

	protected bool WeatherGrounded()
	{
		EZ_QOL_AttackConfig cfg = EZ_QOL_AttackConfig.Get();
		if (!cfg.groundInSandstorm)
			return false;
		return EZ_QOL_DroneManager.WindSpeed() >= cfg.windCancelMs;
	}

	protected bool HasRaisedFirearm(PlayerBase player)
	{
		if (!player || !player.IsRaised())
			return false;
		Weapon_Base w = Weapon_Base.Cast(player.GetHumanInventory().GetEntityInHands());
		return w != null;
	}

	protected string PlayerBlock(PlayerBase player)
	{
		if (!player || !player.IsAlive())
			return "You are dead";
		if (player.IsRestrained())
			return "You are restrained";
		if (player.IsUnconscious())
			return "You are unconscious";
		if (player.IsInVehicle())
			return "Get out of the vehicle first";
		return "";
	}

	protected string ArmBlock(PlayerBase player, string uid)
	{
		EZ_QOL_AttackConfig cfg = EZ_QOL_AttackConfig.Get();
		if (!cfg.enabled)
			return "Attack drones are disabled on this server";
		string pb = PlayerBlock(player);
		if (pb != "")
			return pb;
		if (cfg.requireTablet)
		{
			EZ_QOL_Tablet tab = EZ_QOL_Tablet.Cast(player.GetHumanInventory().GetEntityInHands());
			if (!tab)
				return "Use the tablet to arm a strike";
		}
		if (CooldownLeft(uid) > 0)
			return "Cooldown: " + CooldownLeft(uid).ToString() + " s";
		if (CountActive(uid) >= cfg.maxActivePerPlayer)
			return "You already have a strike in the air";
		if (CountActive("") >= cfg.maxActiveServer)
			return "All attack drones are busy, try later";
		if (WeatherGrounded())
			return "Drones are grounded (high wind / sandstorm)";
		int bal = m_Money.GetBalance(player);
		if (!m_Money.IsFreeMode() && bal >= 0 && bal < cfg.price)
			return "Not enough money";
		return EZ_QOL_PvPGate.BlockReason(player.GetPosition(), player, "You");
	}

	// Aim ray rebuilt on the server: head bone + base aiming angles (same convention as Weapon_Base).
	protected bool AimRay(PlayerBase player, out vector from, out vector dir)
	{
		HumanCommandWeapons hcw = player.GetCommandModifier_Weapons();
		if (!hcw)
			return false;
		int head = player.GetBoneIndexByName("Head");
		if (head >= 0)
			from = player.GetBonePositionWS(head);
		else
			from = player.GetPosition() + Vector(0, 1.6, 0);
		float yawDeg = hcw.GetBaseAimingAngleLR() + player.GetOrientation()[0];
		float pitchDeg = hcw.GetBaseAimingAngleUD();
		vector ypr = Vector(yawDeg, pitchDeg, 0);
		dir = ypr.AnglesToVector();
		return true;
	}

	protected float AngleTo(vector from, vector dir, vector point)
	{
		vector to = point - from;
		float len = to.Length();
		if (len < 0.01)
			return 0;
		vector toN = to.Normalized();
		float d = vector.Dot(dir, toN);
		d = Math.Clamp(d, -1, 1);
		return Math.Acos(d) * Math.RAD2DEG;
	}

	protected bool LineClear(vector from, vector to, Object ignoreA, Object ignoreB)
	{
		vector hitPos;
		vector hitDir;
		int hitComp;
		set<Object> results = new set<Object>;
		bool hit = DayZPhysics.RaycastRV(from, to, hitPos, hitDir, hitComp, results, ignoreB, ignoreA, false, false, ObjIntersectFire, 0.0);
		if (!hit)
			return true;
		return vector.Distance(hitPos, to) < 1.5;
	}

	// Classify an object into a target kind. Returns -1 when not a valid object target (=> use ground point).
	protected int ClassifyObject(Object obj)
	{
		if (!obj)
			return -1;
		if (obj.IsMan())
		{
			Man m = Man.Cast(obj);
			if (m && m.GetIdentity() && m.IsAlive())
				return KIND_PLAYER;
			return -1; // AI / bodies -> ground point
		}
		if (obj.IsTransport())
			return KIND_VEHICLE;
		if (obj.IsInherited(BaseBuildingBase))
			return KIND_BASE;
		return -1; // animals, infected, map buildings, everything else -> ground point
	}

	protected vector AimPointOf(Object obj, int kind)
	{
		vector p = obj.GetPosition();
		if (kind == KIND_PLAYER)
			p = p + Vector(0, 1.0, 0);
		else if (kind == KIND_VEHICLE)
			p = p + Vector(0, 0.8, 0);
		return p;
	}

	// Server validation of the current aim. Writes the validated target (kind/net id/pos).
	// Returns false when the aim isn't on a valid target this tick.
	protected bool ValidateAim(EZ_QOL_Strike s, PlayerBase player, out int kind, out int low, out int high, out vector pos, out Object obj)
	{
		EZ_QOL_AttackConfig cfg = EZ_QOL_AttackConfig.Get();
		kind = KIND_GROUND;
		low = 0;
		high = 0;
		obj = null;
		if (!HasRaisedFirearm(player))
			return false;
		vector from;
		vector dir;
		if (!AimRay(player, from, dir))
			return false;
		vector rayEnd = from + dir * cfg.maxRangeM;
		vector hitPos;
		vector hitDir;
		int hitComp;
		set<Object> results = new set<Object>;
		bool hit = DayZPhysics.RaycastRV(from, rayEnd, hitPos, hitDir, hitComp, results, null, player, true, false, ObjIntersectFire, 0.15);

		// 1) the client's object hint, if the server's own ray hit it or it sits inside the cone with a clear line
		if (s.hintHasObject)
		{
			Object hinted = GetGame().GetObjectByNetworkId(s.hintLow, s.hintHigh);
			int hk = ClassifyObject(hinted);
			if (hinted && hk >= 0)
			{
				vector ap = AimPointOf(hinted, hk);
				bool onRay = false;
				if (results)
				{
					for (int i = 0; i < results.Count(); i++)
					{
						Object r = results[i];
						if (r == hinted)
							onRay = true;
						else if (r && r.GetParent() == hinted)
							onRay = true;
					}
				}
				bool inCone = false;
				if (!onRay && AngleTo(from, dir, ap) <= cfg.coneDeg)
					inCone = LineClear(from, ap, player, hinted);
				if (onRay || inCone)
				{
					kind = hk;
					obj = hinted;
					pos = ap;
					hinted.GetNetworkID(low, high);
					return true;
				}
			}
		}

		// 2) server's own nearest hit: a valid object, otherwise a ground point
		if (!hit)
			return false;
		if (results)
		{
			for (int j = 0; j < results.Count(); j++)
			{
				Object ro = results[j];
				int rk = ClassifyObject(ro);
				if (rk >= 0)
				{
					kind = rk;
					obj = ro;
					pos = AimPointOf(ro, rk);
					ro.GetNetworkID(low, high);
					return true;
				}
			}
		}
		kind = KIND_GROUND;
		pos = hitPos;
		return true;
	}

	// Ground points: same point within groundPointToleranceM plus the cone's spread at that range.
	protected bool SameTarget(EZ_QOL_Strike s, int kind, int low, int high, vector pos, vector shooterPos)
	{
		EZ_QOL_AttackConfig cfg = EZ_QOL_AttackConfig.Get();
		if (kind != s.targetKind)
			return false;
		if (kind == KIND_GROUND)
		{
			float rangeM = vector.Distance(shooterPos, s.targetPos);
			float tol = cfg.groundPointToleranceM + rangeM * Math.Tan(cfg.coneDeg * Math.DEG2RAD);
			return vector.Distance(s.targetPos, pos) <= tol;
		}
		return low == s.targetLow && high == s.targetHigh;
	}

	protected string Describe(int kind, Object obj)
	{
		if (kind == KIND_PLAYER)
		{
			Man m = Man.Cast(obj);
			if (m && m.GetIdentity())
				return "player " + m.GetIdentity().GetName() + " (" + m.GetIdentity().GetId() + ")";
			return "player";
		}
		if (kind == KIND_VEHICLE && obj)
			return "vehicle " + obj.GetType();
		if (kind == KIND_BASE && obj)
			return "base " + obj.GetType();
		return "ground point";
	}

	protected void SendState(EZ_QOL_Strike s, PlayerBase player)
	{
		if (!player || !player.GetIdentity())
			return;
		EZ_QOL_AttackConfig cfg = EZ_QOL_AttackConfig.Get();
		float lockPct = 0;
		if (cfg.lockAcquireSec > 0)
			lockPct = Math.Clamp(s.lockHeld / cfg.lockAcquireSec, 0, 1);
		if (s.state == EZ_QOL_STRIKE_INFLIGHT || s.state == EZ_QOL_STRIKE_DIVE)
			lockPct = 1;
		float gracePct = 1;
		if (cfg.graceSec > 0)
			gracePct = Math.Clamp(s.grace / cfg.graceSec, 0, 1);
		int eta = -1;
		if (s.drone)
		{
			float spd = cfg.speed;
			if (s.state == EZ_QOL_STRIKE_DIVE)
				spd = cfg.speed * cfg.diveSpeedMult;
			eta = Math.Round(vector.Distance(s.dronePos, s.targetPos) / spd);
		}
		Param5<int, float, float, int, string> p = new Param5<int, float, float, int, string>(s.state, lockPct, gracePct, eta, s.lastReason);
		GetRPCManager().SendRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_STRIKE_STATE, p, true, player.GetIdentity());
	}

	protected void SendStateTo(PlayerIdentity id, int state, string reason)
	{
		if (!id)
			return;
		Param5<int, float, float, int, string> p = new Param5<int, float, float, int, string>(state, 0, 0, -1, reason);
		GetRPCManager().SendRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_STRIKE_STATE, p, true, id);
	}

	protected void Notify(PlayerIdentity id, string title, string text)
	{
		if (!id)
			return;
		NotificationSystem.SendNotificationToPlayerIdentityExtended(id, 6, title, text, "");
	}

	// ------------------------------------------------------------ RPCs
	void EZ_QOL_Strike_Info(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
			return;
		PlayerBase player = FindPlayer(sender);
		if (!player)
			return;
		SendInfo(sender, player);
	}

	protected void SendInfo(PlayerIdentity sender, PlayerBase player)
	{
		EZ_QOL_AttackConfig cfg = EZ_QOL_AttackConfig.Get();
		string uid = sender.GetId();
		string reason = "";
		EZ_QOL_Strike s = FindStrike(uid);
		if (!s)
			reason = ArmBlock(player, uid);
		int bal = m_Money.GetBalance(player);
		Param5<int, int, int, string, bool> p = new Param5<int, int, int, string, bool>(cfg.price, CooldownLeft(uid), bal, reason, cfg.enabled);
		GetRPCManager().SendRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_STRIKE_INFOREPLY, p, true, sender);
		if (s)
			SendState(s, player);
	}

	void EZ_QOL_Strike_Arm(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server)
			return;
		PlayerBase player = FindPlayer(sender);
		if (!player)
			return;
		string uid = sender.GetId();
		EZ_QOL_AttackConfig cfg = EZ_QOL_AttackConfig.Get();
		EZ_QOL_Strike existing = FindStrike(uid);
		if (existing)
		{
			SendState(existing, player);
			return;
		}
		string reason = ArmBlock(player, uid);
		if (reason != "")
		{
			Log("Arm refused uid=" + uid + " reason=" + reason);
			SendStateTo(sender, EZ_QOL_STRIKE_ABORTED, reason);
			SendInfo(sender, player);
			return;
		}
		EZ_QOL_Strike s = new EZ_QOL_Strike();
		s.uid = uid;
		s.name = sender.GetName();
		s.state = EZ_QOL_STRIKE_ARMED;
		s.armLeft = cfg.armWindowSec;
		s.grace = cfg.graceSec;
		s.targetKind = -1;
		m_Strikes.Insert(s);
		Log("Armed uid=" + uid + " name=" + s.name + " pos=" + player.GetPosition().ToString());
		SendState(s, player);
	}

	void EZ_QOL_Strike_Candidate(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || !sender)
			return;
		Param4<int, int, vector, bool> data;
		if (!ctx.Read(data))
			return;
		EZ_QOL_Strike s = FindStrike(sender.GetId());
		if (!s)
			return;
		int now = GetGame().GetTime();
		if (now - s.hintMs < 150)
			return; // rate limit (client sends at 5 Hz)
		s.hintMs = now;
		s.hintLow = data.param1;
		s.hintHigh = data.param2;
		s.hintPoint = data.param3;
		s.hintHasObject = data.param4;
	}

	void EZ_QOL_Strike_Cancel(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || !sender)
			return;
		EZ_QOL_Strike s = FindStrike(sender.GetId());
		if (!s)
			return;
		Abort(s, "Cancelled by the operator", false);
	}

	// ------------------------------------------------------------ flow
	protected void Abort(EZ_QOL_Strike s, string reason, bool shotDown)
	{
		if (s.state == EZ_QOL_STRIKE_ABORTED || s.state == EZ_QOL_STRIKE_HIT)
			return; // already finished (avoid double refunds)
		EZ_QOL_AttackConfig cfg = EZ_QOL_AttackConfig.Get();
		PlayerBase owner = FindPlayerByUid(s.uid);
		int refund = 0;
		if (s.charged > 0 && !shotDown)
			refund = s.charged * cfg.abortRefundPct / 100;
		bool refunded = true;
		if (refund > 0 && owner)
			refunded = m_Money.Refund(owner, refund);
		Log("Abort uid=" + s.uid + " state=" + s.state.ToString() + " target=" + s.targetDesc + " reason=" + reason + " shotDown=" + shotDown.ToString() + " charged=" + s.charged.ToString() + " refund=" + refund.ToString() + " refundOk=" + refunded.ToString());
		s.state = EZ_QOL_STRIKE_ABORTED;
		s.lastReason = reason;
		if (refund > 0)
			s.lastReason = reason + " (refund " + refund.ToString() + ")";
		if (owner)
		{
			SendState(s, owner);
			Notify(owner.GetIdentity(), "Attack drone", s.lastReason);
		}
		if (s.drone)
		{
			if (shotDown)
			{
				s.drone.EZ_QOL_Detonate(cfg.ammoType); // C4 cooks off where it falls
				DeleteDroneLater(s.drone);
				s.drone = null;
			}
			else
			{
				s.drone.EZ_QOL_SetRotors(true);
				s.leaveLeft = 8; // flies off, then is removed
				return;
			}
		}
		m_Strikes.RemoveItem(s);
	}

	protected void DeleteDroneLater(EZ_QOL_AttackDrone d)
	{
		if (!d)
			return;
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(GetGame().ObjectDelete, 1500, false, d);
	}

	protected void Launch(EZ_QOL_Strike s, PlayerBase player)
	{
		EZ_QOL_AttackConfig cfg = EZ_QOL_AttackConfig.Get();
		float range = vector.Distance(player.GetPosition(), s.targetPos);
		if (range > cfg.maxRangeM)
		{
			Abort(s, "Target out of range", false);
			return;
		}
		if (range < cfg.minRangeM)
		{
			Abort(s, "Target too close (min " + cfg.minRangeM.ToString() + " m)", false);
			return;
		}
		if (s.targetKind == KIND_PLAYER && !cfg.allowPlayerTargets)
		{
			Abort(s, "Player targets are disabled", false);
			return;
		}
		if (s.targetKind == KIND_VEHICLE && !cfg.allowVehicleTargets)
		{
			Abort(s, "Vehicle targets are disabled", false);
			return;
		}
		if (s.targetKind == KIND_BASE && !cfg.allowBaseTargets)
		{
			Abort(s, "Base targets are disabled", false);
			return;
		}
		if (s.targetKind == KIND_GROUND && !cfg.allowGroundTargets)
		{
			Abort(s, "Ground targets are disabled", false);
			return;
		}
		string gate = EZ_QOL_PvPGate.BlockReason(player.GetPosition(), player, "You");
		if (gate == "")
		{
			PlayerBase tp = null;
			if (s.targetKind == KIND_PLAYER)
				tp = FindPlayerByUid(s.targetUid);
			gate = EZ_QOL_PvPGate.BlockReason(s.targetPos, tp, "The target");
		}
		if (gate != "")
		{
			Abort(s, gate, false);
			return;
		}
		if (CountActive("") >= cfg.maxActiveServer)
		{
			Abort(s, "All attack drones are busy, try later", false);
			return;
		}
		if (!m_Money.IsFreeMode())
		{
			if (!m_Money.Charge(player, cfg.price))
			{
				Abort(s, "Not enough money", false);
				return;
			}
			s.charged = cfg.price;
		}
		// spawn 600-900 m out, behind the operator (+-60 deg), at cruise altitude
		vector away = player.GetPosition() - s.targetPos;
		away[1] = 0;
		if (away.Length() < 1)
			away = Vector(1, 0, 0);
		vector awayN = away.Normalized();
		vector awayAng = awayN.VectorToAngles();
		float spawnYaw = awayAng[0] + Math.RandomFloatInclusive(-60, 60);
		vector spawnAng = Vector(spawnYaw, 0, 0);
		vector spawnDir = spawnAng.AnglesToVector();
		float dist = Math.RandomFloatInclusive(cfg.spawnDistanceMin, cfg.spawnDistanceMax);
		vector sp = s.targetPos + spawnDir * dist;
		float spGround = GetGame().SurfaceY(sp[0], sp[2]);
		sp[1] = spGround + cfg.cruiseAltitude;
		Object obj = GetGame().CreateObjectEx("EZ_QOL_AttackDrone", sp, ECE_NONE, RF_DEFAULT);
		s.drone = EZ_QOL_AttackDrone.Cast(obj);
		if (!s.drone)
		{
			if (obj)
				GetGame().ObjectDelete(obj);
			if (s.charged > 0)
				m_Money.Refund(player, s.charged); // full refund: nothing was launched
			s.charged = 0;
			Log("Launch failed: could not create EZ_QOL_AttackDrone (config.cpp class missing?) uid=" + s.uid);
			Abort(s, "Drone unavailable", false);
			return;
		}
		s.drone.SetAllowDamage(true);
		s.drone.EZ_QOL_SetRotors(true);
		s.dronePos = sp;
		s.flight = 0;
		s.grace = cfg.graceSec;
		s.state = EZ_QOL_STRIKE_INFLIGHT;
		s.lastReason = "";
		m_Cooldowns.Set(s.uid, GetGame().GetTime() + cfg.cooldownSec * 1000);
		m_ServerNextMs = GetGame().GetTime() + cfg.serverCooldownSec * 1000;
		Log("LAUNCH uid=" + s.uid + " name=" + s.name + " target=" + s.targetDesc + " targetPos=" + s.targetPos.ToString() + " from=" + player.GetPosition().ToString() + " range=" + range.ToString() + " charged=" + s.charged.ToString() + " spawn=" + sp.ToString());
		Notify(player.GetIdentity(), "Attack drone", "Launched. Hold the lock.");
	}

	protected void Impact(EZ_QOL_Strike s, vector where, string why)
	{
		EZ_QOL_AttackConfig cfg = EZ_QOL_AttackConfig.Get();
		if (s.drone)
		{
			s.drone.EZ_QOL_ServerMove(where, "0 0 0", s.yaw);
			s.drone.EZ_QOL_Detonate(cfg.ammoType);
			DeleteDroneLater(s.drone);
			s.drone = null;
		}
		s.state = EZ_QOL_STRIKE_HIT;
		s.lastReason = "";
		Log("IMPACT uid=" + s.uid + " target=" + s.targetDesc + " at=" + where.ToString() + " why=" + why + " flight=" + s.flight.ToString() + "s");
		PlayerBase owner = FindPlayerByUid(s.uid);
		if (owner)
			SendState(s, owner);
		m_Strikes.RemoveItem(s);
	}

	protected void WarnTargets(EZ_QOL_Strike s, int eta)
	{
		EZ_QOL_AttackConfig cfg = EZ_QOL_AttackConfig.Get();
		s.warned = true;
		if (!cfg.warnTarget)
			return;
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		string warnedList = "";
		for (int i = 0; i < players.Count(); i++)
		{
			Man m = players[i];
			if (!m || !m.GetIdentity())
				continue;
			bool isTarget = s.targetKind == KIND_PLAYER && m.GetIdentity().GetId() == s.targetUid;
			if (!isTarget && vector.Distance(m.GetPosition(), s.targetPos) > cfg.warnRadiusM)
				continue;
			Param2<vector, int> p = new Param2<vector, int>(s.dronePos, eta);
			GetRPCManager().SendRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_STRIKE_WARN, p, true, m.GetIdentity());
			warnedList = warnedList + m.GetIdentity().GetId() + " ";
		}
		Log("Warned uid=" + s.uid + " eta=" + eta.ToString() + " players=" + warnedList);
	}

	// ------------------------------------------------------------ tick
	void Tick()
	{
		float dt = TICK_MS / 1000.0;
		for (int i = m_Strikes.Count() - 1; i >= 0; i--)
		{
			if (i >= m_Strikes.Count())
				continue;
			EZ_QOL_Strike s = m_Strikes[i];
			if (s.state == EZ_QOL_STRIKE_ABORTED)
			{
				TickLeaving(s, dt);
				continue;
			}
			TickStrike(s, dt);
		}
	}

	protected void TickLeaving(EZ_QOL_Strike s, float dt)
	{
		EZ_QOL_AttackConfig cfg = EZ_QOL_AttackConfig.Get();
		s.leaveLeft -= dt;
		if (s.drone && s.leaveLeft > 0)
		{
			vector yawAng = Vector(s.yaw, 0, 0);
			vector fwd = yawAng.AnglesToVector();
			vector vel = fwd * cfg.speed + Vector(0, 4, 0);
			s.dronePos = s.dronePos + vel * dt;
			s.drone.EZ_QOL_ServerMove(s.dronePos, vel, s.yaw);
			return;
		}
		if (s.drone)
			GetGame().ObjectDelete(s.drone);
		s.drone = null;
		m_Strikes.RemoveItem(s);
	}

	protected void TickStrike(EZ_QOL_Strike s, float dt)
	{
		EZ_QOL_AttackConfig cfg = EZ_QOL_AttackConfig.Get();
		PlayerBase player = FindPlayerByUid(s.uid);
		bool inFlight = s.state == EZ_QOL_STRIKE_INFLIGHT || s.state == EZ_QOL_STRIKE_DIVE;
		if (!player)
		{
			Abort(s, "Operator disconnected", false);
			return;
		}
		string pb = PlayerBlock(player);
		if (pb != "")
		{
			Abort(s, pb, false);
			return;
		}
		if (inFlight && s.drone && s.drone.EZ_QOL_IsShotDown())
		{
			Log("SHOT DOWN uid=" + s.uid + " by=" + s.drone.EZ_QOL_GetLastShooterUid() + " at=" + s.dronePos.ToString());
			Abort(s, "Drone shot down", true);
			return;
		}
		if (inFlight && !s.drone)
		{
			Abort(s, "Drone lost", false);
			return;
		}
		if (inFlight && WeatherGrounded())
		{
			Abort(s, "High wind / sandstorm: drone recalled", false);
			return;
		}

		// lock validation (10 Hz, server ray)
		int kind;
		int low;
		int high;
		vector pos;
		Object obj;
		bool valid = ValidateAim(s, player, kind, low, high, pos, obj);

		if (!inFlight)
		{
			s.armLeft -= dt;
			if (s.armLeft <= 0)
			{
				Abort(s, "No lock in time", false);
				return;
			}
			if (valid && s.targetKind >= 0 && SameTarget(s, kind, low, high, pos, player.GetPosition()))
			{
				s.lockHeld += dt;
				s.grace = Math.Min(cfg.graceSec, s.grace + cfg.graceRefillPerSec * dt);
				if (kind != KIND_GROUND)
					s.targetPos = pos;
			}
			else if (valid)
			{
				// new target: restart acquisition
				s.targetKind = kind;
				s.targetLow = low;
				s.targetHigh = high;
				s.targetPos = pos;
				s.targetUid = "";
				if (kind == KIND_PLAYER && obj && Man.Cast(obj).GetIdentity())
					s.targetUid = Man.Cast(obj).GetIdentity().GetId();
				s.targetDesc = Describe(kind, obj);
				s.lockHeld = 0;
				s.grace = cfg.graceSec;
			}
			else if (s.targetKind >= 0)
			{
				s.grace -= dt;
				if (s.grace <= 0)
				{
					s.targetKind = -1;
					s.lockHeld = 0;
					s.grace = cfg.graceSec;
				}
			}
			if (s.targetKind >= 0)
				s.state = EZ_QOL_STRIKE_ACQUIRING;
			else
				s.state = EZ_QOL_STRIKE_ARMED;
			if (s.lockHeld >= cfg.lockAcquireSec)
				Launch(s, player);
		}
		else
		{
			if (valid && SameTarget(s, kind, low, high, pos, player.GetPosition()))
			{
				s.grace = Math.Min(cfg.graceSec, s.grace + cfg.graceRefillPerSec * dt);
				if (kind != KIND_GROUND)
					s.targetPos = pos;
			}
			else
			{
				s.grace -= dt;
				if (s.grace <= 0)
				{
					Abort(s, "Lock lost", false);
					return;
				}
			}
			// moving targets: keep following the locked object even between good aim ticks
			if (s.targetKind != KIND_GROUND)
			{
				Object tobj = GetGame().GetObjectByNetworkId(s.targetLow, s.targetHigh);
				if (tobj)
					s.targetPos = AimPointOf(tobj, s.targetKind);
			}
			s.gateCheckAcc += dt;
			if (s.gateCheckAcc >= 1.0)
			{
				s.gateCheckAcc = 0;
				PlayerBase tp = null;
				if (s.targetKind == KIND_PLAYER)
					tp = FindPlayerByUid(s.targetUid);
				string gate = EZ_QOL_PvPGate.BlockReason(s.targetPos, tp, "The target");
				if (gate == "")
					gate = EZ_QOL_PvPGate.BlockReason(player.GetPosition(), player, "You");
				if (gate != "")
				{
					Abort(s, gate, false);
					return;
				}
			}
			if (!FlyStep(s, dt))
				return; // impacted
			s.flight += dt;
			if (s.flight >= cfg.timeoutSec)
			{
				Abort(s, "Timed out", false);
				return;
			}
		}

		s.stateSendAcc += dt;
		if (s.stateSendAcc >= 0.2)
		{
			s.stateSendAcc = 0;
			SendState(s, player);
		}
	}

	// Returns false when the drone detonated this step.
	protected bool FlyStep(EZ_QOL_Strike s, float dt)
	{
		EZ_QOL_AttackConfig cfg = EZ_QOL_AttackConfig.Get();
		vector cur = s.dronePos;
		vector flat = s.targetPos - cur;
		flat[1] = 0;
		float horiz = flat.Length();
		vector vel;
		float spd;
		if (horiz > cfg.diveStartM)
		{
			spd = cfg.speed;
			vector flatN = flat.Normalized();
			vel = flatN * spd;
			float wantY = GetGame().SurfaceY(cur[0], cur[2]) + cfg.cruiseAltitude;
			float minY = s.targetPos[1] + cfg.cruiseAltitude * 0.5;
			if (wantY < minY)
				wantY = minY;
			float climb = Math.Clamp((wantY - cur[1]) * 0.5, -6, 6);
			vel[1] = climb;
			s.state = EZ_QOL_STRIKE_INFLIGHT;
		}
		else
		{
			spd = cfg.speed * cfg.diveSpeedMult;
			vector toT = s.targetPos - cur;
			vector toTN = toT.Normalized();
			vel = toTN * spd;
			s.state = EZ_QOL_STRIKE_DIVE;
		}
		if (horiz > 1)
		{
			vector flatAng = flat.VectorToAngles();
			s.yaw = flatAng[0];
		}
		vector next = cur + vel * dt;
		float distT = vector.Distance(cur, s.targetPos);
		float eta = distT / spd;
		if (!s.warned && eta <= cfg.warnLeadSec)
		{
			int etaInt = Math.Round(eta);
			WarnTargets(s, etaInt);
		}

		// detonate: close enough, or the step would pass the target
		if (distT <= cfg.detonateDistanceM || distT <= spd * dt)
		{
			Impact(s, s.targetPos, "proximity");
			return false;
		}
		// detonate on anything in the way (walls, trees, vehicles) along this step
		vector hitPos;
		vector hitDir;
		int hitComp;
		if (DayZPhysics.RaycastRV(cur, next, hitPos, hitDir, hitComp, null, null, s.drone, false, false, ObjIntersectFire, 0.3))
		{
			Impact(s, hitPos, "contact");
			return false;
		}
		float ground = GetGame().SurfaceY(next[0], next[2]);
		if (next[1] <= ground + 0.5)
		{
			vector gp = Vector(next[0], ground + 0.3, next[2]);
			Impact(s, gp, "ground");
			return false;
		}
		s.dronePos = next;
		s.drone.EZ_QOL_ServerMove(next, vel, s.yaw);
		return true;
	}
}
