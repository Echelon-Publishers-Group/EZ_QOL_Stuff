// Shared log -> $profile:EZ_QOL/logs/ezqol_YYYY-MM-DD.log (drone has its own EZ_QOL_DroneLog)
class EZ_QOL_Log
{
	static void EnsureDir(string path)
	{
		if (!FileExist(path))
			MakeDirectory(path);
	}

	static void EnsureDirs()
	{
		EnsureDir(EZ_QOL_PROFILE_DIR);
		EnsureDir(EZ_QOL_PROFILE_DIR + "/logs");
		EnsureDir(EZ_QOL_GARAGE_DIR);
		EnsureDir(EZ_QOL_GARAGE_ORPHANS);
	}

	static string Stamp(bool dateOnly)
	{
		int y, mo, d, h, mi, s;
		GetYearMonthDay(y, mo, d);
		GetHourMinuteSecond(h, mi, s);
		string date = y.ToString() + "-" + mo.ToStringLen(2) + "-" + d.ToStringLen(2);
		if (dateOnly)
			return date;
		return date + " " + h.ToStringLen(2) + ":" + mi.ToStringLen(2) + ":" + s.ToStringLen(2);
	}

	static void Write(string level, string msg)
	{
		string line = "[" + Stamp(false) + "] [" + level + "] " + msg;
		Print("[EZ_QOL] " + line);
		if (!GetGame() || !GetGame().IsServer())
			return;
		EnsureDirs();
		FileHandle fh = OpenFile(EZ_QOL_PROFILE_DIR + "/logs/ezqol_" + Stamp(true) + ".log", FileMode.APPEND);
		if (fh != 0)
		{
			FPrintln(fh, line);
			CloseFile(fh);
		}
	}

	static void Info(string msg) { Write("INFO", msg); }
	static void Warn(string msg) { Write("WARN", msg); }
	static void Err(string msg)  { Write("ERROR", msg); }
}
