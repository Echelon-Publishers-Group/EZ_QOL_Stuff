// Server singleton: ledger, park/retrieve/pack/deploy, startup dupe check, orphans, RPCs.
// Coexistence: own RPC names (EZ_QOL_Garage_*), own folder $profile:EZ_QOL/Garage; never reads VGS data.
class EZ_QOL_LedgerEntry
{
	string Id;
	int State;
	string VehType;
	string ParkedVehKey;    // persistent id of the car that was parked (must not exist while STORED)
	string RestoredVehKey;  // persistent id of the car created by the last retrieve
	int LastSeen;           // epoch s, holder last seen
	int OrphanedAt;
	int Generation;
}

class EZ_QOL_Ledger
{
	int Version = 1;
	ref array<ref EZ_QOL_LedgerEntry> Entries = new array<ref EZ_QOL_LedgerEntry>;
}

[CF_RegisterModule(EZ_QOL_GarageModule)]
class EZ_QOL_GarageModule : CF_ModuleWorld
{
	protected static EZ_QOL_GarageModule s_Instance;
	protected ref EZ_QOL_Ledger m_Ledger;
	protected ref map<string, EZ_QOL_LedgerEntry> m_ById;
	protected ref map<string, EntityAI> m_Holders;      // storage id -> current kit/garage
	protected ref map<string, float> m_LastRpc;         // uid -> time
	protected ref array<string> m_LoadedVehKeys;        // vehicles seen during startup window
	protected ref map<string, CarScript> m_LoadedVeh;
	protected bool m_StartupDone;

	// client side
	static ref ScriptInvoker s_OnClientState = new ScriptInvoker(); // (bool hasVehicle, string type, bool locked)
	static ref ScriptInvoker s_OnClientResult = new ScriptInvoker(); // (bool ok, string reasonKey)

	static EZ_QOL_GarageModule Get() { return s_Instance; }

	override void OnInit()
	{
		super.OnInit();
		s_Instance = this;
		EnableMissionStart();
		EnableInvokeConnect();
		m_ById = new map<string, EZ_QOL_LedgerEntry>;
		m_Holders = new map<string, EntityAI>;
		m_LastRpc = new map<string, float>;
		m_LoadedVehKeys = new array<string>;
		m_LoadedVeh = new map<string, CarScript>;

		GetRPCManager().AddRPC(EZ_QOL_RPC_NS, EZ_QOL_RPC_GARAGE_REQUESTSTATE, this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC(EZ_QOL_RPC_NS, EZ_QOL_RPC_GARAGE_PARK, this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC(EZ_QOL_RPC_NS, EZ_QOL_RPC_GARAGE_RETRIEVE, this, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC(EZ_QOL_RPC_NS, EZ_QOL_RPC_GARAGE_STATE, this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC(EZ_QOL_RPC_NS, EZ_QOL_RPC_GARAGE_RESULT, this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC(EZ_QOL_RPC_NS, EZ_QOL_RPC_GARAGE_CONFIG, this, SingleplayerExecutionType.Client);
	}

	override void OnMissionStart(Class sender, CF_EventArgs args)
	{
		super.OnMissionStart(sender, args);
		if (!GetGame().IsServer())
			return;
		EZ_QOL_GarageConfig.Load();
		LoadLedger();
		int delay = EZ_QOL_GarageConfig.Get().StartupCheckDelaySec * 1000;
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(StartupDupeCheck, delay, false);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(OrphanSweep, delay + 30000, false);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(OrphanSweep, 6 * 3600 * 1000, true);
		EZ_QOL_Log.Info("Garage module started, " + m_Ledger.Entries.Count() + " ledger entries");
	}

	override void OnInvokeConnect(Class sender, CF_EventArgs args)
	{
		super.OnInvokeConnect(sender, args);
		CF_EventPlayerArgs pa = CF_EventPlayerArgs.Cast(args);
		if (pa && pa.Identity)
			GetRPCManager().SendRPC(EZ_QOL_RPC_NS, EZ_QOL_RPC_GARAGE_CONFIG, new Param1<string>(EZ_QOL_GarageConfig.Get().ClientJson()), true, pa.Identity);
	}

	// ------------------------------------------------------------------ ledger
	protected void LoadLedger()
	{
		m_Ledger = new EZ_QOL_Ledger();
		if (FileExist(EZ_QOL_GARAGE_LEDGER))
			JsonFileLoader<EZ_QOL_Ledger>.JsonLoadFile(EZ_QOL_GARAGE_LEDGER, m_Ledger);
		m_ById.Clear();
		foreach (EZ_QOL_LedgerEntry e : m_Ledger.Entries)
			m_ById.Set(e.Id, e);
	}

	protected void SaveLedger()
	{
		string tmp = EZ_QOL_GARAGE_LEDGER + ".tmp";
		JsonFileLoader<EZ_QOL_Ledger>.JsonSaveFile(tmp, m_Ledger);
		CopyFile(tmp, EZ_QOL_GARAGE_LEDGER);
		DeleteFile(tmp);
	}

	static int Now() { return CF_Date.Now(true).DateToEpoch(); }

	static string NewId()
	{
		string id = Now().ToString();
		for (int i = 0; i < 3; i++)
			id += "-" + Math.RandomInt(0x1000, 0xFFFF).ToString();
		return id;
	}

	EZ_QOL_LedgerEntry GetEntry(string id, bool create = false)
	{
		EZ_QOL_LedgerEntry e;
		if (m_ById.Find(id, e))
			return e;
		if (!create)
			return null;
		e = new EZ_QOL_LedgerEntry();
		e.Id = id;
		e.State = EZ_QOL_GarageState.EMPTY;
		e.LastSeen = Now();
		m_Ledger.Entries.Insert(e);
		m_ById.Set(id, e);
		return e;
	}

	int GetState(string id)
	{
		EZ_QOL_LedgerEntry e = GetEntry(id);
		if (!e) return EZ_QOL_GarageState.EMPTY;
		return e.State;
	}

	// ------------------------------------------------------------------ holders (kit / garage)
	// Returns false if the id is already held by another live entity (dupe): caller clears its id.
	bool RegisterHolder(string id, EntityAI holder)
	{
		if (id == "") return true;
		EntityAI cur;
		if (m_Holders.Find(id, cur) && cur && cur != holder && !cur.IsSetForDeletion())
		{
			EZ_QOL_Log.Warn("DUPE holder for " + id + ": " + holder.GetType() + " at " + holder.GetPosition() + " (live one at " + cur.GetPosition() + "), cleared");
			return false;
		}
		m_Holders.Set(id, holder);
		EZ_QOL_LedgerEntry e = GetEntry(id, true);
		e.LastSeen = Now();
		if (e.State == EZ_QOL_GarageState.ORPHANED)
			EZ_QOL_Log.Warn("Holder for orphaned " + id + " came back; admin can restore from Orphans/");
		return true;
	}

	void UnregisterHolder(string id, EntityAI holder)
	{
		EntityAI cur;
		if (m_Holders.Find(id, cur) && cur == holder)
			m_Holders.Remove(id);
	}

	void OnHolderRuined(string id)
	{
		EZ_QOL_LedgerEntry e = GetEntry(id);
		if (e && e.State == EZ_QOL_GarageState.STORED)
			Orphan(e, "holder ruined");
	}

	protected void Orphan(EZ_QOL_LedgerEntry e, string why)
	{
		EZ_QOL_GarageStore.MoveTo(e.Id, EZ_QOL_GARAGE_ORPHANS);
		e.State = EZ_QOL_GarageState.ORPHANED;
		e.OrphanedAt = Now();
		SaveLedger();
		EZ_QOL_Log.Warn("Orphaned " + e.Id + " (" + e.VehType + "): " + why);
	}

	void OrphanSweep()
	{
		EZ_QOL_GarageConfig cfg = EZ_QOL_GarageConfig.Get();
		int now = Now();
		bool dirty;
		for (int i = m_Ledger.Entries.Count() - 1; i >= 0; i--)
		{
			EZ_QOL_LedgerEntry e = m_Ledger.Entries[i];
			EntityAI h;
			if (m_Holders.Find(e.Id, h) && h)
			{
				e.LastSeen = now;
				dirty = true;
				continue;
			}
			if (e.State == EZ_QOL_GarageState.STORED && now - e.LastSeen > cfg.OrphanDays * 86400)
				Orphan(e, "holder not seen for " + cfg.OrphanDays + " days");
			else if (e.State == EZ_QOL_GarageState.ORPHANED && now - e.OrphanedAt > cfg.OrphanPurgeDays * 86400)
			{
				TStringArray exts = {".bin", ".bak", ".json"};
				foreach (string ext : exts)
					DeleteFile(EZ_QOL_GARAGE_ORPHANS + "/" + e.Id + ext);
				EZ_QOL_Log.Info("Purged orphan " + e.Id);
				m_ById.Remove(e.Id);
				m_Ledger.Entries.Remove(i);
				dirty = true;
			}
			else if (e.State == EZ_QOL_GarageState.EMPTY && now - e.LastSeen > cfg.OrphanDays * 86400)
			{
				m_ById.Remove(e.Id);
				m_Ledger.Entries.Remove(i);
				dirty = true;
			}
		}
		if (dirty) SaveLedger();
	}

	// ------------------------------------------------------------------ startup dupe check
	static void OnVehicleLoaded(CarScript car)
	{
		if (!s_Instance) return;
		string key = car.EZ_QOL_GetPersistentKey();
		if (!s_Instance.m_StartupDone)
		{
			s_Instance.m_LoadedVeh.Set(key, car);
			return;
		}
		s_Instance.CheckVehicle(key, car);
	}

	protected void CheckVehicle(string key, CarScript car)
	{
		foreach (EZ_QOL_LedgerEntry e : m_Ledger.Entries)
		{
			if (e.ParkedVehKey == key)
			{
				if (e.State == EZ_QOL_GarageState.STORED)
				{
					EZ_QOL_Log.Warn("DUPE: " + car.GetType() + " " + key + " exists while stored in " + e.Id + "; deleting the world copy");
					GetGame().ObjectDelete(car);
				}
				else if (e.State == EZ_QOL_GarageState.PENDING)
				{
					EZ_QOL_Log.Warn("Park of " + e.Id + " did not finish; keeping the car, dropping the file");
					EZ_QOL_GarageStore.Finish(e.Id);
					e.State = EZ_QOL_GarageState.EMPTY;
					SyncHolder(e.Id);
				}
				e.ParkedVehKey = "";
				SaveLedger();
				return;
			}
			if (e.RestoredVehKey == key && e.State == EZ_QOL_GarageState.RETRIEVING)
			{
				EZ_QOL_Log.Info("Retrieve of " + e.Id + " confirmed after restart");
				EZ_QOL_GarageStore.Finish(e.Id);
				e.State = EZ_QOL_GarageState.EMPTY;
				SyncHolder(e.Id);
				SaveLedger();
				return;
			}
		}
	}

	void StartupDupeCheck()
	{
		foreach (string key, CarScript car : m_LoadedVeh)
		{
			if (car)
				CheckVehicle(key, car);
		}
		m_LoadedVeh.Clear();
		foreach (EZ_QOL_LedgerEntry e : m_Ledger.Entries)
		{
			if (e.State == EZ_QOL_GarageState.RETRIEVING)
			{
				string bin = EZ_QOL_GarageStore.Bin(e.Id);
				string bak = EZ_QOL_GarageStore.Bak(e.Id);
				if (!FileExist(bin) && FileExist(bak))
					CopyFile(bak, bin);
				e.State = EZ_QOL_GarageState.STORED;
				EZ_QOL_Log.Warn("Retrieve of " + e.Id + " not confirmed, restored .bak -> STORED");
			}
			else if (e.State == EZ_QOL_GarageState.PENDING)
			{
				if (FileExist(EZ_QOL_GarageStore.Bin(e.Id)))
					e.State = EZ_QOL_GarageState.STORED;
				else
					e.State = EZ_QOL_GarageState.EMPTY;
				EZ_QOL_Log.Warn("Pending park " + e.Id + " resolved to state " + e.State);
			}
			SyncHolder(e.Id);
		}
		// files with no ledger entry
		string name;
		FileAttr attr;
		FindFileHandle fh = FindFile(EZ_QOL_GARAGE_DIR + "/*.bin", name, attr, 0);
		if (fh)
		{
			bool more = true;
			while (more)
			{
				string id = name.Substring(0, name.Length() - 4);
				if (!GetEntry(id))
					EZ_QOL_Log.Warn("Unknown storage file " + name + " (no ledger entry)");
				more = FindNextFile(fh, name, attr);
			}
			CloseFindFile(fh);
		}
		m_StartupDone = true;
		SaveLedger();
		EZ_QOL_Log.Info("Startup dupe check done");
	}

	protected void SyncHolder(string id)
	{
		EntityAI h;
		if (!m_Holders.Find(id, h) || !h) return;
		EZ_QOL_GarageKit kit;
		EZ_QOL_Garage g;
		EZ_QOL_LedgerEntry e = GetEntry(id);
		string type = "";
		if (e && e.State == EZ_QOL_GarageState.STORED) type = e.VehType;
		if (Class.CastTo(g, h)) g.EZ_QOL_SetStorage(id, type);
		else if (Class.CastTo(kit, h)) kit.EZ_QOL_SetStorage(id, type);
	}

	// ------------------------------------------------------------------ operations (server)
	bool CanUse(EZ_QOL_Garage g, PlayerBase p)
	{
		return g.EZ_QOL_CanPlayerUse(p);
	}

	CarScript FindVehicleAtSpot(EZ_QOL_Garage g)
	{
		vector spot = g.EZ_QOL_GetVehicleSpot();
		float r = EZ_QOL_GarageConfig.Get().ParkRadius;
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition3D(spot, r, objs, null);
		CarScript best;
		float bestD = r + 1;
		foreach (Object o : objs)
		{
			CarScript c;
			if (Class.CastTo(c, o) && !c.IsRuined())
			{
				float d = vector.Distance(spot, c.GetPosition());
				if (d < bestD) { best = c; bestD = d; }
			}
		}
		return best;
	}

	string CheckVehicleRules(CarScript car)
	{
		EZ_QOL_GarageConfig cfg = EZ_QOL_GarageConfig.Get();
		if (!car) return EZ_QOL_Reason.NO_VEHICLE;
		if (cfg.RequireEngineOff && car.EngineIsOn()) return EZ_QOL_Reason.ENGINE_ON;
		for (int i = 0; i < car.CrewSize(); i++)
		{
			if (car.CrewMember(i)) return EZ_QOL_Reason.CREW;
		}
		foreach (string bl : cfg.VehicleBlacklist)
		{
			if (car.IsKindOf(bl)) return EZ_QOL_Reason.VEH_BLACKLIST;
		}
		vector s = EZ_QOL_GarageStore.VehicleSize(car);
		if (s[0] > cfg.MaxVehicleSize[0] || s[2] > cfg.MaxVehicleSize[1] || s[1] > cfg.MaxVehicleSize[2])
			return EZ_QOL_Reason.TOO_BIG;
		return EZ_QOL_Reason.OK;
	}

	string Park(EZ_QOL_Garage g, PlayerBase p, CarScript car = null)
	{
		if (!g || !p) return EZ_QOL_Reason.FAILED;
		if (!CanUse(g, p)) return EZ_QOL_Reason.LOCKED;
		string id = g.EZ_QOL_GetStorageId();
		if (id == "")
		{
			id = NewId();
			g.EZ_QOL_SetStorage(id, "");
			RegisterHolder(id, g);
		}
		EZ_QOL_LedgerEntry e = GetEntry(id, true);
		if (e.State != EZ_QOL_GarageState.EMPTY) return EZ_QOL_Reason.NOT_EMPTY;
		if (!car) car = FindVehicleAtSpot(g);
		string why = CheckVehicleRules(car);
		if (why != "") return why;

		e.State = EZ_QOL_GarageState.PENDING;
		e.VehType = car.GetType();
		e.ParkedVehKey = car.EZ_QOL_GetPersistentKey();
		e.Generation++;
		SaveLedger();

		EZ_QOL_GarageSidecar sc;
		if (!EZ_QOL_GarageStore.Store(car, id, sc))
		{
			e.State = EZ_QOL_GarageState.EMPTY;
			e.ParkedVehKey = "";
			SaveLedger();
			return EZ_QOL_Reason.FAILED;
		}
		e.State = EZ_QOL_GarageState.STORED;
		SaveLedger();
		EZ_QOL_Log.Info(p.GetIdentity().GetPlainId() + " parked " + e.VehType + " " + e.ParkedVehKey + " in " + id + " (" + sc.ItemCount + " items)");
		GetGame().ObjectDelete(car);
		g.EZ_QOL_SetStorage(id, e.VehType);
		return EZ_QOL_Reason.PARKED;
	}

	string Retrieve(EZ_QOL_Garage g, PlayerBase p)
	{
		if (!g || !p) return EZ_QOL_Reason.FAILED;
		if (!CanUse(g, p)) return EZ_QOL_Reason.LOCKED;
		string id = g.EZ_QOL_GetStorageId();
		EZ_QOL_LedgerEntry e = GetEntry(id);
		if (!e || e.State != EZ_QOL_GarageState.STORED) return EZ_QOL_Reason.EMPTY;

		// spot must be clear
		EZ_QOL_GarageSidecar sc = EZ_QOL_GarageStore.LoadSidecar(id);
		vector spot = g.EZ_QOL_GetVehicleSpot();
		vector ori = g.EZ_QOL_GetVehicleOrientation();
		vector size = Vector(Math.Max(sc.SizeX, 1) * 0.9, Math.Max(sc.SizeY, 1) * 0.8, Math.Max(sc.SizeZ, 1) * 0.9);
		array<Object> excl = {g, p};
		array<Object> hits = new array<Object>;
		if (GetGame().IsBoxColliding(spot + Vector(0, size[1] * 0.5 + 0.2, 0), ori, size, excl, hits))
		{
			foreach (Object h : hits)
			{
				if (h && !h.IsBush() && h != g)
					return EZ_QOL_Reason.SPOT_BLOCKED;
			}
		}

		e.State = EZ_QOL_GarageState.RETRIEVING;
		SaveLedger();
		EntityAI ent = EZ_QOL_GarageStore.Restore(id, spot + Vector(0, 0.3, 0), ori);
		if (!ent)
		{
			e.State = EZ_QOL_GarageState.STORED;
			SaveLedger();
			return EZ_QOL_Reason.FAILED;
		}
		CarScript car;
		if (Class.CastTo(car, ent))
			e.RestoredVehKey = car.EZ_QOL_GetPersistentKey();
		SaveLedger();
		// Confirm: if the car is still alive a few seconds later, drop the .bak and close the entry.
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ConfirmRetrieve, 5000, false, id, ent);
		g.EZ_QOL_SetStorage(id, "");
		EZ_QOL_Log.Info(p.GetIdentity().GetPlainId() + " retrieved " + e.VehType + " from " + id);
		return EZ_QOL_Reason.RETRIEVED;
	}

	void ConfirmRetrieve(string id, EntityAI ent)
	{
		EZ_QOL_LedgerEntry e = GetEntry(id);
		if (!e || e.State != EZ_QOL_GarageState.RETRIEVING) return;
		if (ent && !ent.IsSetForDeletion())
		{
			EZ_QOL_GarageStore.Finish(id);
			e.State = EZ_QOL_GarageState.EMPTY;
			e.ParkedVehKey = "";
			SaveLedger();
		}
		else
		{
			EZ_QOL_Log.Err("Retrieved vehicle for " + id + " vanished; .bak kept, state stays RETRIEVING until restart check");
		}
	}

	// garage -> kit
	string Pack(EZ_QOL_Garage g, PlayerBase p)
	{
		if (!g || !p) return EZ_QOL_Reason.FAILED;
		if (g.ExpansionHasCodeLock()) return EZ_QOL_Reason.HAS_LOCK;
		if (!CanUse(g, p)) return EZ_QOL_Reason.LOCKED;
		string id = g.EZ_QOL_GetStorageId();
		if (GetState(id) == EZ_QOL_GarageState.PENDING || GetState(id) == EZ_QOL_GarageState.RETRIEVING) return EZ_QOL_Reason.BUSY;

		vector pos = p.GetPosition() + p.GetDirection() * 0.8;
		EZ_QOL_GarageKit kit = EZ_QOL_GarageKit.Cast(GetGame().CreateObjectEx("EZ_QOL_GarageKit", pos, ECE_PLACE_ON_SURFACE));
		if (!kit) return EZ_QOL_Reason.FAILED;
		string type = "";
		if (GetState(id) == EZ_QOL_GarageState.STORED) type = GetEntry(id).VehType;
		UnregisterHolder(id, g);
		g.EZ_QOL_ClearStorage(); // so its EEDelete does nothing
		kit.EZ_QOL_SetStorage(id, type);
		RegisterHolder(id, kit);
		GetGame().ObjectDelete(g);
		EZ_QOL_Log.Info(p.GetIdentity().GetPlainId() + " packed garage " + id + " (" + type + ")");
		return EZ_QOL_Reason.PACKED;
	}

	// kit -> garage
	string Deploy(EZ_QOL_GarageKit kit, PlayerBase p, vector pos, vector ori)
	{
		if (!kit || !p) return EZ_QOL_Reason.FAILED;
		vector aPos, aOri;
		string reason, detail;
		if (!EZ_QOL_GaragePlacement.Validate(p, pos, ori, aPos, aOri, reason, detail, kit))
			return reason;
		EZ_QOL_Garage g = EZ_QOL_Garage.Cast(GetGame().CreateObjectEx("EZ_QOL_Garage", aPos, ECE_KEEPHEIGHT | ECE_CREATEPHYSICS));
		if (!g) return EZ_QOL_Reason.FAILED;
		g.SetOrientation(aOri);
		string id = kit.EZ_QOL_GetStorageId();
		string type = kit.EZ_QOL_GetVehType();
		UnregisterHolder(id, kit);
		kit.EZ_QOL_ClearStorage();
		if (id != "")
		{
			g.EZ_QOL_SetStorage(id, type);
			RegisterHolder(id, g);
		}
		GetGame().ObjectDelete(kit);
		EZ_QOL_Log.Info(p.GetIdentity().GetPlainId() + " planted garage " + id + " at " + aPos);
		return EZ_QOL_Reason.PLANTED;
	}

	// ------------------------------------------------------------------ messages
	static void SendResult(PlayerBase p, string reasonKey)
	{
		if (!p || !p.GetIdentity()) return;
		bool ok = (reasonKey == EZ_QOL_Reason.PARKED || reasonKey == EZ_QOL_Reason.RETRIEVED || reasonKey == EZ_QOL_Reason.PACKED || reasonKey == EZ_QOL_Reason.PLANTED);
		GetRPCManager().SendRPC(EZ_QOL_RPC_NS, EZ_QOL_RPC_GARAGE_RESULT, new Param2<bool, string>(ok, reasonKey), true, p.GetIdentity());
	}

	// ------------------------------------------------------------------ RPC handlers
	protected bool RateOk(PlayerIdentity sender)
	{
		if (!sender) return false;
		float now = GetGame().GetTickTime();
		float last;
		if (m_LastRpc.Find(sender.GetId(), last) && now - last < EZ_QOL_GarageConfig.Get().RpcCooldownSec)
			return false;
		m_LastRpc.Set(sender.GetId(), now);
		return true;
	}

	protected bool RpcContext(PlayerIdentity sender, Object target, out PlayerBase p, out EZ_QOL_Garage g)
	{
		if (!sender) return false;
		p = PlayerBase.Cast(sender.GetPlayer());
		g = EZ_QOL_Garage.Cast(target);
		if (!p || !g || !p.IsAlive()) return false;
		return vector.Distance(p.GetPosition(), g.GetPosition()) <= EZ_QOL_GarageConfig.Get().UseDistance;
	}

	void EZ_QOL_Garage_RequestState(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server) return;
		PlayerBase p; EZ_QOL_Garage g;
		if (!RpcContext(sender, target, p, g)) return;
		string id = g.EZ_QOL_GetStorageId();
		bool has = GetState(id) == EZ_QOL_GarageState.STORED;
		string vt = "";
		if (has) vt = GetEntry(id).VehType;
		GetRPCManager().SendRPC(EZ_QOL_RPC_NS, EZ_QOL_RPC_GARAGE_STATE, new Param3<bool, string, bool>(has, vt, g.ExpansionIsLocked()), true, sender);
	}

	void EZ_QOL_Garage_Park(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server) return;
		PlayerBase p; EZ_QOL_Garage g;
		if (!RpcContext(sender, target, p, g)) return;
		if (!RateOk(sender)) { SendResult(p, EZ_QOL_Reason.RATE); return; }
		SendResult(p, Park(g, p));
		EZ_QOL_Garage_RequestState(type, ctx, sender, target);
	}

	void EZ_QOL_Garage_Retrieve(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server) return;
		PlayerBase p; EZ_QOL_Garage g;
		if (!RpcContext(sender, target, p, g)) return;
		if (!RateOk(sender)) { SendResult(p, EZ_QOL_Reason.RATE); return; }
		SendResult(p, Retrieve(g, p));
		EZ_QOL_Garage_RequestState(type, ctx, sender, target);
	}

	// client
	void EZ_QOL_Garage_State(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client) return;
		Param3<bool, string, bool> d;
		if (!ctx.Read(d)) return;
		s_OnClientState.Invoke(d.param1, d.param2, d.param3);
	}

	void EZ_QOL_Garage_Result(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client) return;
		Param2<bool, string> d;
		if (!ctx.Read(d)) return;
		string icon = "set:dayz_gui image:icon_x";
		if (d.param1) icon = "set:dayz_gui image:icon_check"; // UNVERIFIED image names
		NotificationSystem.AddNotificationExtended(4, "#STR_EZ_QOL_TITLE", "#" + d.param2, icon);
		s_OnClientResult.Invoke(d.param1, d.param2);
	}

	void EZ_QOL_Garage_Config(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client) return;
		Param1<string> d;
		if (!ctx.Read(d)) return;
		EZ_QOL_GarageConfig.ApplyClientJson(d.param1);
	}
}
