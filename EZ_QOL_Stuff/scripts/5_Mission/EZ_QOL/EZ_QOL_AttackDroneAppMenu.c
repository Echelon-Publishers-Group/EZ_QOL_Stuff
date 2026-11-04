// Standalone host for EZ_QOL_AttackDroneApp (menu EZ_QOL_MENU_ATTACKDRONE). Copy of EZ_QOL_DroneAppMenu's pattern.
class EZ_QOL_AttackDroneAppMenu: UIScriptedMenu
{
	protected ref EZ_QOL_AttackDroneApp m_App;
	protected ButtonWidget m_CloseBtn;
	protected bool m_InputLocked;
	protected bool m_Closing;

	override Widget Init()
	{
		m_App = new EZ_QOL_AttackDroneApp();
		layoutRoot = GetGame().GetWorkspace().CreateWidgets(m_App.GetLayout());
		if (layoutRoot)
		{
			// layout root is 1x1 relative (fills the shell's app frame); centre it when hosted standalone
			layoutRoot.SetPos(0.2, 0.15);
			layoutRoot.SetSize(0.6, 0.7);
			m_CloseBtn = ButtonWidget.Cast(layoutRoot.FindAnyWidget("CloseButton"));
			if (m_CloseBtn)
				m_CloseBtn.Show(true); // hidden by default so the tablet shell (own X) doesn't show two
			m_App.m_HostMenu = this;
			m_App.OnOpen(layoutRoot);
		}
		return layoutRoot;
	}

	void ~EZ_QOL_AttackDroneAppMenu()
	{
		UnlockInput(); // safety net if the menu is destroyed without OnHide
		if (m_App)
			m_App.OnClose();
		if (layoutRoot)
			layoutRoot.Unlink();
	}

	protected void LockInput()
	{
		if (m_InputLocked)
			return;
		m_InputLocked = true;
		GetGame().GetMission().PlayerControlDisable(INPUT_EXCLUDE_ALL);
		GetGame().GetInput().ChangeGameFocus(1);
		GetGame().GetUIManager().ShowUICursor(true);
	}

	protected void UnlockInput()
	{
		if (!m_InputLocked)
			return;
		m_InputLocked = false;
		if (GetGame().GetMission())
			GetGame().GetMission().PlayerControlEnable(true);
		GetGame().GetInput().ChangeGameFocus(-1);
		GetGame().GetUIManager().ShowUICursor(false);
	}

	void RequestClose()
	{
		if (m_Closing)
			return;
		m_Closing = true;
		Close(); // UIManager.HideScriptedMenu(this) -> OnHide
	}

	override void OnShow()
	{
		super.OnShow();
		m_Closing = false;
		LockInput();
	}

	override void OnHide()
	{
		super.OnHide();
		UnlockInput();
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);
		if (m_Closing)
			return;
		// Esc / Tab (inventory). UAUIBack/UAGear are UAInput press checks, polled each frame here.
		if (GetUApi().GetInputByID(UAUIBack).LocalPress() || GetUApi().GetInputByID(UAGear).LocalPress())
			RequestClose();
	}

	// Raw key fallback: works even if an input exclude swallows UAUIBack.
	override bool OnKeyDown(Widget w, int x, int y, int key)
	{
		if (key == KeyCode.KC_ESCAPE || key == KeyCode.KC_TAB)
		{
			RequestClose();
			return true;
		}
		return super.OnKeyDown(w, x, y, key);
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (m_App && m_App.OnHoverWidget(w, true))
			return true;
		return super.OnMouseEnter(w, x, y);
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (m_App && m_App.OnHoverWidget(w, false))
			return true;
		return super.OnMouseLeave(w, enterW, x, y);
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (m_CloseBtn && w == m_CloseBtn)
		{
			RequestClose();
			return true;
		}
		if (m_App && m_App.OnClickWidget(w))
			return true;
		return super.OnClick(w, x, y, button);
	}
}
