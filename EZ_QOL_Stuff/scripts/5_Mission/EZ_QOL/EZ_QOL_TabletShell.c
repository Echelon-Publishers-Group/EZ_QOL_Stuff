// Tablet menu: app list on the home screen, one app open at a time in "AppContent".
class EZ_QOL_TabletShell : UIScriptedMenu
{
	protected static ref array<ref EZ_QOL_TabletApp> s_Apps;
	protected static EZ_QOL_TabletShell s_Open;

	protected Widget m_Home;
	protected Widget m_AppList;
	protected Widget m_AppContent;
	protected Widget m_AppRoot;
	protected TextWidget m_Title;
	protected ButtonWidget m_Back;
	protected ButtonWidget m_Close;
	protected ButtonWidget m_Exit;
	protected bool m_EscWasDown;
	protected bool m_TabWasDown;
	protected ref map<Widget, string> m_Buttons;
	protected EZ_QOL_TabletApp m_Current;
	protected bool m_InputLocked;
	protected bool m_Closing;

	static void RegisterApp(EZ_QOL_TabletApp app)
	{
		if (!app) return;
		if (!s_Apps) s_Apps = new array<ref EZ_QOL_TabletApp>;
		foreach (EZ_QOL_TabletApp a : s_Apps)
		{
			if (a.GetId() == app.GetId()) return; // already registered (mission re-init)
		}
		s_Apps.Insert(app);
	}

	static array<ref EZ_QOL_TabletApp> GetApps() { return s_Apps; }
	static EZ_QOL_TabletShell GetOpen() { return s_Open; }

	override Widget Init()
	{
		layoutRoot = GetGame().GetWorkspace().CreateWidgets("EZ_QOL_Stuff/gui/layouts/tablet_shell.layout");
		m_Home       = layoutRoot.FindAnyWidget("Home");
		m_AppList    = layoutRoot.FindAnyWidget("AppList");
		m_AppContent = layoutRoot.FindAnyWidget("AppContent");
		m_Title      = TextWidget.Cast(layoutRoot.FindAnyWidget("TitleText"));
		m_Back       = ButtonWidget.Cast(layoutRoot.FindAnyWidget("BackButton"));
		m_Close      = ButtonWidget.Cast(layoutRoot.FindAnyWidget("CloseButton"));
		m_Exit       = ButtonWidget.Cast(layoutRoot.FindAnyWidget("ExitButton"));
		m_Buttons = new map<Widget, string>;
		BuildAppList();
		ShowHome();
		s_Open = this;
		return layoutRoot;
	}

	protected void BuildAppList()
	{
		if (!s_Apps || !m_AppList) return;
		foreach (EZ_QOL_TabletApp app : s_Apps)
		{
			Widget b = GetGame().GetWorkspace().CreateWidgets("EZ_QOL_Stuff/gui/layouts/tablet_app_button.layout", m_AppList);
			TextWidget t = TextWidget.Cast(b.FindAnyWidget("AppName"));
			if (t) t.SetText(app.GetTitle());
			ImageWidget ic = ImageWidget.Cast(b.FindAnyWidget("AppIcon"));
			TextWidget gl = TextWidget.Cast(b.FindAnyWidget("AppGlyph"));
			if (ic && app.GetIcon() != "")
			{
				ic.LoadImageFile(0, app.GetIcon());
				ic.Show(true);
				if (gl) gl.Show(false);
			}
			else if (gl)
			{
				gl.SetText(AppBigLabel(app.GetTitle()));
			}
			Widget dg = b.FindAnyWidget("DroneGlyph");
			if (dg) dg.Show(app.GetId() == "drone" && app.GetIcon() == "");
			Widget btn = b.FindAnyWidget("AppButton");
			if (!btn) btn = b;
			m_Buttons.Set(btn, app.GetId());
		}
	}

	// "Drone Delivery" -> "DRONE": first word, upper-case, for the tile label under the glyph.
	static string AppBigLabel(string title)
	{
		array<string> parts = new array<string>;
		title.Split(" ", parts);
		string r = title;
		if (parts.Count() > 0 && parts[0].Length() > 0) r = parts[0];
		if (r.Length() > 8) r = r.Substring(0, 8);
		r.ToUpper();
		return r;
	}

	// "Drone Delivery" -> "DD" for the tile glyph when an app has no icon.
	static string AppInitials(string title)
	{
		array<string> parts = new array<string>;
		title.Split(" ", parts);
		string r = "";
		foreach (string p : parts)
		{
			if (p.Length() > 0 && r.Length() < 2) r = r + p.Substring(0, 1);
		}
		r.ToUpper();
		return r;
	}

	protected void SetTileHover(Widget w, bool on)
	{
		if (!w || !m_Buttons.Contains(w)) return;
		Widget tf = w.FindAnyWidget("TileFrame");
		Widget tile = w.FindAnyWidget("Tile");
		if (on)
		{
			if (tf) tf.SetColor(ARGB(255, 255, 170, 70));
			if (tile) tile.SetColor(ARGB(255, 44, 54, 66));
		}
		else
		{
			if (tf) tf.SetColor(ARGB(230, 242, 140, 31));
			if (tile) tile.SetColor(ARGB(255, 33, 41, 51));
		}
	}

	protected void SetExitHover(Widget w, bool on)
	{
		if (!w || (w != m_Exit && w != m_Close)) return;
		if (on) w.SetColor(ARGB(255, 200, 60, 50));
		else w.SetColor(ARGB(255, 140, 41, 36));
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		SetExitHover(w, true);
		SetTileHover(w, true);
		return super.OnMouseEnter(w, x, y);
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		SetExitHover(w, false);
		SetTileHover(w, false);
		return super.OnMouseLeave(w, enterW, x, y);
	}

	protected void ShowHome()
	{
		CloseCurrent();
		if (m_Home) m_Home.Show(true);
		if (m_AppContent) m_AppContent.Show(false);
		if (m_Back) m_Back.Show(false);
		if (m_Title) m_Title.SetText("#STR_EZ_QOL_TABLET");
	}

	protected void CloseCurrent()
	{
		if (m_Current) m_Current.OnClose();
		m_Current = null;
		if (m_AppRoot) m_AppRoot.Unlink();
		m_AppRoot = null;
	}

	// Drone app runs as its own full-screen menu: close the shell first (OnHide unlocks input),
	// then open EZ_QOL_MENU_DRONE on a later frame so focus counters stay balanced.
	static void OpenDroneMenuDeferred()
	{
		UIManager ui = GetGame().GetUIManager();
		if (!ui) return;
		if (ui.FindMenu(EZ_QOL_MENU_DRONE)) return;
		ui.EnterScriptedMenu(EZ_QOL_MENU_DRONE, null);
	}

	void OpenApp(string id)
	{
		if (!s_Apps) return;
		if (id == "drone")
		{
			RequestClose();
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(EZ_QOL_TabletShell.OpenDroneMenuDeferred, 50, false);
			return;
		}
		foreach (EZ_QOL_TabletApp app : s_Apps)
		{
			if (app.GetId() != id) continue;
			CloseCurrent();
			m_AppRoot = GetGame().GetWorkspace().CreateWidgets(app.GetLayout(), m_AppContent);
			if (!m_AppRoot)
			{
				EZ_QOL_Log.Warn("Tablet: layout missing for app " + id + ": " + app.GetLayout());
				return;
			}
			m_Current = app;
			if (m_Home) m_Home.Show(false);
			if (m_AppContent) m_AppContent.Show(true);
			if (m_Back) m_Back.Show(true);
			if (m_Title) m_Title.SetText(app.GetTitle());
			app.OnOpen(m_AppRoot);
			return;
		}
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if ((m_Close && w == m_Close) || (m_Exit && w == m_Exit)) { RequestClose(); return true; }
		if (w == m_Back) { ShowHome(); return true; }
		string id;
		if (m_Buttons.Find(w, id)) { OpenApp(id); return true; }
		if (m_Current && m_Current.OnClick(w)) return true;
		return super.OnClick(w, x, y, button);
	}

	void RequestClose()
	{
		if (m_Closing) return;
		m_Closing = true;
		Close(); // UIManager.HideScriptedMenu(this) -> OnHide
		// Belt and braces: if we are still the active menu, hide explicitly and make sure input/HUD come back.
		UIManager ui = GetGame().GetUIManager();
		if (ui && ui.GetMenu() == this) ui.HideScriptedMenu(this);
		UnlockInput();
		if (s_Open == this) s_Open = null;
	}

	protected void LockInput()
	{
		if (m_InputLocked) return;
		m_InputLocked = true;
		GetGame().GetMission().PlayerControlDisable(INPUT_EXCLUDE_ALL);
		GetGame().GetInput().ChangeGameFocus(1);
		GetGame().GetUIManager().ShowUICursor(true);
		// Vanilla HUD (status icons) draws over script menus; hide it while the tablet is open.
		if (GetGame().GetMission() && GetGame().GetMission().GetHud()) GetGame().GetMission().GetHud().Show(false);
	}

	protected void UnlockInput()
	{
		if (!m_InputLocked) return;
		m_InputLocked = false;
		if (GetGame().GetMission()) GetGame().GetMission().PlayerControlEnable(true);
		GetGame().GetInput().ChangeGameFocus(-1);
		GetGame().GetUIManager().ShowUICursor(false);
		if (GetGame().GetMission() && GetGame().GetMission().GetHud()) GetGame().GetMission().GetHud().Show(true);
	}

	void ~EZ_QOL_TabletShell()
	{
		UnlockInput(); // safety net if destroyed without OnHide
		if (s_Open == this) s_Open = null;
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);
		if (m_Current) m_Current.Update(timeslice);
		if (m_Closing) return;
		// Raw key-state edge detection first: works without widget focus and even when
		// INPUT_EXCLUDE_ALL suppresses UAUIBack. ClearKey stops the same Esc opening the pause menu.
		bool escDown = KeyState(KeyCode.KC_ESCAPE) != 0;
		bool tabDown = KeyState(KeyCode.KC_TAB) != 0;
		bool escPressed = escDown && !m_EscWasDown;
		bool tabPressed = tabDown && !m_TabWasDown;
		m_EscWasDown = escDown;
		m_TabWasDown = tabDown;
		if (escPressed || tabPressed)
		{
			CloseFromKey();
			return;
		}
		if (GetUApi().GetInputByID(UAUIBack).LocalPress() || GetUApi().GetInputByID(UAGear).LocalPress())
			CloseFromKey();
	}

	// Close via keyboard and swallow the key so MissionGameplay doesn't open the pause menu / inventory this frame.
	void CloseFromKey()
	{
		ClearKey(KeyCode.KC_ESCAPE);
		ClearKey(KeyCode.KC_TAB);
		RequestClose();
	}

	// Raw key fallback: works even if an input exclude swallows UAUIBack.
	override bool OnKeyDown(Widget w, int x, int y, int key)
	{
		if (key == KeyCode.KC_ESCAPE || key == KeyCode.KC_TAB)
		{
			CloseFromKey();
			return true;
		}
		return super.OnKeyDown(w, x, y, key);
	}

	override void OnShow()
	{
		super.OnShow();
		m_Closing = false;
		// treat keys already held at open (e.g. the action key combo) as not-new presses
		m_EscWasDown = KeyState(KeyCode.KC_ESCAPE) != 0;
		m_TabWasDown = KeyState(KeyCode.KC_TAB) != 0;
		LockInput();
		if (layoutRoot) SetFocus(layoutRoot); // so OnKeyDown reaches this menu
	}

	override void OnHide()
	{
		super.OnHide();
		CloseCurrent();
		UnlockInput();
		if (s_Open == this) s_Open = null;
	}
}
