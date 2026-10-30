// Client side RPC receiver for the drone module; holds the last catalog/status and pushes to the open app.
class EZ_QOL_DroneClient
{
	protected static ref EZ_QOL_DroneClient s_Instance;
	static bool s_AppRegistered; // RegisterApp done (shell keeps a static list)
	ref EZ_QOL_DroneCatalog m_Catalog;
	int m_State;
	int m_Eta;
	bool m_WindWarning;
	string m_LastMessage;
	int m_CatalogMs;  // GetTime() when the catalog arrived (local cooldown countdown)
	int m_StatusMs;   // GetTime() when the last status arrived (local ETA countdown)
	EZ_QOL_DroneApp m_App; // set while the app is open

	static EZ_QOL_DroneClient Get()
	{
		if (!s_Instance)
			s_Instance = new EZ_QOL_DroneClient();
		return s_Instance;
	}

	void EZ_QOL_DroneClient()
	{
		GetRPCManager().AddRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_DRONE_CATALOG, this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_DRONE_ORDERRESULT, this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_DRONE_STATUS, this, SingleplayerExecutionType.Client);
	}

	// seconds left, counted down locally since the catalog arrived
	int CooldownLeftNow(EZ_QOL_DroneCatalogCategory c)
	{
		if (!c)
			return 0;
		int left = c.cooldownLeft - (GetGame().GetTime() - m_CatalogMs) / 1000;
		if (left < 0)
			return 0;
		return left;
	}

	int EtaNow()
	{
		int left = m_Eta - (GetGame().GetTime() - m_StatusMs) / 1000;
		if (left < 0)
			return 0;
		return left;
	}

	void RequestCatalog()
	{
		GetRPCManager().SendRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_DRONE_REQUESTCATALOG, new Param1<int>(0), true);
	}

	void PlaceOrder(string categoryId)
	{
		GetRPCManager().SendRPC(EZ_QOL_DRONE_RPC_NS, EZ_QOL_RPC_DRONE_PLACEORDER, new Param1<string>(categoryId), true);
	}

	protected void Notify(string text)
	{
		m_LastMessage = text;
		NotificationSystem.AddNotificationExtended(6, "Drone Delivery", text, "set:dayz_gui image:icon_info");
	}

	void EZ_QOL_Drone_Catalog(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;
		Param1<string> data;
		if (!ctx.Read(data))
			return;
		EZ_QOL_DroneCatalog cat = new EZ_QOL_DroneCatalog();
		string err;
		JsonSerializer js = new JsonSerializer();
		if (!js.ReadFromString(cat, data.param1, err))
		{
			Print("[EZ_QOL Drone] catalog parse error: " + err);
			return;
		}
		m_Catalog = cat;
		m_CatalogMs = GetGame().GetTime();
		if (m_App)
			m_App.Refresh();
	}

	void EZ_QOL_Drone_OrderResult(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;
		Param3<bool, string, int> data;
		if (!ctx.Read(data))
			return;
		if (data.param1)
		{
			m_State = EZ_QOL_DRONE_STATE_INBOUND;
			m_Eta = data.param3;
			m_StatusMs = GetGame().GetTime();
			Notify("Order accepted. ETA " + data.param3.ToString() + "s");
		}
		else
		{
			Notify("Order refused: " + data.param2);
		}
		if (m_App)
		{
			RequestCatalog(); // refresh balance + cooldown
			m_App.Refresh();
		}
	}

	void EZ_QOL_Drone_Status(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Client)
			return;
		Param3<int, int, bool> data;
		if (!ctx.Read(data))
			return;
		bool changed = data.param1 != m_State;
		m_State = data.param1;
		m_StatusMs = GetGame().GetTime();
		m_Eta = data.param2;
		m_WindWarning = data.param3;
		if (changed)
			Notify(EZ_QOL_DroneStateText(m_State));
		if (m_App)
			m_App.Refresh();
	}
}
