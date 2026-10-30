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
	protected ref map<Widget, string> m_Buttons;
	protected EZ_QOL_TabletApp m_Current;

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
			if (ic && app.GetIcon() != "") ic.LoadImageFile(0, app.GetIcon());
			Widget btn = b.FindAnyWidget("AppButton");
			if (!btn) btn = b;
			m_Buttons.Set(btn, app.GetId());
		}
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

	void OpenApp(string id)
	{
		if (!s_Apps) return;
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
		if (w == m_Close) { Close(); return true; }
		if (w == m_Back) { ShowHome(); return true; }
		string id;
		if (m_Buttons.Find(w, id)) { OpenApp(id); return true; }
		if (m_Current && m_Current.OnClick(w)) return true;
		return super.OnClick(w, x, y, button);
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);
		if (m_Current) m_Current.Update(timeslice);
		if (GetGame().GetInput().LocalPress("UAUIBack", false)) Close();
	}

	override void OnShow()
	{
		super.OnShow();
		GetGame().GetMission().PlayerControlDisable(INPUT_EXCLUDE_ALL);
		GetGame().GetUIManager().ShowUICursor(true);
	}

	override void OnHide()
	{
		super.OnHide();
		CloseCurrent();
		GetGame().GetMission().PlayerControlEnable(true);
		GetGame().GetUIManager().ShowUICursor(false);
		if (s_Open == this) s_Open = null;
	}
}
