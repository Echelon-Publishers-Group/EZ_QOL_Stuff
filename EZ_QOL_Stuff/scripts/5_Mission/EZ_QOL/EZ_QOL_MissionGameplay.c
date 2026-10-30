// Tablet menu creation + garage app registration. Super-calling; ClearcREEk's DroneMissionHooks registers the drone app.
modded class MissionGameplay
{
	override void OnInit()
	{
		super.OnInit();
		EZ_QOL_TabletShell.RegisterApp(new EZ_QOL_GarageApp());
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
}
