// EZ_QOL per-client settings, saved in the CLIENT profile: <Documents>/DayZ/EZ_QOL_client.json
// Earplugs + vehicle flip moved here from EZLoderScreen (EZLoderScreen_client.json keeps tags/gags only).
class EZ_QOL_ClientSettings
{
	int EarplugsEnabled = 1;                      // 0 = the EZ_QOL earplug key does nothing (use another mod's earplugs)
	ref array<int> EarplugLevels = {100, 50, 25}; // cycle order, percent
	int EarplugHud = 1;
	int FlipConfirm = 1;                          // keybind flip needs a second press within 3 s
	int SettingsVersion = 1;

	static const string PATH = "$profile:EZ_QOL_client.json";
	protected static ref EZ_QOL_ClientSettings s_Inst;

	static EZ_QOL_ClientSettings Get()
	{
		if (!s_Inst)
		{
			s_Inst = new EZ_QOL_ClientSettings();
			if (FileExist(PATH))
				JsonFileLoader<EZ_QOL_ClientSettings>.JsonLoadFile(PATH, s_Inst);
			s_Inst.Sanitize();
		}
		return s_Inst;
	}

	void Sanitize()
	{
		if (!EarplugLevels || EarplugLevels.Count() == 0)
			EarplugLevels = {100, 50, 25};
		for (int i = 0; i < EarplugLevels.Count(); i++)
			EarplugLevels[i] = Math.Clamp(EarplugLevels[i], 0, 100);
	}

	void Save()
	{
		Sanitize();
		JsonFileLoader<EZ_QOL_ClientSettings>.JsonSaveFile(PATH, this);
	}
}
