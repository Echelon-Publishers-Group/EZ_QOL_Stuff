// Drone module hooks. Kept in their own file so Chief's MissionServer.c / MissionGameplay.c stay untouched.
modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();
		EZ_QOL_DroneManager.Get().Init();
	}
}

modded class MissionGameplay
{
	override void OnInit()
	{
		super.OnInit();
		EZ_QOL_DroneClient.Get();
		// RegisterApp keeps a static list: register once per game session, not per reconnect
		if (!EZ_QOL_DroneClient.s_AppRegistered)
		{
			EZ_QOL_DroneClient.s_AppRegistered = true;
			EZ_QOL_TabletShell.RegisterApp(new EZ_QOL_DroneApp());
		}
	}

	// Drone-only fallback menu (opened by EZ_QOL_ActionDroneOpenApp). Chief's tablet id 47310 is untouched.
	override UIScriptedMenu CreateScriptedMenu(int id)
	{
		UIScriptedMenu menu = super.CreateScriptedMenu(id);
		if (!menu && id == EZ_QOL_MENU_DRONE)
		{
			menu = new EZ_QOL_DroneAppMenu();
			menu.SetID(id);
		}
		return menu;
	}
}
