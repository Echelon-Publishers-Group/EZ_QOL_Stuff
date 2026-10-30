// Drone-only fallback host for EZ_QOL_DroneApp while Chief's EZ_QOL_TabletShell is a stub.
// Not the shell: it hosts only the drone app, using the same TabletApp interface.
class EZ_QOL_DroneAppMenu: UIScriptedMenu
{
	protected ref EZ_QOL_DroneApp m_App;

	override Widget Init()
	{
		m_App = new EZ_QOL_DroneApp();
		layoutRoot = GetGame().GetWorkspace().CreateWidgets(m_App.GetLayout());
		if (layoutRoot)
		{
			// layout root is 1x1 relative (fills the shell's app frame); centre it when hosted standalone
			layoutRoot.SetPos(0.2, 0.15);
			layoutRoot.SetSize(0.6, 0.7);
			m_App.OnOpen(layoutRoot);
		}
		return layoutRoot;
	}

	void ~EZ_QOL_DroneAppMenu()
	{
		if (m_App)
			m_App.OnClose();
		if (layoutRoot)
			layoutRoot.Unlink();
	}

	override void OnShow()
	{
		super.OnShow();
		GetGame().GetMission().PlayerControlDisable(INPUT_EXCLUDE_INVENTORY);
		GetGame().GetUIManager().ShowCursor(true);
		GetGame().GetInput().ChangeGameFocus(1);
	}

	override void OnHide()
	{
		super.OnHide();
		GetGame().GetMission().PlayerControlEnable(false);
		GetGame().GetUIManager().ShowCursor(false);
		GetGame().GetInput().ResetGameFocus();
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);
		if (GetUApi().GetInputByID(UAUIBack).LocalPress())
			Close();
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_App && m_App.OnClickWidget(w))
			return true;
		return super.OnClick(w, x, y, button);
	}
}
