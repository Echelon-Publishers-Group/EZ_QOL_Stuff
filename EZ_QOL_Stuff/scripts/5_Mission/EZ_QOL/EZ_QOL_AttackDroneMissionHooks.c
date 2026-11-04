// Attack drone hooks. Own file so Chief's mission files and the delivery-drone hooks stay untouched.
modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();
		EZ_QOL_StrikeManager.Get().Init();
	}
}

modded class MissionGameplay
{
	override void OnInit()
	{
		super.OnInit();
		EZ_QOL_StrikeClient.Get();
		// RegisterApp keeps a static list (dedupes by id); register once per game session
		if (!EZ_QOL_StrikeClient.s_AppRegistered)
		{
			EZ_QOL_StrikeClient.s_AppRegistered = true;
			EZ_QOL_TabletShell.RegisterApp(new EZ_QOL_AttackDroneApp());
		}
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		EZ_QOL_StrikeClient.Get().OnUpdate(timeslice);
	}

	override UIScriptedMenu CreateScriptedMenu(int id)
	{
		UIScriptedMenu menu = super.CreateScriptedMenu(id);
		if (!menu && id == EZ_QOL_MENU_ATTACKDRONE)
		{
			menu = new EZ_QOL_AttackDroneAppMenu();
			menu.SetID(id);
		}
		return menu;
	}
}
