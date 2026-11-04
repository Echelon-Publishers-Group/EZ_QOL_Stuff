// Tablet menu creation. Garage app intentionally NOT registered (no exit, trapped players); garage works via world actions only.
modded class MissionGameplay
{
	override void OnInit()
	{
		super.OnInit();
	}

	override UIScriptedMenu CreateScriptedMenu(int id)
	{
		if (id == EZ_QOL_MENU_TABLET)
		{
			UIScriptedMenu menu = new EZ_QOL_TabletShell();
			menu.SetID(id);
			return menu;
		}
		return super.CreateScriptedMenu(id);
	}

	// Vanilla OnKeyPress/OnUpdate treat Esc as "open pause menu" and Tab as "inventory".
	// While the tablet is open, close it and swallow the key instead.
	override void OnKeyPress(int key)
	{
		EZ_QOL_TabletShell shell = EZ_QOL_TabletShell.GetOpen();
		if (shell && (key == KeyCode.KC_ESCAPE || key == KeyCode.KC_TAB))
		{
			shell.CloseFromKey();
			return;
		}
		super.OnKeyPress(key);
	}
}
