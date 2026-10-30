// Garage tablet app: nearest planted garage within UseDistance, state, Park / Retrieve via RPC (server re-checks all).
class EZ_QOL_GarageApp : EZ_QOL_TabletApp
{
	protected Widget m_Root;
	protected TextWidget m_GarageText;
	protected TextWidget m_VehicleText;
	protected TextWidget m_LockText;
	protected TextWidget m_ResultText;
	protected ButtonWidget m_ParkBtn;
	protected ButtonWidget m_RetrieveBtn;
	protected ButtonWidget m_RefreshBtn;
	protected EZ_QOL_Garage m_Garage;

	override string GetId() { return "garage"; }
	override string GetTitle() { return "#STR_EZ_QOL_APP_GARAGE"; }
	override string GetLayout() { return "EZ_QOL_Stuff/gui/layouts/garage_app.layout"; }

	override void OnOpen(Widget root)
	{
		m_Root = root;
		m_GarageText  = TextWidget.Cast(root.FindAnyWidget("GarageText"));
		m_VehicleText = TextWidget.Cast(root.FindAnyWidget("VehicleText"));
		m_LockText    = TextWidget.Cast(root.FindAnyWidget("LockText"));
		m_ResultText  = TextWidget.Cast(root.FindAnyWidget("ResultText"));
		m_ParkBtn     = ButtonWidget.Cast(root.FindAnyWidget("ParkButton"));
		m_RetrieveBtn = ButtonWidget.Cast(root.FindAnyWidget("RetrieveButton"));
		m_RefreshBtn  = ButtonWidget.Cast(root.FindAnyWidget("RefreshButton"));
		EZ_QOL_GarageModule.s_OnClientState.Insert(OnState);
		EZ_QOL_GarageModule.s_OnClientResult.Insert(OnResult);
		Refresh();
	}

	override void OnClose()
	{
		EZ_QOL_GarageModule.s_OnClientState.Remove(OnState);
		EZ_QOL_GarageModule.s_OnClientResult.Remove(OnResult);
		m_Root = null;
		m_Garage = null;
	}

	protected EZ_QOL_Garage FindNearest()
	{
		PlayerBase p = PlayerBase.Cast(GetGame().GetPlayer());
		if (!p) return null;
		float r = EZ_QOL_GarageConfig.Get().UseDistance;
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition3D(p.GetPosition(), r, objs, null);
		EZ_QOL_Garage best;
		float bestD = r + 1;
		foreach (Object o : objs)
		{
			EZ_QOL_Garage g;
			if (Class.CastTo(g, o))
			{
				float d = vector.Distance(p.GetPosition(), g.GetPosition());
				if (d < bestD) { best = g; bestD = d; }
			}
		}
		return best;
	}

	protected void Refresh()
	{
		m_Garage = FindNearest();
		SetButtons(false, false);
		if (!m_Garage)
		{
			SetText(m_GarageText, "#STR_EZ_QOL_APP_NO_GARAGE");
			SetText(m_VehicleText, "");
			SetText(m_LockText, "");
			return;
		}
		SetText(m_GarageText, "#STR_EZ_QOL_APP_GARAGE_NEAR");
		GetRPCManager().SendRPC(EZ_QOL_RPC_NS, EZ_QOL_RPC_GARAGE_REQUESTSTATE, new Param1<int>(0), true, null, m_Garage);
	}

	void OnState(bool hasVehicle, string vehType, bool locked)
	{
		if (!m_Root) return;
		if (hasVehicle) SetText(m_VehicleText, vehType);
		else SetText(m_VehicleText, "#STR_EZ_QOL_EMPTY");
		if (locked) SetText(m_LockText, "#STR_EZ_QOL_LOCKED");
		else SetText(m_LockText, "");
		SetButtons(!hasVehicle, hasVehicle);
	}

	void OnResult(bool ok, string reasonKey)
	{
		if (!m_Root) return;
		SetText(m_ResultText, "#" + reasonKey);
	}

	override bool OnClick(Widget w)
	{
		if (!m_Root) return false;
		if (w == m_RefreshBtn) { Refresh(); return true; }
		if (!m_Garage) return false;
		if (w == m_ParkBtn)
		{
			GetRPCManager().SendRPC(EZ_QOL_RPC_NS, EZ_QOL_RPC_GARAGE_PARK, new Param1<int>(0), true, null, m_Garage);
			SetButtons(false, false);
			return true;
		}
		if (w == m_RetrieveBtn)
		{
			GetRPCManager().SendRPC(EZ_QOL_RPC_NS, EZ_QOL_RPC_GARAGE_RETRIEVE, new Param1<int>(0), true, null, m_Garage);
			SetButtons(false, false);
			return true;
		}
		return false;
	}

	protected void SetButtons(bool park, bool retrieve)
	{
		if (m_ParkBtn) m_ParkBtn.Enable(park);
		if (m_RetrieveBtn) m_RetrieveBtn.Enable(retrieve);
	}

	protected void SetText(TextWidget t, string s) { if (t) t.SetText(s); }
}
