// Drone module log -> $profile:EZ_QOL/logs/drone_YYYY-MM-DD.log (separate from the shared EZ_QOL_Log.c)
class EZ_QOL_DroneLog
{
	static bool s_Enabled = true;

	static void EnsureDirs()
	{
		if (!FileExist("$profile:EZ_QOL"))
			MakeDirectory("$profile:EZ_QOL");
		if (!FileExist("$profile:EZ_QOL/logs"))
			MakeDirectory("$profile:EZ_QOL/logs");
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
		Print("[EZ_QOL Drone] " + line);
		if (!s_Enabled || !GetGame() || !GetGame().IsServer())
			return;
		EnsureDirs();
		FileHandle fh = OpenFile("$profile:EZ_QOL/logs/drone_" + Stamp(true) + ".log", FileMode.APPEND);
		if (fh != 0)
		{
			FPrintln(fh, line);
			CloseFile(fh);
		}
	}

	static void Info(string msg)  { Write("INFO", msg); }
	static void Warn(string msg)  { Write("WARN", msg); }
	static void Err(string msg)   { Write("ERROR", msg); }
}
