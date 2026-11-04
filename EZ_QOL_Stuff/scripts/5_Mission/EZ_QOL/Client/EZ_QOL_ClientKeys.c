// EZ_QOL keybinds (moved from EZLoderScreen EZ_HoloClient.c): vehicle flip, earplugs, settings menu.
// Input names live in data/inputs.xml (group "EZ_QOL"); players can rebind under Controls.
modded class MissionGameplay
{
	int m_EZ_QOL_EarIdx;            // index into EarplugLevels; 0 = first level (normally 100%)
	float m_EZ_QOL_OrigVolume = -1;
	Widget m_EZ_QOL_EarHud;
	float m_EZ_QOL_FlipArmed;

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (m_EZ_QOL_FlipArmed > 0)
			m_EZ_QOL_FlipArmed -= timeslice;
		if (GetGame().GetUIManager().GetMenu())
			return;

		PlayerBase me = PlayerBase.Cast(GetGame().GetPlayer());
		if (EZ_QOL_Pressed("UAEZQOLEarplugs"))
			EZ_QOL_CycleEarplugs();
		if (EZ_QOL_Pressed("UAEZQOLSettings"))
			GetGame().GetUIManager().EnterScriptedMenu(EZ_QOL_MENU_SETTINGS, null);
		if (me && EZ_QOL_Pressed("UAEZQOLFlip"))
			EZ_QOL_FlipKey(me);
	}

	protected bool EZ_QOL_Pressed(string name)
	{
		UAInput inp = GetUApi().GetInputByName(name);
		return inp && inp.LocalPress();
	}

	void EZ_QOL_FlipKey(PlayerBase me)
	{
		CarScript car;
		vector from = GetGame().GetCurrentCameraPosition();
		vector to = from + GetGame().GetCurrentCameraDirection() * 5;
		vector hitPos, hitNormal;
		int comp;
		set<Object> hits = new set<Object>;
		if (DayZPhysics.RaycastRV(from, to, hitPos, hitNormal, comp, hits, null, me, false, false, ObjIntersectView, 0.3))
		{
			foreach (Object o : hits)
			{
				car = CarScript.Cast(o);
				if (car)
					break;
			}
		}
		if (!car || !EZ_QOL_Flip.CanFlip(me, car))
		{
			NotificationSystem.AddNotificationExtended(3, "Flip vehicle", "Look at an overturned, empty vehicle with the engine off.", "set:dayz_gui image:icon_info");
			return;
		}
		if (EZ_QOL_ClientSettings.Get().FlipConfirm && m_EZ_QOL_FlipArmed <= 0)
		{
			m_EZ_QOL_FlipArmed = 3.0;
			NotificationSystem.AddNotificationExtended(3, "Flip vehicle", "Press again within 3 s to flip.", "set:dayz_gui image:icon_info");
			return;
		}
		m_EZ_QOL_FlipArmed = 0;
		me.RPCSingleParam(EZ_QOL_RPC_FLIP_REQUEST, new Param1<Object>(car), true);
	}

	void EZ_QOL_CycleEarplugs()
	{
		EZ_QOL_ClientSettings s = EZ_QOL_ClientSettings.Get();
		if (!s.EarplugsEnabled)
			return;
		AbstractSoundScene scene = GetGame().GetSoundScene();
		if (!scene)
			return;
		if (m_EZ_QOL_OrigVolume < 0)
			m_EZ_QOL_OrigVolume = scene.GetSoundVolume();
		m_EZ_QOL_EarIdx = (m_EZ_QOL_EarIdx + 1) % s.EarplugLevels.Count();
		int pct = s.EarplugLevels[m_EZ_QOL_EarIdx];
		scene.SetSoundVolume(m_EZ_QOL_OrigVolume * pct / 100.0, 0.5);
		string txt = "Earplugs OUT (" + pct.ToString() + "%)";
		if (pct < 100)
			txt = "Earplugs IN - game volume " + pct.ToString() + "%";
		NotificationSystem.AddNotificationExtended(2, "Earplugs", txt, "set:dayz_gui image:icon_info");
		if (!m_EZ_QOL_EarHud)
			m_EZ_QOL_EarHud = GetGame().GetWorkspace().CreateWidgets("EZ_QOL_Stuff/gui/layouts/ez_qol_earhud.layout");
		if (m_EZ_QOL_EarHud)
		{
			TextWidget t = TextWidget.Cast(m_EZ_QOL_EarHud);
			if (t)
				t.SetText("EARPLUGS " + pct.ToString() + "%");
			m_EZ_QOL_EarHud.Show(pct < 100 && s.EarplugHud);
		}
	}

	// Called when the player switches the earplug key off in the menu: restore full volume.
	void EZ_QOL_ResetEarplugs()
	{
		AbstractSoundScene scene = GetGame().GetSoundScene();
		if (scene && m_EZ_QOL_OrigVolume >= 0)
			scene.SetSoundVolume(m_EZ_QOL_OrigVolume, 0.5);
		m_EZ_QOL_EarIdx = 0;
		if (m_EZ_QOL_EarHud)
			m_EZ_QOL_EarHud.Show(false);
	}

	override void OnMissionFinish()
	{
		AbstractSoundScene scene = GetGame().GetSoundScene();
		if (scene && m_EZ_QOL_OrigVolume >= 0)
			scene.SetSoundVolume(m_EZ_QOL_OrigVolume, 0);
		if (m_EZ_QOL_EarHud)
			m_EZ_QOL_EarHud.Unlink();
		super.OnMissionFinish();
	}
}
