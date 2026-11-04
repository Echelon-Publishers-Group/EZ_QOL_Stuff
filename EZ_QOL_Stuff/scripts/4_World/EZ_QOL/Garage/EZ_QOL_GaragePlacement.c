// Shared client/server placement check. Server re-runs it before creating the garage.
class EZ_QOL_GaragePlacement
{
	static bool IsInZone(vector pos, array<ref EZ_QOL_Zone> zones, out string name)
	{
		if (!zones)
			return false;
		foreach (EZ_QOL_Zone z : zones)
		{
			float dx = pos[0] - z.X;
			float dz = pos[2] - z.Z;
			if (dx * dx + dz * dz <= z.Radius * z.Radius)
			{
				name = z.Name;
				return true;
			}
		}
		return false;
	}

	// corners of the footprint in world space
	static void Corners(vector pos, vector ori, out array<vector> pts)
	{
		pts = new array<vector>;
		vector mat[3];
		Math3D.YawPitchRollMatrix(Vector(ori[0], 0, 0), mat);
		float hx = EZ_QOL_GARAGE_WIDTH * 0.5;
		float hz = EZ_QOL_GARAGE_LENGTH * 0.5;
		array<vector> ezCorners = {Vector(-hx, 0, -hz), Vector(hx, 0, -hz), Vector(-hx, 0, hz), Vector(hx, 0, hz), Vector(0, 0, 0)};
		foreach (vector l : ezCorners)
		{
			vector w = pos + l[0] * mat[0] + l[2] * mat[2];
			w[1] = GetGame().SurfaceY(w[0], w[2]);
			pts.Insert(w);
		}
	}

	// Returns true if OK. alignedPos/alignedOri: height and pitch/roll fitted to the terrain.
	static bool Validate(PlayerBase player, vector pos, vector ori, out vector alignedPos, out vector alignedOri, out string reasonKey, out string detail, Object ignore = null)
	{
		EZ_QOL_GarageConfig cfg = EZ_QOL_GarageConfig.Get();
		alignedPos = pos;
		alignedOri = ori;
		detail = "";
		if (!cfg.Enabled) { reasonKey = EZ_QOL_Reason.DISABLED; return false; }

	#ifdef EXPANSIONMODCORE
		if (!cfg.AllowInSafeZones && ExpansionZoneModule.IsInsideSafeZone(pos)) { reasonKey = EZ_QOL_Reason.SAFEZONE; return false; }
		if (!cfg.AllowInTraderZones && ExpansionZoneModule.IsInside(pos, ExpansionZoneType.MARKET)) { reasonKey = EZ_QOL_Reason.TRADER; return false; }
	#endif
		if (IsInZone(pos, cfg.BlacklistZones, detail)) { reasonKey = EZ_QOL_Reason.BLACKLIST; return false; }
		if (GetGame().SurfaceIsSea(pos[0], pos[2]) || GetGame().SurfaceIsPond(pos[0], pos[2])) { reasonKey = EZ_QOL_Reason.WATER; return false; }

		// slope: steepest side of the footprint
		array<vector> c;
		Corners(pos, ori, c);
		vector c0 = c[0]; vector c1 = c[1]; vector c2 = c[2]; vector c3 = c[3];
		float y0 = c0[1]; float y1 = c1[1]; float y2 = c2[1]; float y3 = c3[1];
		float pitch = Math.Atan2(((y2 + y3) - (y0 + y1)) * 0.5, EZ_QOL_GARAGE_LENGTH) * Math.RAD2DEG;
		float roll  = Math.Atan2(((y1 + y3) - (y0 + y2)) * 0.5, EZ_QOL_GARAGE_WIDTH) * Math.RAD2DEG;
		if (Math.AbsFloat(pitch) > cfg.MaxSlopeDeg || Math.AbsFloat(roll) > cfg.MaxSlopeDeg)
		{
			reasonKey = EZ_QOL_Reason.SLOPE;
			detail = Math.Round(Math.Max(Math.AbsFloat(pitch), Math.AbsFloat(roll))).ToString();
			return false;
		}
		float avgY = (y0 + y1 + y2 + y3) * 0.25;
		alignedPos = Vector(pos[0], avgY, pos[2]);
		alignedOri = Vector(ori[0], -pitch, roll); // UNVERIFIED sign convention, check in game

		// distance to other EZ_QOL garages
		array<Object> near = new array<Object>;
		GetGame().GetObjectsAtPosition3D(pos, cfg.MinDistanceBetweenGarages, near, null);
		foreach (Object o : near)
		{
			if (o != ignore && o.IsInherited(EZ_QOL_Garage))
			{
				reasonKey = EZ_QOL_Reason.TOO_CLOSE;
				detail = Math.Round(vector.Distance(pos, o.GetPosition())).ToString();
				return false;
			}
		}

		// clearance box (slightly shrunk, lifted off the ground so terrain doesn't count)
		vector center = alignedPos + Vector(0, EZ_QOL_GARAGE_HEIGHT * 0.5 + 0.3, 0);
		vector edge = Vector(EZ_QOL_GARAGE_WIDTH * 0.95, EZ_QOL_GARAGE_HEIGHT - 0.4, EZ_QOL_GARAGE_LENGTH * 0.95);
		array<Object> excluded = new array<Object>;
		if (player) excluded.Insert(player);
		if (ignore) excluded.Insert(ignore);
		array<Object> hits = new array<Object>;
		GetGame().IsBoxColliding(center, alignedOri, edge, excluded, hits);
		foreach (Object h : hits)
		{
			if (!h || h.IsBush() || h.IsInherited(Hologram) || IsIgnored(h, cfg.IgnoreObstacleClasses))
				continue;
			if (h.IsInherited(EZ_QOL_GarageKit))
				continue;
			reasonKey = EZ_QOL_Reason.BLOCKED;
			detail = h.GetDisplayName();
			if (detail == "") detail = h.GetType();
			return false;
		}

		reasonKey = EZ_QOL_Reason.OK;
		return true;
	}

	static bool IsIgnored(Object o, TStringArray classes)
	{
		if (!classes) return false;
		foreach (string cls : classes)
		{
			if (o.IsKindOf(cls))
				return true;
		}
		return false;
	}
}
