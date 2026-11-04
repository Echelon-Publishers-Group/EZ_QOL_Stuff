// Tablet app: Attack Drone (lock-on C4 strike). Hosted in the tablet shell's AppContent (id "attackdrone",
// own home icon) or standalone in EZ_QOL_AttackDroneAppMenu. Server is authoritative; this is display + buttons.
class EZ_QOL_AttackDroneApp: EZ_QOL_TabletApp
{
	EZ_QOL_AttackDroneAppMenu m_HostMenu; // set only when hosted standalone

	protected Widget m_Root;
	protected TextListboxWidget m_InfoList;
	protected TextWidget m_Price;
	protected TextWidget m_Balance;
	protected TextWidget m_Cooldown;
	protected TextWidget m_Status;
	protected Widget m_Banner;
	protected TextWidget m_BannerText;
	protected Widget m_CooldownChip;
	protected ButtonWidget m_ArmBtn;
	protected ButtonWidget m_CancelBtn;
	protected bool m_ArmEnabled;
	protected bool m_ArmHover;
	protected int m_OpenMs;

	override string GetId() { return "attackdrone"; }
	override string GetTitle() { return "Attack Drone"; }
	override string GetIcon() { return "EZ_QOL_Stuff/gui/icons/ez_qol_attackdrone_icon.paa"; }
	override string GetLayout() { return "EZ_QOL_Stuff/gui/layouts/attackdrone_app.layout"; }

	override void OnOpen(Widget root)
	{
		m_Root = root;
		m_InfoList     = TextListboxWidget.Cast(root.FindAnyWidget("InfoList"));
		m_Price        = TextWidget.Cast(root.FindAnyWidget("PriceText"));
		m_Balance      = TextWidget.Cast(root.FindAnyWidget("BalanceText"));
		m_Cooldown     = TextWidget.Cast(root.FindAnyWidget("CooldownText"));
		m_Status       = TextWidget.Cast(root.FindAnyWidget("StatusText"));
		m_Banner       = root.FindAnyWidget("WindBanner");
		m_BannerText   = TextWidget.Cast(root.FindAnyWidget("WindText"));
		m_CooldownChip = root.FindAnyWidget("CooldownChip");
		m_ArmBtn       = ButtonWidget.Cast(root.FindAnyWidget("ArmButton"));
		m_CancelBtn    = ButtonWidget.Cast(root.FindAnyWidget("CancelButton"));

		EZ_QOL_StrikeClient.Get().m_App = this;
		m_OpenMs = GetGame().GetTime();
		EZ_QOL_StrikeClient.Get().RequestInfo();
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(Refresh, 1000, true);
		Refresh();
	}

	override void OnRpc(string fn, ParamsReadContext ctx) {}

	override void OnClose()
	{
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(Refresh);
		if (EZ_QOL_StrikeClient.Get().m_App == this)
			EZ_QOL_StrikeClient.Get().m_App = null;
		m_Root = null;
		m_HostMenu = null;
	}

	override bool OnClick(Widget w)
	{
		return OnClickWidget(w);
	}

	bool OnClickWidget(Widget w)
	{
		if (m_ArmBtn && w == m_ArmBtn)
		{
			if (!m_ArmEnabled)
				return true;
			EZ_QOL_StrikeClient.Get().Arm();
			CloseHost(); // the player needs their hands and view back to aim
			return true;
		}
		if (m_CancelBtn && w == m_CancelBtn)
		{
			EZ_QOL_StrikeClient.Get().Cancel();
			return true;
		}
		return false;
	}

	protected void CloseHost()
	{
		EZ_QOL_TabletShell shell = EZ_QOL_TabletShell.GetOpen();
		if (shell)
		{
			shell.RequestClose();
			return;
		}
		if (m_HostMenu)
			m_HostMenu.RequestClose();
	}

	protected void UpdateArmColor()
	{
		if (!m_ArmBtn)
			return;
		if (!m_ArmEnabled)
			m_ArmBtn.SetColor(ARGB(255, 70, 74, 80));
		else if (m_ArmHover)
			m_ArmBtn.SetColor(ARGB(255, 255, 150, 50));
		else
			m_ArmBtn.SetColor(ARGB(255, 224, 120, 20));
	}

	bool OnHoverWidget(Widget w, bool enter)
	{
		if (m_ArmBtn && w == m_ArmBtn)
		{
			m_ArmHover = enter;
			UpdateArmColor();
			return true;
		}
		return false;
	}

	void Refresh()
	{
		if (!m_Root)
			return;
		EZ_QOL_StrikeClient cl = EZ_QOL_StrikeClient.Get();

		if (m_InfoList)
		{
			m_InfoList.ClearItems();
			m_InfoList.AddItem("#STR_EZ_QOL_AD_HELP1", null, 0);
			m_InfoList.AddItem("#STR_EZ_QOL_AD_HELP2", null, 0);
			m_InfoList.AddItem("#STR_EZ_QOL_AD_HELP3", null, 0);
			m_InfoList.AddItem("#STR_EZ_QOL_AD_HELP4", null, 0);
			m_InfoList.AddItem("#STR_EZ_QOL_AD_HELP5", null, 0);
		}
		if (m_Price)
		{
			if (cl.m_HasInfo)
				m_Price.SetText("Price: " + cl.m_Price.ToString());
			else
				m_Price.SetText("Price: -");
		}
		if (m_Balance)
		{
			if (cl.m_HasInfo && cl.m_Balance >= 0)
				m_Balance.SetText("Balance: " + cl.m_Balance.ToString());
			else
				m_Balance.SetText("Balance: -");
		}
		int cd = cl.CooldownLeftNow();
		if (m_Cooldown)
		{
			if (cd > 0)
				m_Cooldown.SetText("Cooldown " + EZ_QOL_DroneApp.FormatTime(cd));
			else
				m_Cooldown.SetText("Ready");
		}
		if (m_CooldownChip)
		{
			if (cd > 0)
				m_CooldownChip.SetColor(ARGB(242, 150, 95, 20));
			else
				m_CooldownChip.SetColor(ARGB(242, 40, 115, 56));
		}

		bool active = cl.IsActive();
		string st = EZ_QOL_StrikeStateText(cl.m_State);
		if (cl.m_Reason != "")
			st = st + " (" + cl.m_Reason + ")";
		if (!cl.m_HasInfo)
		{
			st = "Connecting...";
			if (GetGame().GetTime() - m_OpenMs > 10000)
				st = "No reply from server - close and reopen the app";
		}
		if (m_Status)
			m_Status.SetText(st);

		string block = "";
		if (cl.m_HasInfo && !cl.m_Enabled)
			block = "Attack drones are disabled on this server";
		else if (cl.m_HasInfo && cl.m_BlockedReason != "")
			block = cl.m_BlockedReason;
		if (m_Banner)
			m_Banner.Show(block != "");
		if (m_BannerText && block != "")
			m_BannerText.SetText(block);

		bool canArm = cl.m_HasInfo && cl.m_Enabled && block == "" && cd <= 0 && !active;
		if (m_ArmBtn)
		{
			m_ArmBtn.Enable(canArm);
			m_ArmEnabled = canArm;
			UpdateArmColor();
		}
		if (m_CancelBtn)
			m_CancelBtn.Show(active);
	}
}
