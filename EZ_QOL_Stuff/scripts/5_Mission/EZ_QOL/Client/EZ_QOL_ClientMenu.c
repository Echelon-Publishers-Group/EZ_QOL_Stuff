// EZ_QOL client settings menu (earplugs + vehicle flip). Opened from the "EZ QOL settings" button
// injected into DayZ Options, or with the EZ_QOL settings keybind. Saved to the client profile.
const int EZ_QOL_MENU_SETTINGS = 47320;

class EZ_QOL_SettingsMenu : UIScriptedMenu
{
	override Widget Init()
	{
		layoutRoot = GetGame().GetWorkspace().CreateWidgets("EZ_QOL_Stuff/gui/layouts/ez_qol_settings.layout");
		Refresh();
		return layoutRoot;
	}

	protected void SetBtn(string id, string text)
	{
		ButtonWidget b = ButtonWidget.Cast(layoutRoot.FindAnyWidget("EZQOLBtn_" + id));
		if (b)
			b.SetText(text);
	}

	protected string OnOff(int v)
	{
		if (v)
			return "ON";
		return "OFF";
	}

	void Refresh()
	{
		EZ_QOL_ClientSettings s = EZ_QOL_ClientSettings.Get();
		string lv = "";
		foreach (int l : s.EarplugLevels)
			lv = lv + l.ToString() + "% ";
		SetBtn("EarplugsEnabled", "Earplug key: " + OnOff(s.EarplugsEnabled));
		SetBtn("Earplugs", "Earplug levels: " + lv);
		SetBtn("EarplugHud", "Earplug HUD indicator: " + OnOff(s.EarplugHud));
		SetBtn("FlipConfirm", "Flip key needs confirm press: " + OnOff(s.FlipConfirm));
		SetBtn("Close", "Save & close");
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		EZ_QOL_ClientSettings s = EZ_QOL_ClientSettings.Get();
		string n = w.GetName();
		if (n == "EZQOLBtn_EarplugsEnabled")
		{
			s.EarplugsEnabled = !s.EarplugsEnabled;
			MissionGameplay mg = MissionGameplay.Cast(GetGame().GetMission());
			if (mg && !s.EarplugsEnabled)
				mg.EZ_QOL_ResetEarplugs();
		}
		else if (n == "EZQOLBtn_Earplugs")
		{
			// presets: 100/50/25 -> 100/30/10 -> 100/60 -> 100/20
			if (s.EarplugLevels.Count() == 3 && s.EarplugLevels[1] == 50) s.EarplugLevels = {100, 30, 10};
			else if (s.EarplugLevels.Count() == 3) s.EarplugLevels = {100, 60};
			else if (s.EarplugLevels.Count() > 1 && s.EarplugLevels[1] == 60) s.EarplugLevels = {100, 20};
			else s.EarplugLevels = {100, 50, 25};
		}
		else if (n == "EZQOLBtn_EarplugHud") s.EarplugHud = !s.EarplugHud;
		else if (n == "EZQOLBtn_FlipConfirm") s.FlipConfirm = !s.FlipConfirm;
		else if (n == "EZQOLBtn_Close") { s.Save(); Close(); return true; }
		else return false;
		s.Save();
		Refresh();
		return true;
	}
}

// Inject an "EZ QOL settings" button into DayZ Options (placed below EZLoderScreen's button if both mods run).
modded class OptionsMenu
{
	protected ButtonWidget m_EZ_QOL_Btn;

	override Widget Init()
	{
		Widget r = super.Init();
		if (r)
			m_EZ_QOL_Btn = ButtonWidget.Cast(GetGame().GetWorkspace().CreateWidgets("EZ_QOL_Stuff/gui/layouts/ez_qol_optbutton.layout", r));
		return r;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_EZ_QOL_Btn && w == m_EZ_QOL_Btn)
		{
			GetGame().GetUIManager().EnterScriptedMenu(EZ_QOL_MENU_SETTINGS, this);
			return true;
		}
		return super.OnClick(w, x, y, button);
	}
}

modded class MissionBase
{
	override UIScriptedMenu CreateScriptedMenu(int id)
	{
		if (id == EZ_QOL_MENU_SETTINGS)
		{
			EZ_QOL_SettingsMenu m = new EZ_QOL_SettingsMenu();
			m.SetID(id);
			return m;
		}
		return super.CreateScriptedMenu(id);
	}
}
