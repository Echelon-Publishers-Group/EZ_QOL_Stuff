// Attack drone client: RPC receiver, HUD (reticle, lock bar, grace bar, status/ETA), tones, incoming warning,
// and the aim hint sender. The hint is ONLY a hint: the server raycasts the aim itself (EZ_QOL_StrikeManager).
class EZ_QOL_StrikeClient
{
	protected static ref EZ_QOL_StrikeClient s_Instance;
	static bool s_AppRegistered;

	// info (InfoReply)
	bool m_HasInfo;
	int m_Price;
	int m_CooldownLeft;
	int m_CooldownAtMs;
	int m_Balance = -1;
	string m_BlockedReason;
	bool m_Enabled = true;

	// strike state (State)
	int m_State;
	float m_LockPct;
	float m_GracePct = 1;
	int m_Eta = -1;
	string m_Reason;
	int m_StateAtMs;

	EZ_QOL_AttackDroneApp m_App;

	protected Widget m_Hud;
	protected Widget m_Reticle;
	protected Widget m_HudPanel;
	protected TextWidget m_HudStatus;
	protected Widget m_LockFill;
	protected Widget m_GraceFill;
	protected Widget m_WarnBanner;
	protected TextWidget m_WarnText;
	protected int m_WarnUntilMs;
	protected int m_WarnEta;
	protected int m_WarnAtMs;

	protected float m_HintTimer;
	protected float m_BeepTimer;
	protected bool m_LockedToneDone;

	static EZ_QOL_StrikeClient Get()
	{
		if (!s_Instance)
			s_Instance = new EZ_QOL_StrikeClient();
		return s_Instance;
	}

	void EZ_QOL_StrikeClient()
	{
		GetRPCManager().AddRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_STRIKE_INFOREPLY, this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_STRIKE_STATE, this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_STRIKE_WARN, this, SingleplayerExecutionType.Client);
	}

	// ---------- requests ----------
	void RequestInfo()
	{
		GetRPCManager().SendRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_STRIKE_INFO, new Param1<int>(0), true);
	}

	void Arm()
	{
		GetRPCManager().SendRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_STRIKE_ARM, new Param1<int>(0), true);
	}

	void Cancel()
	{
		GetRPCManager().SendRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_STRIKE_CANCEL, new Param1<int>(0), true);
	}

	bool IsActive()
	{
		if (m_State == EZ_QOL_STRIKE_ARMED || m_State == EZ_QOL_STRIKE_ACQUIRING)
			return true;
		if (m_State == EZ_QOL_STRIKE_INFLIGHT || m_State == EZ_QOL_STRIKE_DIVE)
			return true;
		return false;
	}

	int CooldownLeftNow()
	{
		int left = m_CooldownLeft - (GetGame().GetTime() - m_CooldownAtMs) / 1000;
		if (left < 0)
			left = 0;
		return left;
	}

	int EtaNow()
	{
		if (m_Eta < 0)
			return -1;
		int left = m_Eta - (GetGame().GetTime() - m_StateAtMs) / 1000;
		if (left < 0)
			left = 0;
		return left;
	}

	// ---------- RPC handlers ----------
	void EZ_QOL_Strike_InfoReply(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;
		Param5<int, int, int, string, bool> data;
		if (!ctx.Read(data))
			return;
		m_HasInfo = true;
		m_Price = data.param1;
		m_CooldownLeft = data.param2;
		m_CooldownAtMs = GetGame().GetTime();
		m_Balance = data.param3;
		m_BlockedReason = data.param4;
		m_Enabled = data.param5;
		if (m_App)
			m_App.Refresh();
	}

	void EZ_QOL_Strike_State(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;
		Param5<int, float, float, int, string> data;
		if (!ctx.Read(data))
			return;
		int prev = m_State;
		m_State = data.param1;
		m_LockPct = data.param2;
		m_GracePct = data.param3;
		m_Eta = data.param4;
		m_Reason = data.param5;
		m_StateAtMs = GetGame().GetTime();

		if (prev != m_State)
			OnStateChanged(prev);
		if (m_App)
			m_App.Refresh();
	}

	void EZ_QOL_Strike_Warn(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;
		Param2<vector, int> data;
		if (!ctx.Read(data))
			return;
		m_WarnEta = data.param2;
		m_WarnAtMs = GetGame().GetTime();
		m_WarnUntilMs = m_WarnAtMs + (data.param2 + 3) * 1000;
		EnsureHud();
		PlayAt("UndergroundDoor_Alarm_Start_SoundSet");
		NotificationSystem.AddNotificationExtended(6, "#STR_EZ_QOL_AD_INCOMING", "#STR_EZ_QOL_AD_INCOMING_DETAIL", "");
	}

	protected void OnStateChanged(int prev)
	{
		m_LockedToneDone = false;
		if (m_State == EZ_QOL_STRIKE_ARMED)
			PlayAt("digitalLock_beep_Access_SoundSet");
		if (m_State == EZ_QOL_STRIKE_INFLIGHT)
			PlayAt("digitalLock_beep_Access_SoundSet");
		if (m_State == EZ_QOL_STRIKE_ABORTED)
			PlayAt("digitalLock_beep_Error_SoundSet");
		if (m_State == EZ_QOL_STRIKE_ABORTED || m_State == EZ_QOL_STRIKE_HIT)
		{
			string msg = EZ_QOL_StrikeStateText(m_State);
			if (m_Reason != "")
				msg = m_Reason;
			NotificationSystem.AddNotificationExtended(5, "#STR_EZ_QOL_APP_ATTACKDRONE", msg, "");
			RequestInfo(); // refresh cooldown/balance
		}
	}

	protected void PlayAt(string soundSet)
	{
		PlayerBase pb = PlayerBase.Cast(GetGame().GetPlayer());
		if (!pb)
			return;
		EffectSound snd = SEffectManager.PlaySound(soundSet, pb.GetPosition());
		if (snd)
			snd.SetSoundAutodestroy(true);
	}

	// ---------- HUD ----------
	protected void EnsureHud()
	{
		if (m_Hud)
			return;
		m_Hud = GetGame().GetWorkspace().CreateWidgets("EZ_QOL_Stuff/gui/layouts/attackdrone_hud.layout");
		if (!m_Hud)
			return;
		m_Reticle    = m_Hud.FindAnyWidget("Reticle");
		m_HudPanel   = m_Hud.FindAnyWidget("HudPanel");
		m_HudStatus  = TextWidget.Cast(m_Hud.FindAnyWidget("HudStatus"));
		m_LockFill   = m_Hud.FindAnyWidget("LockBarFill");
		m_GraceFill  = m_Hud.FindAnyWidget("GraceBarFill");
		m_WarnBanner = m_Hud.FindAnyWidget("WarnBanner");
		m_WarnText   = TextWidget.Cast(m_Hud.FindAnyWidget("WarnText"));
	}

	protected void DestroyHud()
	{
		if (m_Hud)
			m_Hud.Unlink();
		m_Hud = null;
		m_Reticle = null;
		m_HudPanel = null;
		m_HudStatus = null;
		m_LockFill = null;
		m_GraceFill = null;
		m_WarnBanner = null;
		m_WarnText = null;
	}

	// Called every frame from MissionGameplay.OnUpdate.
	void OnUpdate(float dt)
	{
		int now = GetGame().GetTime();
		bool active = IsActive();
		bool showEnd = (m_State == EZ_QOL_STRIKE_HIT || m_State == EZ_QOL_STRIKE_ABORTED) && now - m_StateAtMs < 2500;
		bool warning = now < m_WarnUntilMs;

		if (!active && !showEnd && !warning)
		{
			if (m_Hud)
				DestroyHud();
			return;
		}
		EnsureHud();
		if (!m_Hud)
			return;

		bool menuOpen = GetGame().GetUIManager().GetMenu() != null;
		if (m_Reticle)
			m_Reticle.Show(active && !menuOpen);
		if (m_HudPanel)
			m_HudPanel.Show(active || showEnd);

		if (m_HudStatus)
		{
			string st = EZ_QOL_StrikeStateText(m_State);
			int eta = EtaNow();
			if ((m_State == EZ_QOL_STRIKE_INFLIGHT || m_State == EZ_QOL_STRIKE_DIVE) && eta >= 0)
				st = st + "  ETA " + eta.ToString() + "s";
			if (m_Reason != "")
				st = st + "  (" + m_Reason + ")";
			m_HudStatus.SetText(st);
		}
		if (m_LockFill)
		{
			float lp = Math.Clamp(m_LockPct, 0, 1);
			m_LockFill.SetSize(lp, 1);
			if (lp >= 1)
				m_LockFill.SetColor(ARGB(255, 224, 120, 20));
			else
				m_LockFill.SetColor(ARGB(255, 242, 140, 31));
		}
		if (m_GraceFill)
		{
			float gp = Math.Clamp(m_GracePct, 0, 1);
			m_GraceFill.SetSize(gp, 1);
			if (gp < 0.5)
				m_GraceFill.SetColor(ARGB(255, 140, 41, 36));
			else
				m_GraceFill.SetColor(ARGB(255, 140, 230, 140));
		}
		if (m_Reticle)
		{
			if (m_LockPct >= 1)
				SetReticleColor(ARGB(255, 140, 230, 140));
			else
				SetReticleColor(ARGB(255, 242, 140, 31));
		}
		if (m_WarnBanner)
			m_WarnBanner.Show(warning);
		if (warning && m_WarnText)
		{
			int wleft = m_WarnEta - (now - m_WarnAtMs) / 1000;
			if (wleft < 0)
				wleft = 0;
			m_WarnText.SetText("INCOMING DRONE  " + wleft.ToString() + "s");
		}

		if (active)
		{
			Tones(dt);
			m_HintTimer += dt;
			if (m_HintTimer >= 0.2)
			{
				m_HintTimer = 0;
				SendHint();
			}
		}
	}

	protected void SetReticleColor(int col)
	{
		array<string> names = new array<string>;
		names.Insert("RetTop");
		names.Insert("RetBottom");
		names.Insert("RetLeft");
		names.Insert("RetRight");
		for (int i = 0; i < names.Count(); i++)
		{
			Widget w = m_Reticle.FindAnyWidget(names[i]);
			if (w)
				w.SetColor(col);
		}
	}

	// Beep faster as the lock builds; one confirm tone at full lock; error tone when grace is low.
	protected void Tones(float dt)
	{
		if (m_State == EZ_QOL_STRIKE_ACQUIRING)
		{
			float interval = 0.8 - 0.6 * Math.Clamp(m_LockPct, 0, 1);
			m_BeepTimer += dt;
			if (m_BeepTimer >= interval)
			{
				m_BeepTimer = 0;
				PlayAt("digitalLock_beep_Access_SoundSet");
			}
			return;
		}
		if ((m_State == EZ_QOL_STRIKE_INFLIGHT || m_State == EZ_QOL_STRIKE_DIVE) && m_GracePct < 0.99)
		{
			m_BeepTimer += dt;
			if (m_BeepTimer >= 0.5)
			{
				m_BeepTimer = 0;
				PlayAt("digitalLock_beep_Error_SoundSet");
			}
		}
	}

	// Camera-ray hint (5 Hz). The server re-does the aim from the player's own weapon direction.
	protected void SendHint()
	{
		PlayerBase pb = PlayerBase.Cast(GetGame().GetPlayer());
		if (!pb)
			return;
		vector from = GetGame().GetCurrentCameraPosition();
		vector dir = GetGame().GetCurrentCameraDirection();
		vector to = from + dir * 450;
		vector hitPos;
		vector hitDir;
		int hitComp;
		set<Object> results = new set<Object>;
		int low = 0;
		int high = 0;
		bool hasObj = false;
		vector ground = to;
		if (DayZPhysics.RaycastRV(from, to, hitPos, hitDir, hitComp, results, null, pb, true, false, ObjIntersectFire, 0.1))
		{
			ground = hitPos;
			for (int i = 0; i < results.Count(); i++)
			{
				Object o = results[i];
				if (!o || o == pb)
					continue;
				o.GetNetworkID(low, high);
				hasObj = true;
				break;
			}
		}
		Param4<int, int, vector, bool> p = new Param4<int, int, vector, bool>(low, high, ground, hasObj);
		GetRPCManager().SendRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_STRIKE_CANDIDATE, p, true);
	}
}
