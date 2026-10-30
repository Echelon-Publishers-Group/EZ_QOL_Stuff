// Tablet app: 4 category tabs, item list, price, balance, cooldown, ETA/status, wind banner.
class EZ_QOL_DroneApp: EZ_QOL_TabletApp
{
	protected Widget m_Root;
	protected ref array<ButtonWidget> m_Tabs;
	protected TextListboxWidget m_ItemList;
	protected TextWidget m_Price;
	protected TextWidget m_Balance;
	protected TextWidget m_Cooldown;
	protected TextWidget m_Status;
	protected Widget m_WindBanner;
	protected TextWidget m_WindText;
	protected ButtonWidget m_OrderBtn;
	protected string m_SelectedId;

	override string GetId() { return "drone"; }
	override string GetTitle() { return "Drone Delivery"; }
	override string GetLayout() { return "EZ_QOL_Stuff/gui/layouts/drone_app.layout"; }

	override void OnOpen(Widget root)
	{
		m_Root = root;
		m_Tabs = new array<ButtonWidget>;
		for (int i = 0; i < 4; i++)
		{
			ButtonWidget b = ButtonWidget.Cast(root.FindAnyWidget("Tab" + i.ToString()));
			m_Tabs.Insert(b);
		}
		m_ItemList   = TextListboxWidget.Cast(root.FindAnyWidget("ItemList"));
		m_Price      = TextWidget.Cast(root.FindAnyWidget("PriceText"));
		m_Balance    = TextWidget.Cast(root.FindAnyWidget("BalanceText"));
		m_Cooldown   = TextWidget.Cast(root.FindAnyWidget("CooldownText"));
		m_Status     = TextWidget.Cast(root.FindAnyWidget("StatusText"));
		m_WindBanner = root.FindAnyWidget("WindBanner");
		m_WindText   = TextWidget.Cast(root.FindAnyWidget("WindText"));
		m_OrderBtn   = ButtonWidget.Cast(root.FindAnyWidget("OrderButton"));


		EZ_QOL_DroneClient.Get().m_App = this;
		EZ_QOL_DroneClient.Get().RequestCatalog();
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(Refresh, 1000, true);
		Refresh();
	}

	// Orders/status use CF RPCs directly (EZ_QOL_DroneClient), so the shell RPC hook is unused.
	override void OnRpc(string fn, ParamsReadContext ctx) {}

	override void OnClose()
	{
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(Refresh);
		if (EZ_QOL_DroneClient.Get().m_App == this)
			EZ_QOL_DroneClient.Get().m_App = null;
		m_Root = null;
	}

	protected EZ_QOL_DroneCatalogCategory Selected()
	{
		EZ_QOL_DroneCatalog cat = EZ_QOL_DroneClient.Get().m_Catalog;
		if (!cat)
			return null;
		for (int i = 0; i < cat.categories.Count(); i++)
		{
			if (cat.categories[i].id == m_SelectedId)
				return cat.categories[i];
		}
		if (cat.categories.Count() > 0)
		{
			m_SelectedId = cat.categories[0].id;
			return cat.categories[0];
		}
		return null;
	}

	// Chief's EZ_QOL_TabletShell forwards clicks here; EZ_QOL_DroneAppMenu does the same.
	override bool OnClick(Widget w)
	{
		return OnClickWidget(w);
	}

	bool OnClickWidget(Widget w)
	{
		EZ_QOL_DroneCatalog cat = EZ_QOL_DroneClient.Get().m_Catalog;
		for (int i = 0; i < m_Tabs.Count(); i++)
		{
			if (w == m_Tabs[i] && cat && i < cat.categories.Count())
			{
				m_SelectedId = cat.categories[i].id;
				Refresh();
				return true;
			}
		}
		if (w == m_OrderBtn)
		{
			EZ_QOL_DroneCatalogCategory sel = Selected();
			if (sel)
				EZ_QOL_DroneClient.Get().PlaceOrder(sel.id);
			return true;
		}
		return false;
	}

	static string FormatTime(int sec)
	{
		int m = sec / 60;
		int s = sec % 60;
		string ss = s.ToString();
		if (s < 10)
			ss = "0" + ss;
		return m.ToString() + ":" + ss;
	}

	void Refresh()
	{
		if (!m_Root)
			return;
		EZ_QOL_DroneClient cl = EZ_QOL_DroneClient.Get();
		EZ_QOL_DroneCatalog cat = cl.m_Catalog;

		for (int t = 0; t < m_Tabs.Count(); t++)
		{
			if (!m_Tabs[t])
				continue;
			if (cat && t < cat.categories.Count())
			{
				m_Tabs[t].Show(true);
				m_Tabs[t].SetText(cat.categories[t].name);
			}
			else
			{
				m_Tabs[t].Show(false);
			}
		}

		EZ_QOL_DroneCatalogCategory sel = Selected();
		if (m_ItemList)
		{
			m_ItemList.ClearItems();
			if (sel)
			{
				for (int i = 0; i < sel.items.Count(); i++)
				{
					string line = sel.items[i].qty.ToString() + "x " + sel.items[i].displayName;
					if (sel.pickOne)
						line = "? " + sel.items[i].displayName;
					m_ItemList.AddItem(line, null, 0);
				}
			}
		}

		if (m_Price)
		{
			if (sel)
				m_Price.SetText("Price: " + sel.price.ToString());
			else
				m_Price.SetText("Price: -");
		}
		if (m_Balance)
		{
			if (cat && cat.balance >= 0)
				m_Balance.SetText("Balance: " + cat.balance.ToString());
			else
				m_Balance.SetText("Balance: -");
		}
		if (m_Cooldown)
		{
			int cd = cl.CooldownLeftNow(sel);
			if (cd > 0)
				m_Cooldown.SetText("Cooldown: " + EZ_QOL_DroneApp.FormatTime(cd));
			else
				m_Cooldown.SetText("Ready");
		}
		if (m_Status)
		{
			string st = EZ_QOL_DroneStateText(cl.m_State);
			if (cl.m_State == EZ_QOL_DRONE_STATE_INBOUND || cl.m_State == EZ_QOL_DRONE_STATE_HOVER)
			{
				int eta = cl.EtaNow();
				st = st + "  ETA " + EZ_QOL_DroneApp.FormatTime(eta);
			}
			if (cat && cat.blockedReason != "")
				st = cat.blockedReason;
			m_Status.SetText(st);
		}
		if (m_OrderBtn)
		{
			bool canOrder = sel != null && cl.CooldownLeftNow(sel) <= 0;
			if (cl.m_State == EZ_QOL_DRONE_STATE_INBOUND || cl.m_State == EZ_QOL_DRONE_STATE_HOVER)
				canOrder = false;
			if (cat && cat.blockedReason != "")
				canOrder = false;
			m_OrderBtn.Enable(canOrder);
		}
		bool warn = cl.m_WindWarning;
		if (cat && cat.windWarning)
			warn = true;
		if (m_WindBanner)
			m_WindBanner.Show(warn);
		if (m_WindText && cat)
			m_WindText.SetText("High wind / sandstorm: " + cat.windSpeed.ToString() + " m/s - drone may be grounded, delayed or crash");
	}
}
